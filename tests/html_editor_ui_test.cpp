#include <QtTest>
#include <QApplication>
#include <QTextBlock>
#include <QTextDocument>
#include <QWidget>
#include <QPushButton>
#include "editor/html_editor_widget.h"
#include "editor/html_syntax_highlighter.h"
#include "editor/diagnostics_widget.h"

class HtmlEditorUITest : public QObject
{
    Q_OBJECT

private slots:
    void testHtmlEditorWidget() {
        HtmlEditorWidget widget;
        QSignalSpy executeSpy(&widget, &HtmlEditorWidget::executeRequested);
        QSignalSpy editedSpy(&widget, &HtmlEditorWidget::contentEdited);

        const QString testHtml = "<html>\n<body>\n<!-- Comment -->\n<div id=\"test\"></div>\n</body>\n</html>";
        widget.setHtmlContent(testHtml);
        QCOMPARE(widget.getHtmlContent(), testHtml);
        QCOMPARE(editedSpy.count(), 0);

        // 普通编辑只改变文本，绝不能自动触发代码到画布的同步。
        widget.textEdit()->appendPlainText("Test typing");
        QTRY_COMPARE(editedSpy.count(), 1);
        QCOMPARE(executeSpy.count(), 0);

        auto *executeButton = widget.findChild<QPushButton *>("HtmlExecuteButton");
        QVERIFY(executeButton != nullptr);
        QTest::mouseClick(executeButton, Qt::LeftButton);
        QCOMPARE(executeSpy.count(), 1);
    }

    void testDiagnosticsWidget() {
        DiagnosticsWidget widget;
        
        QSignalSpy spy(&widget, &DiagnosticsWidget::lineDoubleClicked);
        
        DiagnosticMessage msg1 = { DiagnosticMessage::Level::Info, 10, "Test Info" };
        DiagnosticMessage msg2 = { DiagnosticMessage::Level::Error, 25, "Test Error" };
        
        widget.addMessage(msg1);
        widget.addMessage(msg2);

        // Problems 面板应按严重程度排序，确保错误不会被无行号的提示/警告遮住。
        QCOMPARE(widget.treeWidget()->topLevelItemCount(), 2);
        QCOMPARE(widget.treeWidget()->topLevelItem(0)->text(0), QStringLiteral("Error"));
        QCOMPARE(widget.treeWidget()->topLevelItem(0)->text(1), QStringLiteral("25"));

        widget.clearMessages();
        QCOMPARE(widget.treeWidget()->topLevelItemCount(), 0);
    }

    void testSyntaxHighlighterTracksMixedMultilineStates() {
        QTextDocument document;
        HtmlSyntaxHighlighter highlighter(&document);
        document.setPlainText(QStringLiteral(
            "ordinary prefix <!-- closed --> <script>\n"
            "{\"marker\": \"<!-- not an HTML comment -->\"}\n"
            "</script><!-- open comment\n"
            "comment end -->"));
        highlighter.rehighlight();

        QCOMPARE(document.findBlockByNumber(0).userState(), 2);
        QCOMPARE(document.findBlockByNumber(1).userState(), 2);
        QCOMPARE(document.findBlockByNumber(2).userState(), 1);
        QCOMPARE(document.findBlockByNumber(3).userState(), 0);
    }
};

QTEST_MAIN(HtmlEditorUITest)
#include "html_editor_ui_test.moc"
