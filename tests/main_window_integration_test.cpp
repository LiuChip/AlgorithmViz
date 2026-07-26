#include <QtTest>

#include "core/commands/undo_commands.h"
#include "core/undo_manager.h"
#include "editor/diagnostics_widget.h"
#include "editor/html_editor_widget.h"
#include "shapes/connector/connector.h"
#include "shapes/rect_shape.h"
#include "shapes/shape.h"
#include "shapes/text_label.h"
#include "ui/main_window.h"
#include "ui/property_panel_widget.h"

#include <QAction>
#include <QDockWidget>
#include <QFile>
#include <QFontComboBox>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QToolButton>
#include <QGraphicsScene>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <QUndoCommand>

#include <limits>

namespace {

int rootShapeCount(const QGraphicsScene *scene)
{
    int count = 0;
    for (QGraphicsItem *item : scene->items()) {
        if (!item->parentItem() && dynamic_cast<Shape *>(item))
            ++count;
    }
    return count;
}

QString oneRectangleDocument()
{
    return QStringLiteral(
        "<div id=\"canvas\" class=\"viz-container\">\n"
        "  <rect id=\"101\" data-name=\"Test Rectangle\" data-layer=\"4\" "
        "x=\"10\" y=\"20\" width=\"100\" height=\"50\"/>\n"
        "</div>\n");
}

} // namespace

class MainWindowIntegrationTest : public QObject
{
    Q_OBJECT

private slots:
    void testEditorOnlyChangesCanvasAfterExplicitExecute()
    {
        MainWindow window;
        QGraphicsScene *scene = window.canvasWidget()->canvas()->scene();
        QCOMPARE(rootShapeCount(scene), 0);

        window.htmlEditorWidget()->setHtmlContent(oneRectangleDocument());
        QCoreApplication::processEvents();
        QCOMPARE(rootShapeCount(scene), 0);

        QPushButton *executeButton = window.htmlEditorWidget()->findChild<QPushButton *>(
            QStringLiteral("HtmlExecuteButton"));
        QVERIFY(executeButton);
        executeButton->click();
        QTRY_COMPARE(rootShapeCount(scene), 1);
    }

    void testMalformedEditorContentPreservesCanvasAndReportsLine()
    {
        MainWindow window;
        QPushButton *executeButton = window.htmlEditorWidget()->findChild<QPushButton *>(
            QStringLiteral("HtmlExecuteButton"));
        QVERIFY(executeButton);

        window.htmlEditorWidget()->setHtmlContent(oneRectangleDocument());
        executeButton->click();
        QTRY_COMPARE(rootShapeCount(window.canvasWidget()->canvas()->scene()), 1);

        window.htmlEditorWidget()->setHtmlContent(QStringLiteral(
            "<div id=\"canvas\">\n"
            "  <rect id=\"102\" x=\"0\" y=\"0\" width=\"20\" height=\"20\">\n"
            "</div>\n"));
        executeButton->click();

        QCOMPARE(rootShapeCount(window.canvasWidget()->canvas()->scene()), 1);
        QTreeWidget *diagnostics = window.diagnosticsWidget()->treeWidget();
        QVERIFY(diagnostics->topLevelItemCount() > 0);
        QCOMPARE(diagnostics->topLevelItem(0)->text(1), QStringLiteral("3"));
    }

    void testGridActionControlsCanvasGrid()
    {
        MainWindow window;
        Canvas *canvas = window.canvasWidget()->canvas();
        QVERIFY(!canvas->isGridVisible());

        QAction *grid = window.findChild<QAction *>(QStringLiteral("GridVisibilityAction"));
        QVERIFY(grid);
        QVERIFY(grid->isCheckable());
        grid->setChecked(true);
        QVERIFY(canvas->isGridVisible());
        grid->setChecked(false);
        QVERIFY(!canvas->isGridVisible());
    }

    void testSceneSelectionUpdatesPropertyPanel()
    {
        MainWindow window;
        window.resize(1400, 900);
        window.show();
        QTest::qWait(40);

        window.htmlEditorWidget()->setHtmlContent(oneRectangleDocument());
        QPushButton *executeButton = window.htmlEditorWidget()->findChild<QPushButton *>(
            QStringLiteral("HtmlExecuteButton"));
        QVERIFY(executeButton);
        executeButton->click();
        QTRY_COMPARE(rootShapeCount(window.canvasWidget()->canvas()->scene()), 1);

        Shape *shape = nullptr;
        for (QGraphicsItem *item : window.canvasWidget()->canvas()->scene()->items()) {
            if (auto *candidate = dynamic_cast<Shape *>(item); candidate && !candidate->parentItem()) {
                shape = candidate;
                break;
            }
        }
        QVERIFY(shape);
        shape->setSelected(true);

        PropertyPanelWidget *propertyPanel = window.findChild<PropertyPanelWidget *>(
            QStringLiteral("PropertyPanel"));
        QVERIFY(propertyPanel);
        QLineEdit *nameEdit = propertyPanel->findChild<QLineEdit *>(
            QStringLiteral("PropertyShapeNameEdit"));
        QWidget *form = propertyPanel->findChild<QWidget *>(
            QStringLiteral("PropertyFormContainer"));
        QVERIFY(nameEdit);
        QVERIFY(form);
        QTRY_COMPARE(nameEdit->text(), QStringLiteral("Test Rectangle"));
        QTRY_VERIFY(form->isVisible());
    }

    void testClassicPresetMatchesRequestedDockTopology()
    {
        MainWindow window;
        window.resize(1400, 900);
        window.show();
        QTest::qWait(80);

        QAction *classic = window.findChild<QAction *>(QStringLiteral("PresetClassicAction"));
        QVERIFY(classic);
        classic->trigger();
        QCoreApplication::processEvents();

        auto *toolbar = window.findChild<QDockWidget *>(QStringLiteral("ToolbarDock"));
        auto *canvas = window.findChild<QDockWidget *>(QStringLiteral("CanvasDock"));
        auto *editor = window.findChild<QDockWidget *>(QStringLiteral("CodeEditorDock"));
        auto *explorer = window.findChild<QDockWidget *>(QStringLiteral("ShapeExplorerDock"));
        auto *property = window.findChild<QDockWidget *>(QStringLiteral("PropertyPanelDock"));
        auto *diagnostics = window.findChild<QDockWidget *>(QStringLiteral("DiagnosticsDock"));
        QVERIFY(toolbar && canvas && editor && explorer && property && diagnostics);

        QVERIFY(toolbar->geometry().center().x() < canvas->geometry().left());
        QVERIFY(editor->geometry().top() >= canvas->geometry().bottom() - 12);
        QVERIFY(explorer->geometry().left() >= canvas->geometry().right() - 12);
        QVERIFY(property->geometry().left() >= explorer->geometry().right() - 12);
        QVERIFY(diagnostics->geometry().top() >= explorer->geometry().bottom() - 12);
        QVERIFY(qAbs(diagnostics->geometry().left() - explorer->geometry().left()) <= 16);
        QVERIFY(qAbs(diagnostics->geometry().right() - property->geometry().right()) <= 16);
    }

    void testDualCorePresetMatchesRequestedDockTopology()
    {
        MainWindow window;
        window.resize(1400, 900);
        window.show();
        QTest::qWait(80);

        QAction *dual = window.findChild<QAction *>(QStringLiteral("PresetDualCoreAction"));
        QVERIFY(dual);
        dual->trigger();
        QCoreApplication::processEvents();

        auto *canvas = window.findChild<QDockWidget *>(QStringLiteral("CanvasDock"));
        auto *editor = window.findChild<QDockWidget *>(QStringLiteral("CodeEditorDock"));
        auto *explorer = window.findChild<QDockWidget *>(QStringLiteral("ShapeExplorerDock"));
        auto *property = window.findChild<QDockWidget *>(QStringLiteral("PropertyPanelDock"));
        auto *diagnostics = window.findChild<QDockWidget *>(QStringLiteral("DiagnosticsDock"));
        QVERIFY(canvas && editor && explorer && property && diagnostics);

        QVERIFY(explorer->geometry().left() >= canvas->geometry().right() - 12);
        QVERIFY(property->geometry().top() >= canvas->geometry().bottom() - 12);
        QVERIFY(qAbs(property->geometry().left() - canvas->geometry().left()) <= 16);
        QVERIFY(qAbs(property->geometry().right() - explorer->geometry().right()) <= 16);
        QVERIFY(editor->geometry().left() >= explorer->geometry().right() - 12);
        QVERIFY(diagnostics->geometry().top() >= editor->geometry().bottom() - 12);
        QVERIFY(qAbs(diagnostics->geometry().left() - editor->geometry().left()) <= 16);
        QVERIFY(qAbs(diagnostics->geometry().right() - editor->geometry().right()) <= 16);
    }

    void testZenPresetOnlyKeepsDrawingWorkspaceVisible()
    {
        MainWindow window;
        window.resize(1400, 900);
        window.show();
        QTest::qWait(80);

        QAction *zen = window.findChild<QAction *>(QStringLiteral("PresetZenAction"));
        QVERIFY(zen);
        zen->trigger();
        QCoreApplication::processEvents();

        auto *toolbar = window.findChild<QDockWidget *>(QStringLiteral("ToolbarDock"));
        auto *canvas = window.findChild<QDockWidget *>(QStringLiteral("CanvasDock"));
        auto *editor = window.findChild<QDockWidget *>(QStringLiteral("CodeEditorDock"));
        auto *explorer = window.findChild<QDockWidget *>(QStringLiteral("ShapeExplorerDock"));
        auto *property = window.findChild<QDockWidget *>(QStringLiteral("PropertyPanelDock"));
        auto *diagnostics = window.findChild<QDockWidget *>(QStringLiteral("DiagnosticsDock"));
        QVERIFY(toolbar && canvas && editor && explorer && property && diagnostics);

        QVERIFY(toolbar->isVisible());
        QVERIFY(canvas->isVisible());
        QVERIFY(property->isVisible());
        QVERIFY(!editor->isVisible());
        QVERIFY(!explorer->isVisible());
        QVERIFY(!diagnostics->isVisible());
        QVERIFY(toolbar->geometry().center().x() < canvas->geometry().left());
        QVERIFY(property->geometry().left() >= canvas->geometry().right() - 12);
    }

    void testCodeFocusedPresetOnlyKeepsCodeWorkspaceVisible()
    {
        MainWindow window;
        window.resize(1400, 900);
        window.show();
        QTest::qWait(80);

        QAction *code = window.findChild<QAction *>(QStringLiteral("PresetCodeAction"));
        QVERIFY(code);
        code->trigger();
        QCoreApplication::processEvents();

        auto *toolbar = window.findChild<QDockWidget *>(QStringLiteral("ToolbarDock"));
        auto *canvas = window.findChild<QDockWidget *>(QStringLiteral("CanvasDock"));
        auto *editor = window.findChild<QDockWidget *>(QStringLiteral("CodeEditorDock"));
        auto *explorer = window.findChild<QDockWidget *>(QStringLiteral("ShapeExplorerDock"));
        auto *property = window.findChild<QDockWidget *>(QStringLiteral("PropertyPanelDock"));
        auto *diagnostics = window.findChild<QDockWidget *>(QStringLiteral("DiagnosticsDock"));
        QVERIFY(toolbar && canvas && editor && explorer && property && diagnostics);

        QVERIFY(toolbar->isVisible());
        QVERIFY(canvas->isVisible());
        QVERIFY(editor->isVisible());
        QVERIFY(diagnostics->isVisible());
        QVERIFY(!explorer->isVisible());
        QVERIFY(!property->isVisible());
        QVERIFY(editor->geometry().left() >= canvas->geometry().right() - 12);
        QVERIFY(diagnostics->geometry().top() >= editor->geometry().bottom() - 12);
    }


    void testViewMenuMatchesDocumentedActions()
    {
        MainWindow window;
        QMenu *viewMenu = nullptr;
        for (QAction *action : window.menuBar()->actions()) {
            if (action->menu() && action->text().startsWith(QStringLiteral("视图"))) {
                viewMenu = action->menu();
                break;
            }
        }
        QVERIFY(viewMenu);
        for (QAction *action : viewMenu->actions()) {
            QVERIFY2(!action->text().contains(QStringLiteral("Fit Scene")),
                     "View menu must follow the handover spec and not expose Fit Scene");
            QVERIFY2(!action->text().contains(QStringLiteral("适合全部")),
                     "View menu must follow the handover spec and not expose Fit Scene");
        }
    }

    void testEditorDirtyStateAndSaveLifecycle()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString filePath = directory.filePath(QStringLiteral("dirty-state.html"));

        MainWindow window;
        QVERIFY(!window.isWindowModified());
        window.htmlEditorWidget()->textEdit()->appendPlainText(QStringLiteral("<!-- user edit -->"));
        QTRY_VERIFY(window.isWindowModified());

        QVERIFY(window.saveDocumentFile(filePath));
        QVERIFY(!window.isWindowModified());
    }

    void testFormatActionMarksProgrammaticEditorContentModified()
    {
        MainWindow window;
        window.htmlEditorWidget()->setHtmlContent(QStringLiteral(
            "<div id=\"canvas\" class=\"viz-container\"><rect id=\"101\" "
            "x=\"10\" y=\"20\" width=\"100\" height=\"50\"/></div>"));
        QVERIFY(!window.isWindowModified());

        QAction *format = window.findChild<QAction *>(QStringLiteral("FormatDocumentAction"));
        QVERIFY(format);
        format->trigger();

        QVERIFY(window.isWindowModified());
        QVERIFY(window.htmlEditorWidget()->getHtmlContent().contains(QLatin1Char('\n')));
    }

    void testUndoRedoMenuActionsFollowUndoStackState()
    {
        MainWindow window;
        QAction *undo = window.findChild<QAction *>(QStringLiteral("UndoAction"));
        QAction *redo = window.findChild<QAction *>(QStringLiteral("RedoAction"));
        QVERIFY(undo && redo);
        QVERIFY(!undo->isEnabled());
        QVERIFY(!redo->isEnabled());

        window.canvasWidget()->canvas()->undoManager()->push(
            new QUndoCommand(QStringLiteral("Synthetic Change")));
        QVERIFY(undo->isEnabled());
        QVERIFY(!redo->isEnabled());

        undo->trigger();
        QVERIFY(!undo->isEnabled());
        QVERIFY(redo->isEnabled());
    }

    void testPropertyPanelExposesCompleteTypographyAndHexControls()
    {
        MainWindow window;
        QVERIFY(window.findChild<QFontComboBox *>(QStringLiteral("FontFamilyCombo")));
        QVERIFY(window.findChild<QToolButton *>(QStringLiteral("BoldButton")));
        QVERIFY(window.findChild<QToolButton *>(QStringLiteral("ItalicButton")));
        QVERIFY(window.findChild<QComboBox *>(QStringLiteral("TextAlignmentCombo")));
        QVERIFY(window.findChild<QLineEdit *>(QStringLiteral("BorderHexEdit")));
        QVERIFY(window.findChild<QLineEdit *>(QStringLiteral("FillHexEdit")));
        QVERIFY(window.findChild<QLineEdit *>(QStringLiteral("TextHexEdit")));
        QVERIFY(window.findChild<QComboBox *>(QStringLiteral("TextLayoutModeCombo")));
    }

    void testStandalonePropertyPanelAppliesTextLayoutWithoutCanvas()
    {
        PropertyPanelWidget panel;
        TextLabel label(QPointF(0, 0), QStringLiteral("Standalone"));
        panel.setShape(&label);

        auto *layoutMode = panel.findChild<QComboBox *>(QStringLiteral("TextLayoutModeCombo"));
        QVERIFY(layoutMode);
        layoutMode->setCurrentIndex(layoutMode->findData(int(TextLabel::TextLayoutMode::FixedSize)));

        QCOMPARE(label.getTextLayoutMode(), TextLabel::TextLayoutMode::FixedSize);
    }

    void testStandalonePropertyPanelSwitchesConnectorEndStyle()
    {
        PropertyPanelWidget panel;
        Connector connector(QPointF(0.0, 0.0), QPointF(100.0, 0.0));
        panel.setShape(&connector);

        auto *endStyle = panel.findChild<QComboBox *>(QStringLiteral("ConnectorEndStyleCombo"));
        QVERIFY(endStyle);
        QCOMPARE(endStyle->currentData().toInt(), int(Connector::EndStyle::None));

        endStyle->setCurrentIndex(endStyle->findData(int(Connector::EndStyle::Arrow)));
        QCOMPARE(connector.getEndStyle(), Connector::EndStyle::Arrow);

        endStyle->setCurrentIndex(endStyle->findData(int(Connector::EndStyle::None)));
        QCOMPARE(connector.getEndStyle(), Connector::EndStyle::None);
    }

    void testPropertyPanelLayerActionsRejectIntegerOverflow()
    {
        Canvas canvas;
        PropertyPanelWidget panel;
        panel.connectToCanvas(&canvas);

        auto *shape = new RectShape(QPointF(0.0, 0.0), QSizeF(80.0, 50.0));
        canvas.scene()->addItem(shape);
        panel.setShape(shape);

        shape->setLayer(std::numeric_limits<int>::max());
        QVERIFY(QMetaObject::invokeMethod(&panel, "onBringToFront", Qt::DirectConnection));
        QCOMPARE(shape->getLayer(), std::numeric_limits<int>::max());
        QCOMPARE(canvas.undoManager()->stack()->count(), 0);

        shape->setLayer(std::numeric_limits<int>::min());
        QVERIFY(QMetaObject::invokeMethod(&panel, "onSendToBack", Qt::DirectConnection));
        QCOMPARE(shape->getLayer(), std::numeric_limits<int>::min());
        QCOMPARE(canvas.undoManager()->stack()->count(), 0);
    }

    void testInvalidOpenAdoptsSourceFileWithoutRiskingPreviousDocument()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString previousPath = directory.filePath(QStringLiteral("previous.html"));
        const QString invalidPath = directory.filePath(QStringLiteral("needs-repair.html"));
        const QString invalidHtml = QStringLiteral(
            "<div id=\"canvas\">\n"
            "  <rect id=\"201\" x=\"0\" y=\"0\" width=\"20\" height=\"20\">\n"
            "</div>\n");

        MainWindow window;
        window.htmlEditorWidget()->setHtmlContent(oneRectangleDocument());
        QVERIFY(window.saveDocumentFile(previousPath));

        QFile invalidFile(invalidPath);
        QVERIFY(invalidFile.open(QIODevice::WriteOnly | QIODevice::Text));
        QCOMPARE(invalidFile.write(invalidHtml.toUtf8()), invalidHtml.toUtf8().size());
        invalidFile.close();

        QVERIFY(!window.loadDocumentFile(invalidPath));
        QCOMPARE(window.htmlEditorWidget()->getHtmlContent(), invalidHtml);
        QVERIFY(window.windowTitle().contains(QStringLiteral("needs-repair.html")));
        QVERIFY(!window.isWindowModified());

        window.htmlEditorWidget()->textEdit()->appendPlainText(QStringLiteral("<!-- repaired later -->"));
        QTRY_VERIFY(window.isWindowModified());
        QAction *save = window.findChild<QAction *>(QStringLiteral("SaveDocumentAction"));
        QVERIFY(save);
        save->trigger();
        QVERIFY(!window.isWindowModified());

        QFile previousFile(previousPath);
        QVERIFY(previousFile.open(QIODevice::ReadOnly | QIODevice::Text));
        QCOMPARE(QString::fromUtf8(previousFile.readAll()), oneRectangleDocument());

        QFile repairedFile(invalidPath);
        QVERIFY(repairedFile.open(QIODevice::ReadOnly | QIODevice::Text));
        QVERIFY(QString::fromUtf8(repairedFile.readAll()).contains(QStringLiteral("repaired later")));
    }

    void testCanvasCommandMarksWindowModifiedImmediately()
    {
        MainWindow window;
        QVERIFY(!window.isWindowModified());

        Canvas *canvas = window.canvasWidget()->canvas();
        auto *shape = new RectShape(QPointF(25.0, 40.0), QSizeF(120.0, 60.0));
        shape->setName(QStringLiteral("Immediate Canvas Change"));
        canvas->undoManager()->push(new CreateShapeCommand(shape, canvas->scene()));

        // 脏状态必须由撤销栈同步更新，不能依赖 25 ms 的序列化定时器。
        QVERIFY(window.isWindowModified());
    }

    void testImmediateSaveFlushesPendingCanvasSynchronization()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString filePath = directory.filePath(QStringLiteral("immediate-save.html"));

        MainWindow window;
        Canvas *canvas = window.canvasWidget()->canvas();
        auto *shape = new RectShape(QPointF(25.0, 40.0), QSizeF(120.0, 60.0));
        shape->setName(QStringLiteral("Immediate Canvas Change"));
        canvas->undoManager()->push(new CreateShapeCommand(shape, canvas->scene()));

        // 不等待事件循环：保存路径自身必须先冲刷尚未执行的画布到编辑器同步。
        QVERIFY(window.saveDocumentFile(filePath));

        QFile savedFile(filePath);
        QVERIFY(savedFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString savedHtml = QString::fromUtf8(savedFile.readAll());
        QVERIFY(savedHtml.contains(QStringLiteral("Immediate Canvas Change")));
        QVERIFY(!window.isWindowModified());
    }

    void testImmediateSaveAfterUndoFlushesRestoredCanvas()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString filePath = directory.filePath(QStringLiteral("undo-save.html"));

        MainWindow window;
        Canvas *canvas = window.canvasWidget()->canvas();
        auto *shape = new RectShape(QPointF(25.0, 40.0), QSizeF(120.0, 60.0));
        shape->setName(QStringLiteral("Undo Before Save"));
        canvas->undoManager()->push(new CreateShapeCommand(shape, canvas->scene()));
        QTRY_VERIFY(window.htmlEditorWidget()->getHtmlContent().contains(
            QStringLiteral("Undo Before Save")));

        canvas->undoManager()->undo();
        // 同样不等待 scene::changed：撤销回到 clean index 后也必须冲刷恢复后的场景。
        QVERIFY(window.saveDocumentFile(filePath));

        QFile savedFile(filePath);
        QVERIFY(savedFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString savedHtml = QString::fromUtf8(savedFile.readAll());
        QVERIFY(!savedHtml.contains(QStringLiteral("Undo Before Save")));
        QCOMPARE(rootShapeCount(canvas->scene()), 0);
        QVERIFY(!window.isWindowModified());
    }

    void testSaveAndLoadHelpersRoundTripDocument()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString filePath = directory.filePath(QStringLiteral("roundtrip.html"));

        MainWindow sourceWindow;
        sourceWindow.htmlEditorWidget()->setHtmlContent(oneRectangleDocument());
        QVERIFY(sourceWindow.saveDocumentFile(filePath));

        QFile savedFile(filePath);
        QVERIFY(savedFile.open(QIODevice::ReadOnly | QIODevice::Text));
        QCOMPARE(QString::fromUtf8(savedFile.readAll()), oneRectangleDocument());

        MainWindow loadedWindow;
        QVERIFY(loadedWindow.loadDocumentFile(filePath));
        QCOMPARE(rootShapeCount(loadedWindow.canvasWidget()->canvas()->scene()), 1);
    }
};

QTEST_MAIN(MainWindowIntegrationTest)
#include "main_window_integration_test.moc"
