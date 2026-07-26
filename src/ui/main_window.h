#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QByteArray>
#include <QMainWindow>
#include <QString>
#include <QStringList>

#include "auto_hiding_dock_widget.h"
#include "canvas_widget.h"
#include "property_panel_widget.h"
#include "toolbar_widget.h"

class QAction;
class QCloseEvent;
class QMenu;
class QTimer;
class DiagnosticsWidget;
class HtmlEditorWidget;
class Shape;
class ShapeExplorerWidget;
struct Diagnostic;
struct DiagnosticMessage;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

    CanvasWidget *canvasWidget() const { return m_canvasWidget; }
    HtmlEditorWidget *htmlEditorWidget() const { return m_htmlEditor; }
    DiagnosticsWidget *diagnosticsWidget() const { return m_diagnosticsWidget; }
    ShapeExplorerWidget *shapeExplorerWidget() const { return m_shapeExplorer; }

    // 无文件对话框版本，供菜单动作和自动化测试共同复用。
    bool loadDocumentFile(const QString &filePath);
    bool saveDocumentFile(const QString &filePath);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    enum class LayoutPreset { Classic, DualCore, Zen, CodeFocused };

    void setupUI();
    void setupDocks();
    void setupMenus();
    void setupSynchronization();
    void buildLayoutPresets();
    void constructLayout(LayoutPreset preset);
    void restoreLayout(const QByteArray &state, LayoutPreset preset);

    void applyPresetClassic();
    void applyPresetDualCore();
    void applyPresetZen();
    void applyPresetCode();

    void newDocument();
    void openDocument();
    void saveDocument();
    void saveDocumentAs();
    void exportCanvasImage();
    void exportCanvasSvg();
    void exportFullHtml();
    void quitApplication();

    void synchronizeCanvasToEditor();
    void flushPendingCanvasSynchronization();
    void executeEditorContent();
    void formatEditorContent();
    bool applyEditorContent();
    void showDiagnostics(const QList<Diagnostic> &diagnostics);
    void showDiagnostics(const QList<DiagnosticMessage> &diagnostics);
    void goToEditorLine(int lineNumber);

    bool maybeSave();
    bool saveCurrentDocument();
    void establishCleanBaseline();
    void refreshModifiedState();
    void updateWindowTitle();
    void addRecentFile(const QString &filePath);
    void updateRecentFilesMenu();

    QList<Shape *> selectedShapes() const;
    void applyArrangement(int mode);
    void bringSelectedToFront();
    void sendSelectedBackward();

    CanvasWidget *m_canvasWidget = nullptr;
    PropertyPanelWidget *m_propertyPanel = nullptr;
    ToolBarWidget *m_toolbar = nullptr;
    HtmlEditorWidget *m_htmlEditor = nullptr;
    DiagnosticsWidget *m_diagnosticsWidget = nullptr;
    ShapeExplorerWidget *m_shapeExplorer = nullptr;

    AutoHidingDockWidget *m_toolbarDock = nullptr;
    AutoHidingDockWidget *m_canvasDock = nullptr;
    AutoHidingDockWidget *m_propertyDock = nullptr;
    AutoHidingDockWidget *m_codeEditorDock = nullptr;
    AutoHidingDockWidget *m_diagnosticsDock = nullptr;
    AutoHidingDockWidget *m_shapeExplorerDock = nullptr;

    QAction *m_actionPresetDefault = nullptr;
    QAction *m_actionPresetDualCore = nullptr;
    QAction *m_actionPresetZen = nullptr;
    QAction *m_actionPresetCodeFocused = nullptr;
    QAction *m_gridAction = nullptr;
    QMenu *m_recentFilesMenu = nullptr;
    QTimer *m_canvasSyncTimer = nullptr;

    QByteArray m_layoutClassic;
    QByteArray m_layoutDualCore;
    QByteArray m_layoutZen;
    QByteArray m_layoutCodeFocused;

    QString m_currentFilePath;
    QString m_savedDocumentContent;
    bool m_applyingEditorDocument = false;
    bool m_canvasSerializationPending = false;
};

#endif // MAIN_WINDOW_H
