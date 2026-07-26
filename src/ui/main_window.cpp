#include "main_window.h"

#include "../core/commands/undo_commands.h"
#include "../core/document/diagram_document.h"
#include "../core/document/document_parser.h"
#include "../core/document/document_serializer.h"
#include "../core/layout_engine/layout_engine.h"
#include "../core/undo_manager.h"
#include "../editor/diagnostics_widget.h"
#include "../editor/html_editor_widget.h"
#include "../export/scene_export_utils.h"
#include "../export/svg_exporter.h"
#include "../shapes/shape.h"
#include "shape_explorer_widget.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QSaveFile>
#include <QGraphicsScene>
#include <QImage>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QSettings>
#include <QStandardPaths>
#include <QTextBlock>
#include <QTextCursor>
#include <QTimer>
#include <QUndoCommand>

#include <algorithm>

namespace {
constexpr int kLayoutStateVersion = 2;
constexpr int kRecentFileLimit = 8;

DiagnosticMessage::Level diagnosticLevel(Diagnostic::Level level)
{
    switch (level) {
    case Diagnostic::Info:
        return DiagnosticMessage::Level::Info;
    case Diagnostic::Warning:
        return DiagnosticMessage::Level::Warning;
    case Diagnostic::Error:
        return DiagnosticMessage::Level::Error;
    }
    return DiagnosticMessage::Level::Error;
}

template <typename Receiver, typename Slot>
QAction *addMenuAction(QMenu *menu, const QString &text, const QKeySequence &shortcut,
                       Receiver *receiver, Slot slot)
{
    QAction *action = menu->addAction(text);
    action->setShortcut(shortcut);
    QObject::connect(action, &QAction::triggered, receiver, slot);
    return action;
}

QString defaultExportDirectory()
{
    const QString pictures = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    return pictures.isEmpty() ? QDir::homePath() : pictures;
}
} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
    setupDocks();
    setupMenus();
    setupSynchronization();

    resize(1400, 900);
    setMinimumSize(900, 600);
    setStyleSheet(QStringLiteral(
        "QMainWindow { background-color: #f0f2f5; }"
        "QMenuBar { background: #ffffff; color: #303133; border-bottom: 1px solid #e4e7ed; }"
        "QMenuBar::item { padding: 5px 9px; background: transparent; }"
        "QMenuBar::item:selected { background: #ecf5ff; color: #409eff; }"
        "QMenu { background: #ffffff; color: #303133; border: 1px solid #dcdfe6; padding: 5px; }"
        "QMenu::item { padding: 6px 28px 6px 24px; border-radius: 4px; }"
        "QMenu::item:selected { background: #ecf5ff; color: #409eff; }"));
    synchronizeCanvasToEditor();
    establishCleanBaseline();
    updateWindowTitle();
}

void MainWindow::setupUI()
{
    // QMainWindow 在部分 macOS/Qt 组合下不能安全地完全省略 centralWidget。
    // 保留一个零尺寸布局锚点，实际工作区全部使用 QDockWidget，以支持任意嵌套布局与浮动。
    auto *layoutAnchor = new QWidget(this);
    layoutAnchor->setObjectName(QStringLiteral("DockLayoutAnchor"));
    layoutAnchor->setFixedSize(0, 0);
    setCentralWidget(layoutAnchor);

    setDockOptions(QMainWindow::AllowNestedDocks |
                   QMainWindow::AllowTabbedDocks |
                   QMainWindow::AnimatedDocks |
                   QMainWindow::GroupedDragging);
}

void MainWindow::setupDocks()
{
    m_toolbar = new ToolBarWidget();
    m_toolbarDock = new AutoHidingDockWidget(QStringLiteral("工具栏"), false, this);
    m_toolbarDock->setObjectName(QStringLiteral("ToolbarDock"));
    m_toolbarDock->setWidget(m_toolbar);
    m_toolbarDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_toolbarDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

    m_canvasWidget = new CanvasWidget();
    m_canvasDock = new AutoHidingDockWidget(QStringLiteral("绘图区"), true, this);
    m_canvasDock->setObjectName(QStringLiteral("CanvasDock"));
    m_canvasDock->setWidget(m_canvasWidget);

    m_propertyPanel = new PropertyPanelWidget();
    m_propertyPanel->connectToCanvas(m_canvasWidget->canvas());
    m_propertyDock = new AutoHidingDockWidget(QStringLiteral("图形属性"), true, this);
    m_propertyDock->setObjectName(QStringLiteral("PropertyPanelDock"));
    m_propertyDock->setWidget(m_propertyPanel);

    m_htmlEditor = new HtmlEditorWidget();
    m_codeEditorDock = new AutoHidingDockWidget(QStringLiteral("代码编辑区"), true, this);
    m_codeEditorDock->setObjectName(QStringLiteral("CodeEditorDock"));
    m_codeEditorDock->setWidget(m_htmlEditor);

    m_diagnosticsWidget = new DiagnosticsWidget();
    m_diagnosticsDock = new AutoHidingDockWidget(QStringLiteral("代码报错警告区"), true, this);
    m_diagnosticsDock->setObjectName(QStringLiteral("DiagnosticsDock"));
    m_diagnosticsDock->setWidget(m_diagnosticsWidget);

    m_shapeExplorer = new ShapeExplorerWidget();
    m_shapeExplorer->setCanvas(m_canvasWidget->canvas());
    m_shapeExplorerDock = new AutoHidingDockWidget(QStringLiteral("图形列表"), true, this);
    m_shapeExplorerDock->setObjectName(QStringLiteral("ShapeExplorerDock"));
    m_shapeExplorerDock->setWidget(m_shapeExplorer);

    for (AutoHidingDockWidget *dock : {m_canvasDock, m_propertyDock, m_codeEditorDock,
                                      m_diagnosticsDock, m_shapeExplorerDock}) {
        dock->setAllowedAreas(Qt::AllDockWidgetAreas);
        dock->setFeatures(QDockWidget::DockWidgetClosable |
                          QDockWidget::DockWidgetMovable |
                          QDockWidget::DockWidgetFloatable);
    }

    connect(m_toolbar, &ToolBarWidget::toolChanged,
            m_canvasWidget, &CanvasWidget::setToolFromToolbar);

    buildLayoutPresets();
}

void MainWindow::constructLayout(LayoutPreset preset)
{
    const QList<QDockWidget *> docks = {m_toolbarDock, m_canvasDock, m_propertyDock,
                                         m_codeEditorDock, m_diagnosticsDock,
                                         m_shapeExplorerDock};
    for (QDockWidget *dock : docks) {
        removeDockWidget(dock);
        dock->setFloating(false);
        dock->hide();
    }

    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    addDockWidget(Qt::LeftDockWidgetArea, m_toolbarDock);
    m_toolbarDock->show();
    addDockWidget(Qt::RightDockWidgetArea, m_canvasDock);
    m_canvasDock->show();

    switch (preset) {
    case LayoutPreset::Classic:
        // H(V(Canvas, Editor), V(H(Explorer, Property), Diagnostics))
        splitDockWidget(m_canvasDock, m_shapeExplorerDock, Qt::Horizontal);
        splitDockWidget(m_canvasDock, m_codeEditorDock, Qt::Vertical);
        splitDockWidget(m_shapeExplorerDock, m_diagnosticsDock, Qt::Vertical);
        splitDockWidget(m_shapeExplorerDock, m_propertyDock, Qt::Horizontal);
        for (QDockWidget *dock : {m_codeEditorDock, m_shapeExplorerDock,
                                  m_propertyDock, m_diagnosticsDock})
            dock->show();
        resizeDocks({m_toolbarDock}, {92}, Qt::Horizontal);
        resizeDocks({m_canvasDock, m_shapeExplorerDock, m_propertyDock},
                    {650, 225, 255}, Qt::Horizontal);
        resizeDocks({m_canvasDock, m_codeEditorDock}, {520, 260}, Qt::Vertical);
        resizeDocks({m_shapeExplorerDock, m_diagnosticsDock}, {520, 260}, Qt::Vertical);
        break;

    case LayoutPreset::DualCore:
        // H(V(H(Canvas, Explorer), Property), V(Editor, Diagnostics))
        splitDockWidget(m_canvasDock, m_codeEditorDock, Qt::Horizontal);
        splitDockWidget(m_canvasDock, m_propertyDock, Qt::Vertical);
        splitDockWidget(m_canvasDock, m_shapeExplorerDock, Qt::Horizontal);
        splitDockWidget(m_codeEditorDock, m_diagnosticsDock, Qt::Vertical);
        for (QDockWidget *dock : {m_codeEditorDock, m_shapeExplorerDock,
                                  m_propertyDock, m_diagnosticsDock})
            dock->show();
        resizeDocks({m_toolbarDock}, {92}, Qt::Horizontal);
        resizeDocks({m_canvasDock, m_shapeExplorerDock, m_codeEditorDock},
                    {520, 255, 520}, Qt::Horizontal);
        resizeDocks({m_canvasDock, m_propertyDock}, {560, 220}, Qt::Vertical);
        resizeDocks({m_codeEditorDock, m_diagnosticsDock}, {560, 220}, Qt::Vertical);
        break;

    case LayoutPreset::Zen:
        // 纯净绘图仍保留可快速编辑的窄属性栏。
        splitDockWidget(m_canvasDock, m_propertyDock, Qt::Horizontal);
        m_propertyDock->show();
        resizeDocks({m_toolbarDock}, {84}, Qt::Horizontal);
        resizeDocks({m_canvasDock, m_propertyDock}, {1000, 290}, Qt::Horizontal);
        break;

    case LayoutPreset::CodeFocused:
        // 代码为主，画布保留预览，诊断固定在编辑器下方。
        splitDockWidget(m_canvasDock, m_codeEditorDock, Qt::Horizontal);
        splitDockWidget(m_codeEditorDock, m_diagnosticsDock, Qt::Vertical);
        m_codeEditorDock->show();
        m_diagnosticsDock->show();
        resizeDocks({m_toolbarDock}, {84}, Qt::Horizontal);
        resizeDocks({m_canvasDock, m_codeEditorDock}, {390, 900}, Qt::Horizontal);
        resizeDocks({m_codeEditorDock, m_diagnosticsDock}, {590, 190}, Qt::Vertical);
        break;
    }
}

void MainWindow::buildLayoutPresets()
{
    constructLayout(LayoutPreset::Classic);
    m_layoutClassic = saveState(kLayoutStateVersion);
    constructLayout(LayoutPreset::DualCore);
    m_layoutDualCore = saveState(kLayoutStateVersion);
    constructLayout(LayoutPreset::Zen);
    m_layoutZen = saveState(kLayoutStateVersion);
    constructLayout(LayoutPreset::CodeFocused);
    m_layoutCodeFocused = saveState(kLayoutStateVersion);
    restoreLayout(m_layoutClassic, LayoutPreset::Classic);
}

void MainWindow::restoreLayout(const QByteArray &state, LayoutPreset preset)
{
    // saveState() 负责恢复 Qt 可持久化的停靠元数据；随后始终以预设定义重新
    // 建立拓扑和可见性。主窗口尚未 show 时保存的状态在部分平台不会可靠记录
    // hidden dock，若把 restoreState() 放在最后，Zen/Code 模式会重新显示无关面板。
    if (!state.isEmpty())
        restoreState(state, kLayoutStateVersion);
    constructLayout(preset);
}

void MainWindow::applyPresetClassic() { restoreLayout(m_layoutClassic, LayoutPreset::Classic); }
void MainWindow::applyPresetDualCore() { restoreLayout(m_layoutDualCore, LayoutPreset::DualCore); }
void MainWindow::applyPresetZen() { restoreLayout(m_layoutZen, LayoutPreset::Zen); }
void MainWindow::applyPresetCode() { restoreLayout(m_layoutCodeFocused, LayoutPreset::CodeFocused); }

void MainWindow::setupSynchronization()
{
    m_canvasSyncTimer = new QTimer(this);
    m_canvasSyncTimer->setSingleShot(true);
    m_canvasSyncTimer->setInterval(25);
    connect(m_canvasSyncTimer, &QTimer::timeout,
            this, &MainWindow::synchronizeCanvasToEditor);

    connect(m_htmlEditor, &HtmlEditorWidget::executeRequested,
            this, &MainWindow::executeEditorContent);
    connect(m_htmlEditor, &HtmlEditorWidget::contentEdited,
            this, &MainWindow::refreshModifiedState);
    connect(m_canvasWidget->canvas()->undoManager(), &UndoManager::historyChanged,
            this, [this]() {
                // QGraphicsScene::changed 可能延迟到事件循环才发出；撤销、重做或
                // 命令入栈时先同步记录待序列化状态，保证立即保存不会遗漏变化。
                if (!m_applyingEditorDocument)
                    m_canvasSerializationPending = true;
                refreshModifiedState();
            });
    connect(m_canvasWidget->canvas()->scene(), &QGraphicsScene::changed,
            this, [this](const QList<QRectF> &) {
                if (!m_applyingEditorDocument) {
                    m_canvasSerializationPending = true;
                    m_canvasSyncTimer->start();
                }
            });
    connect(m_diagnosticsWidget, &DiagnosticsWidget::lineDoubleClicked,
            this, &MainWindow::goToEditorLine);
}

void MainWindow::setupMenus()
{
    QMenuBar *bar = menuBar();
    bar->setNativeMenuBar(true);

    QMenu *fileMenu = bar->addMenu(QStringLiteral("文件 (File)"));
    addMenuAction(fileMenu, QStringLiteral("新建 (New)"), QKeySequence::New,
                  this, &MainWindow::newDocument);
    addMenuAction(fileMenu, QStringLiteral("打开 (Open...)"), QKeySequence::Open,
                  this, &MainWindow::openDocument);
    m_recentFilesMenu = fileMenu->addMenu(QStringLiteral("最近打开 (Open Recent)"));
    updateRecentFilesMenu();
    fileMenu->addSeparator();
    QAction *saveAction = addMenuAction(
        fileMenu, QStringLiteral("保存 (Save)"), QKeySequence::Save,
        this, &MainWindow::saveDocument);
    saveAction->setObjectName(QStringLiteral("SaveDocumentAction"));
    addMenuAction(fileMenu, QStringLiteral("另存为 (Save As...)"), QKeySequence::SaveAs,
                  this, &MainWindow::saveDocumentAs);
    fileMenu->addSeparator();
    QAction *exportImage = fileMenu->addAction(QStringLiteral("导出画布图片 (Export Image...)"));
    connect(exportImage, &QAction::triggered, this, &MainWindow::exportCanvasImage);
    QAction *exportSvg = fileMenu->addAction(QStringLiteral("导出 SVG (Export SVG...)"));
    exportSvg->setObjectName(QStringLiteral("ExportSvgAction"));
    connect(exportSvg, &QAction::triggered, this, &MainWindow::exportCanvasSvg);
    QAction *exportHtml = fileMenu->addAction(QStringLiteral("导出完整 HTML (Export HTML...)"));
    connect(exportHtml, &QAction::triggered, this, &MainWindow::exportFullHtml);
    fileMenu->addSeparator();
    addMenuAction(fileMenu, QStringLiteral("退出 (Exit)"), QKeySequence::Quit,
                  this, &MainWindow::quitApplication);

    QMenu *editMenu = bar->addMenu(QStringLiteral("编辑 (Edit)"));
    // 由 QUndoStack 原生动作维护 enabled 状态及当前命令名称，避免菜单状态与撤销栈脱节。
    QAction *undoAction = m_canvasWidget->canvas()->undoManager()->stack()->createUndoAction(
        this, QStringLiteral("撤销"));
    undoAction->setObjectName(QStringLiteral("UndoAction"));
    undoAction->setShortcut(QKeySequence::Undo);
    editMenu->addAction(undoAction);
    QAction *redoAction = m_canvasWidget->canvas()->undoManager()->stack()->createRedoAction(
        this, QStringLiteral("重做"));
    redoAction->setObjectName(QStringLiteral("RedoAction"));
    redoAction->setShortcut(QKeySequence::Redo);
    editMenu->addAction(redoAction);
    editMenu->addSeparator();
    addMenuAction(editMenu, QStringLiteral("剪切 (Cut)"), QKeySequence::Cut,
                  m_canvasWidget->canvas(), &Canvas::cut);
    addMenuAction(editMenu, QStringLiteral("复制 (Copy)"), QKeySequence::Copy,
                  m_canvasWidget->canvas(), &Canvas::copy);
    addMenuAction(editMenu, QStringLiteral("粘贴 (Paste)"), QKeySequence::Paste,
                  m_canvasWidget->canvas(), &Canvas::paste);
    addMenuAction(editMenu, QStringLiteral("删除 (Delete)"), QKeySequence::Delete,
                  m_canvasWidget->canvas(), &Canvas::deleteSelected);
    addMenuAction(editMenu, QStringLiteral("全选 (Select All)"), QKeySequence::SelectAll,
                  m_canvasWidget->canvas(), &Canvas::selectAll);

    QMenu *windowMenu = bar->addMenu(QStringLiteral("窗口 (Window)"));
    auto *presetGroup = new QActionGroup(this);
    presetGroup->setExclusive(true);
    m_actionPresetDefault = windowMenu->addAction(QStringLiteral("经典混合视图 (Classic 2x3)"));
    m_actionPresetDualCore = windowMenu->addAction(QStringLiteral("双核对开视图 (Dual-Core Split)"));
    m_actionPresetZen = windowMenu->addAction(QStringLiteral("纯净绘图模式 (Zen / Pure Canvas)"));
    m_actionPresetCodeFocused = windowMenu->addAction(QStringLiteral("代码沉浸模式 (Code Focused)"));
    m_actionPresetDefault->setObjectName(QStringLiteral("PresetClassicAction"));
    m_actionPresetDualCore->setObjectName(QStringLiteral("PresetDualCoreAction"));
    m_actionPresetZen->setObjectName(QStringLiteral("PresetZenAction"));
    m_actionPresetCodeFocused->setObjectName(QStringLiteral("PresetCodeAction"));
    for (QAction *action : {m_actionPresetDefault, m_actionPresetDualCore,
                            m_actionPresetZen, m_actionPresetCodeFocused}) {
        action->setCheckable(true);
        presetGroup->addAction(action);
    }
    m_actionPresetDefault->setChecked(true);
    connect(m_actionPresetDefault, &QAction::triggered, this, &MainWindow::applyPresetClassic);
    connect(m_actionPresetDualCore, &QAction::triggered, this, &MainWindow::applyPresetDualCore);
    connect(m_actionPresetZen, &QAction::triggered, this, &MainWindow::applyPresetZen);
    connect(m_actionPresetCodeFocused, &QAction::triggered, this, &MainWindow::applyPresetCode);
    windowMenu->addSeparator();
    for (QDockWidget *dock : {m_toolbarDock, m_canvasDock, m_shapeExplorerDock,
                              m_propertyDock, m_codeEditorDock, m_diagnosticsDock})
        windowMenu->addAction(dock->toggleViewAction());

    QMenu *viewMenu = bar->addMenu(QStringLiteral("视图 (View)"));
    addMenuAction(viewMenu, QStringLiteral("放大 (Zoom In)"), QKeySequence::ZoomIn,
                  m_canvasWidget->canvas(), &Canvas::zoomIn);
    addMenuAction(viewMenu, QStringLiteral("缩小 (Zoom Out)"), QKeySequence::ZoomOut,
                  m_canvasWidget->canvas(), &Canvas::zoomOut);
    addMenuAction(viewMenu, QStringLiteral("实际大小 (100%)"), QKeySequence(Qt::CTRL | Qt::Key_0),
                  m_canvasWidget->canvas(), &Canvas::resetZoom);
    viewMenu->addSeparator();
    m_gridAction = viewMenu->addAction(QStringLiteral("显示网格 (Grid)"));
    m_gridAction->setObjectName(QStringLiteral("GridVisibilityAction"));
    m_gridAction->setCheckable(true);
    m_gridAction->setChecked(m_canvasWidget->canvas()->isGridVisible());
    connect(m_gridAction, &QAction::toggled,
            m_canvasWidget->canvas(), &Canvas::setGridVisible);
    connect(m_canvasWidget->canvas(), &Canvas::gridVisibilityChanged,
            m_gridAction, &QAction::setChecked);

    QMenu *arrangeMenu = bar->addMenu(QStringLiteral("排列 (Arrange)"));
    QAction *front = arrangeMenu->addAction(QStringLiteral("置于顶层 (Bring to Front)"));
    QAction *back = arrangeMenu->addAction(QStringLiteral("置于底层 (Send to Back)"));
    connect(front, &QAction::triggered, this, &MainWindow::bringSelectedToFront);
    connect(back, &QAction::triggered, this, &MainWindow::sendSelectedBackward);
    arrangeMenu->addSeparator();
    const QList<QPair<QString, LayoutConfig::Mode>> arrangements = {
        {QStringLiteral("水平排列"), LayoutConfig::Mode::HorizontalAlignment},
        {QStringLiteral("垂直排列"), LayoutConfig::Mode::VerticalAlignment},
        {QStringLiteral("网格排列"), LayoutConfig::Mode::GridAlignment},
        {QStringLiteral("统一宽度"), LayoutConfig::Mode::MatchWidth},
        {QStringLiteral("统一高度"), LayoutConfig::Mode::MatchHeight},
        {QStringLiteral("统一尺寸"), LayoutConfig::Mode::MatchSize}
    };
    for (const auto &entry : arrangements) {
        QAction *action = arrangeMenu->addAction(entry.first);
        connect(action, &QAction::triggered, this,
                [this, mode = entry.second]() { applyArrangement(static_cast<int>(mode)); });
    }

    QMenu *syncMenu = bar->addMenu(QStringLiteral("同步 (Sync)"));
    QAction *canvasToCode = syncMenu->addAction(QStringLiteral("强制画布同步到代码"));
    connect(canvasToCode, &QAction::triggered,
            this, &MainWindow::synchronizeCanvasToEditor);
    QAction *execute = syncMenu->addAction(QStringLiteral("执行代码并更新画布"));
    execute->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return));
    connect(execute, &QAction::triggered, this, &MainWindow::executeEditorContent);
    QAction *format = syncMenu->addAction(QStringLiteral("格式化文档 (Format)"));
    format->setObjectName(QStringLiteral("FormatDocumentAction"));
    connect(format, &QAction::triggered, this, &MainWindow::formatEditorContent);

    QMenu *helpMenu = bar->addMenu(QStringLiteral("帮助 (Help)"));
    QAction *guide = helpMenu->addAction(QStringLiteral("使用说明"));
    connect(guide, &QAction::triggered, this, [this]() {
        QMessageBox::information(this, QStringLiteral("AlgorithmViz 使用说明"),
            QStringLiteral("从左侧工具栏创建图形；画布修改会实时写入代码。\n"
                           "编辑代码后请点击编辑器右下角 ▶，或按 Ctrl+Enter，显式更新画布。\n"
                           "图形列表支持可见、锁定、重命名与拖拽图层管理。"));
    });
    QAction *about = helpMenu->addAction(QStringLiteral("关于 AlgorithmViz"));
    connect(about, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, QStringLiteral("关于 AlgorithmViz"),
            QStringLiteral("AlgorithmViz\nQt 6 / C++17 可视化图形与代码双向编辑器"));
    });
}

void MainWindow::newDocument()
{
    if (!maybeSave())
        return;

    m_applyingEditorDocument = true;
    m_canvasWidget->canvas()->clearScene();
    m_applyingEditorDocument = false;
    m_currentFilePath.clear();
    showDiagnostics(QList<DiagnosticMessage>{});
    synchronizeCanvasToEditor();
    m_canvasWidget->canvas()->undoManager()->clear();
    establishCleanBaseline();
    updateWindowTitle();
}

void MainWindow::openDocument()
{
    if (!maybeSave())
        return;

    const QString filePath = QFileDialog::getOpenFileName(
        this, QStringLiteral("打开 AlgorithmViz 文档"), QString(),
        QStringLiteral("HTML 文档 (*.html *.htm);;所有文件 (*)"));
    if (!filePath.isEmpty())
        loadDocumentFile(filePath);
}

void MainWindow::saveDocument()
{
    saveCurrentDocument();
}

void MainWindow::saveDocumentAs()
{
    QString filePath = QFileDialog::getSaveFileName(
        this, QStringLiteral("保存 AlgorithmViz 文档"), m_currentFilePath,
        QStringLiteral("HTML 文档 (*.html);;所有文件 (*)"));
    if (filePath.isEmpty())
        return;
    if (QFileInfo(filePath).suffix().isEmpty())
        filePath += QStringLiteral(".html");
    saveDocumentFile(filePath);
}

void MainWindow::exportCanvasImage()
{
    QString filePath = QFileDialog::getSaveFileName(
        this, QStringLiteral("导出画布图片"),
        defaultExportDirectory() + QStringLiteral("/AlgorithmViz.png"),
        QStringLiteral("PNG 图片 (*.png);;JPEG 图片 (*.jpg *.jpeg)"));
    if (filePath.isEmpty())
        return;
    if (QFileInfo(filePath).suffix().isEmpty())
        filePath += QStringLiteral(".png");

    QGraphicsScene *scene = m_canvasWidget->canvas()->scene();
    SceneExport::ScopedOverlayHider overlayHider(scene);
    QRectF bounds = SceneExport::visibleItemsBoundingRect(scene);
    if (!bounds.isValid() || bounds.isEmpty())
        bounds = QRectF(0, 0, 1280, 720);
    bounds.adjust(-24, -24, 24, 24);

    const QSize imageSize = SceneExport::boundedOutputSize(bounds.size(), 2.0, 4096);
    if (imageSize.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("导出失败"),
                             QStringLiteral("画布边界无效，无法生成图片。"));
        return;
    }
    QImage image(imageSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    scene->render(&painter, QRectF(QPointF(0, 0), image.size()), bounds);
    painter.end();

    const QString suffix = QFileInfo(filePath).suffix().toLower();
    const QByteArray format = (suffix == QStringLiteral("jpg") ||
                               suffix == QStringLiteral("jpeg"))
        ? QByteArrayLiteral("JPEG") : QByteArrayLiteral("PNG");
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly) ||
        !image.save(&file, format.constData()) || !file.commit()) {
        QMessageBox::warning(this, QStringLiteral("导出失败"),
                             file.errorString().isEmpty()
                                 ? QStringLiteral("无法写入所选图片文件。")
                                 : file.errorString());
    }
}

void MainWindow::exportCanvasSvg()
{
    QString filePath = QFileDialog::getSaveFileName(
        this, QStringLiteral("导出 SVG"),
        defaultExportDirectory() + QStringLiteral("/AlgorithmViz.svg"),
        QStringLiteral("SVG 矢量图 (*.svg)"));
    if (filePath.isEmpty())
        return;
    if (QFileInfo(filePath).suffix().isEmpty())
        filePath += QStringLiteral(".svg");

    QString errorMessage;
    if (!SvgExporter::exportScene(m_canvasWidget->canvas()->scene(), filePath, &errorMessage))
        QMessageBox::warning(this, QStringLiteral("导出失败"), errorMessage);
}

void MainWindow::exportFullHtml()
{
    QString filePath = QFileDialog::getSaveFileName(
        this, QStringLiteral("导出完整 HTML"), m_currentFilePath,
        QStringLiteral("HTML 文档 (*.html)"));
    if (filePath.isEmpty())
        return;
    if (QFileInfo(filePath).suffix().isEmpty())
        filePath += QStringLiteral(".html");

    flushPendingCanvasSynchronization();
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("导出失败"), file.errorString());
        return;
    }

    const QByteArray content = m_htmlEditor->getHtmlContent().toUtf8();
    if (file.write(content) != content.size() || !file.commit()) {
        QMessageBox::warning(this, QStringLiteral("导出失败"), file.errorString());
    }
}

void MainWindow::quitApplication() { close(); }

bool MainWindow::loadDocumentFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("打开失败"), file.errorString());
        return false;
    }

    const QString html = QString::fromUtf8(file.readAll());
    m_htmlEditor->setHtmlContent(html);

    // 文件已经成功读入后，当前文档的归属就应切换到该文件。即使语法错误导致
    // 画布仍保留上一次可用预览，后续“保存”也必须写回正在修复的新文件，不能
    // 误覆盖此前打开的文档。磁盘原文作为新的干净基线保留在编辑器中。
    m_currentFilePath = QFileInfo(filePath).absoluteFilePath();
    addRecentFile(m_currentFilePath);
    const bool applied = applyEditorContent();
    establishCleanBaseline();
    updateWindowTitle();
    return applied;
}

bool MainWindow::saveDocumentFile(const QString &filePath)
{
    // 画布序列化使用短延时合并高频变化；显式保存不能把尚在定时器中的
    // 最新场景遗漏到磁盘。
    flushPendingCanvasSynchronization();

    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("保存失败"), file.errorString());
        return false;
    }

    const QByteArray content = m_htmlEditor->getHtmlContent().toUtf8();
    if (file.write(content) != content.size() || !file.commit()) {
        QMessageBox::warning(this, QStringLiteral("保存失败"), file.errorString());
        return false;
    }

    m_currentFilePath = QFileInfo(filePath).absoluteFilePath();
    addRecentFile(m_currentFilePath);
    establishCleanBaseline();
    updateWindowTitle();
    return true;
}

bool MainWindow::saveCurrentDocument()
{
    if (!m_currentFilePath.isEmpty())
        return saveDocumentFile(m_currentFilePath);

    QString filePath = QFileDialog::getSaveFileName(
        this, QStringLiteral("保存 AlgorithmViz 文档"), QString(),
        QStringLiteral("HTML 文档 (*.html);;所有文件 (*)"));
    if (filePath.isEmpty())
        return false;
    if (QFileInfo(filePath).suffix().isEmpty())
        filePath += QStringLiteral(".html");
    return saveDocumentFile(filePath);
}

bool MainWindow::maybeSave()
{
    // close/new/open 可能发生在场景 changed 信号的合并定时器触发之前。
    flushPendingCanvasSynchronization();
    if (!isWindowModified())
        return true;

    const QMessageBox::StandardButton choice = QMessageBox::warning(
        this, QStringLiteral("保存更改"),
        QStringLiteral("当前文档包含尚未保存的更改。是否先保存？"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Save);
    if (choice == QMessageBox::Cancel)
        return false;
    if (choice == QMessageBox::Save)
        return saveCurrentDocument();
    return true;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave())
        event->accept();
    else
        event->ignore();
}

void MainWindow::synchronizeCanvasToEditor()
{
    if (m_applyingEditorDocument || !m_canvasWidget || !m_htmlEditor)
        return;
    const DiagramDocument document =
        DiagramDocument::fromScene(m_canvasWidget->canvas()->scene());
    m_htmlEditor->setHtmlContent(DocumentSerializer::serialize(document));
    m_canvasSerializationPending = false;
    refreshModifiedState();
}

void MainWindow::flushPendingCanvasSynchronization()
{
    if (m_applyingEditorDocument || !m_canvasWidget || !m_htmlEditor)
        return;

    const bool timerPending = m_canvasSyncTimer && m_canvasSyncTimer->isActive();
    if (!m_canvasSerializationPending && !timerPending)
        return;

    if (m_canvasSyncTimer)
        m_canvasSyncTimer->stop();
    synchronizeCanvasToEditor();
}

void MainWindow::executeEditorContent() { applyEditorContent(); }

void MainWindow::formatEditorContent()
{
    DiagramDocument document;
    const LoadResult result = DocumentParser::parse(m_htmlEditor->getHtmlContent(), document);
    showDiagnostics(result.diagnostics);
    if (result.success) {
        m_htmlEditor->setHtmlContent(DocumentSerializer::serialize(document));
        refreshModifiedState();
    }
}

bool MainWindow::applyEditorContent()
{
    DiagramDocument document;
    const LoadResult parseResult = DocumentParser::parse(m_htmlEditor->getHtmlContent(), document);
    showDiagnostics(parseResult.diagnostics);
    if (!parseResult.success)
        return false;

    m_applyingEditorDocument = true;
    const LoadResult loadResult = document.applyToScene(
        m_canvasWidget->canvas()->scene(), m_canvasWidget->canvas()->undoManager());
    m_applyingEditorDocument = false;

    if (!loadResult.diagnostics.isEmpty())
        showDiagnostics(loadResult.diagnostics);
    if (!loadResult.success)
        return false;

    m_canvasWidget->canvas()->undoManager()->clear();
    synchronizeCanvasToEditor();
    return true;
}

void MainWindow::showDiagnostics(const QList<Diagnostic> &diagnostics)
{
    QList<DiagnosticMessage> messages;
    messages.reserve(diagnostics.size());
    for (const Diagnostic &diagnostic : diagnostics)
        messages.append({diagnosticLevel(diagnostic.level), diagnostic.lineNumber, diagnostic.message});
    showDiagnostics(messages);
}

void MainWindow::showDiagnostics(const QList<DiagnosticMessage> &diagnostics)
{
    m_diagnosticsWidget->clearMessages();
    for (const DiagnosticMessage &message : diagnostics)
        m_diagnosticsWidget->addMessage(message);
    m_htmlEditor->setDiagnostics(diagnostics);
}

void MainWindow::goToEditorLine(int lineNumber)
{
    if (lineNumber <= 0)
        return;
    QTextBlock block = m_htmlEditor->textEdit()->document()->findBlockByNumber(lineNumber - 1);
    if (!block.isValid())
        return;
    m_htmlEditor->textEdit()->setTextCursor(QTextCursor(block));
    m_htmlEditor->textEdit()->setFocus();
    m_codeEditorDock->show();
}

void MainWindow::establishCleanBaseline()
{
    // 当前编辑器内容在此成为唯一的磁盘基线。打开语法错误的文档时画布会保留
    // 上一次有效预览，因此还必须取消旧场景留下的延迟序列化，避免覆盖源文本。
    if (m_canvasSyncTimer)
        m_canvasSyncTimer->stop();
    m_canvasSerializationPending = false;
    m_savedDocumentContent = m_htmlEditor->getHtmlContent();
    setWindowModified(false);
    if (m_canvasWidget && m_canvasWidget->canvas() && m_canvasWidget->canvas()->undoManager())
        m_canvasWidget->canvas()->undoManager()->stack()->setClean();
}

void MainWindow::refreshModifiedState()
{
    const bool editorModified =
        m_htmlEditor && m_htmlEditor->getHtmlContent() != m_savedDocumentContent;
    const UndoManager *undoManager = (m_canvasWidget && m_canvasWidget->canvas())
        ? m_canvasWidget->canvas()->undoManager() : nullptr;
    const bool canvasModified = undoManager && !undoManager->stack()->isClean();
    setWindowModified(editorModified || canvasModified);
}

void MainWindow::updateWindowTitle()
{
    const QString documentName = m_currentFilePath.isEmpty()
        ? QStringLiteral("未命名") : QFileInfo(m_currentFilePath).fileName();
    setWindowTitle(QStringLiteral("%1[*] — AlgorithmViz").arg(documentName));
}

void MainWindow::addRecentFile(const QString &filePath)
{
    QSettings settings;
    QStringList files = settings.value(QStringLiteral("recentFiles")).toStringList();
    files.removeAll(filePath);
    files.prepend(filePath);
    while (files.size() > kRecentFileLimit)
        files.removeLast();
    settings.setValue(QStringLiteral("recentFiles"), files);
    updateRecentFilesMenu();
}

void MainWindow::updateRecentFilesMenu()
{
    if (!m_recentFilesMenu)
        return;
    m_recentFilesMenu->clear();
    QSettings settings;
    QStringList files = settings.value(QStringLiteral("recentFiles")).toStringList();
    files.erase(std::remove_if(files.begin(), files.end(),
                               [](const QString &path) { return !QFileInfo::exists(path); }),
                files.end());
    settings.setValue(QStringLiteral("recentFiles"), files);
    if (files.isEmpty()) {
        QAction *empty = m_recentFilesMenu->addAction(QStringLiteral("暂无最近文件"));
        empty->setEnabled(false);
        return;
    }
    for (const QString &path : files) {
        QAction *action = m_recentFilesMenu->addAction(QFileInfo(path).fileName());
        action->setToolTip(path);
        connect(action, &QAction::triggered, this, [this, path]() {
            if (maybeSave())
                loadDocumentFile(path);
        });
    }
    m_recentFilesMenu->addSeparator();
    QAction *clear = m_recentFilesMenu->addAction(QStringLiteral("清除记录"));
    connect(clear, &QAction::triggered, this, [this]() {
        QSettings().remove(QStringLiteral("recentFiles"));
        updateRecentFilesMenu();
    });
}

QList<Shape *> MainWindow::selectedShapes() const
{
    QList<Shape *> result;
    for (QGraphicsItem *item : m_canvasWidget->canvas()->scene()->selectedItems()) {
        if (auto *shape = dynamic_cast<Shape *>(item); shape && !shape->parentItem())
            result.append(shape);
    }
    return result;
}

void MainWindow::applyArrangement(int modeValue)
{
    const QList<Shape *> shapes = selectedShapes();
    if (shapes.size() < 2)
        return;
    LayoutConfig config;
    config.mode = static_cast<LayoutConfig::Mode>(modeValue);
    LayoutEngine engine(config);
    LayoutSnapshot snapshot;
    switch (config.mode) {
    case LayoutConfig::Mode::HorizontalAlignment:
        snapshot = engine.alignHorizontal(shapes, shapes.first());
        break;
    case LayoutConfig::Mode::VerticalAlignment:
        snapshot = engine.alignVertical(shapes, shapes.first());
        break;
    case LayoutConfig::Mode::GridAlignment:
        snapshot = engine.arrangeGrid(shapes, shapes.first());
        break;
    case LayoutConfig::Mode::MatchWidth:
    case LayoutConfig::Mode::MatchHeight:
    case LayoutConfig::Mode::MatchSize:
        snapshot = engine.matchSize(shapes, shapes.first());
        break;
    case LayoutConfig::Mode::Null:
        return;
    }
    if (!snapshot.isEmpty())
        m_canvasWidget->canvas()->undoManager()->push(
            new LayoutSnapshotCommand(snapshot, QStringLiteral("Arrange Shapes")));
}

void MainWindow::bringSelectedToFront()
{
    m_canvasWidget->canvas()->bringSelectedToFront();
}

void MainWindow::sendSelectedBackward()
{
    m_canvasWidget->canvas()->sendSelectedToBack();
}
