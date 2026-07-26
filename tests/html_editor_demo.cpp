#include <QApplication>
#include <QSplitter>
#include "editor/html_editor_widget.h"
#include "editor/diagnostics_widget.h"
#include "editor/html_validator.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QSplitter splitter;
    splitter.setWindowTitle("HTML & Diagnostics UI Demo");
    splitter.setOrientation(Qt::Vertical);
    splitter.resize(800, 600);

    HtmlEditorWidget* editor = new HtmlEditorWidget(&splitter);
    editor->setHtmlContent(
        "<!-- This is a sample HTML visualization project -->\n"
        "<div id=\"canvas\" class=\"viz-container\">\n"
        "    <rect id=\"node1\" x=\"10\" y=\"20\" width=\"100\" height=\"50\"></rect>\n"
        "    <circle id=\"node2\" cx=\"200\" cy=\"45\" r=\"30\"></circle>\n"
        "</div>\n"
        "<script type=\"application/json\" id=\"algorithmviz-data\">\n"
        "{\n"
        "    \"version\": \"1.0\",\n"
        "    \"nodes\": 2\n"
        "}\n"
        "</script>"
    );

    DiagnosticsWidget* diagnostics = new DiagnosticsWidget(&splitter);
    diagnostics->addMessage({DiagnosticMessage::Level::Info, 2, "Canvas container loaded."});
    diagnostics->addMessage({DiagnosticMessage::Level::Warning, 4, "Missing style for circle node2."});
    diagnostics->addMessage({DiagnosticMessage::Level::Error, 8, "Unexpected token in JSON data."});

    // Real validation
    QObject::connect(editor, &HtmlEditorWidget::executeRequested, diagnostics, [editor, diagnostics]() {
        const QString text = editor->getHtmlContent();
        diagnostics->clearMessages();
        QList<DiagnosticMessage> messages = HtmlValidator::validateHtmlCode(text);
        for (const auto& msg : messages) {
            diagnostics->addMessage(msg);
        }
        editor->setDiagnostics(messages);
    });

    // Run initial validation

    splitter.show();

    return app.exec();
}
