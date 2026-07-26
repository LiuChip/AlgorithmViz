#ifndef HTML_EDITOR_WIDGET_H
#define HTML_EDITOR_WIDGET_H

#include <QWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QResizeEvent>
#include <QList>
#include "html_syntax_highlighter.h"
#include "diagnostics_widget.h"

#include <QCompleter>

class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit CodeEditor(QWidget *parent = nullptr);
    ~CodeEditor() override;

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();
    
    void setDiagnostics(const QList<DiagnosticMessage>& messages);

    void setCompleter(QCompleter *c);
    QCompleter *completer() const;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *e) override;
    void focusInEvent(QFocusEvent *e) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);
    void insertCompletion(const QString &completion);

private:
    QString textUnderCursor() const;
    QWidget *lineNumberArea;
    QList<DiagnosticMessage> m_diagnostics;
    QCompleter *c = nullptr;
};

class LineNumberArea : public QWidget
{
public:
    explicit LineNumberArea(CodeEditor *editor) : QWidget(editor), codeEditor(editor)
    {}

    QSize sizeHint() const override
    {
        return QSize(codeEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        codeEditor->lineNumberAreaPaintEvent(event);
    }

private:
    CodeEditor *codeEditor;
};

class HtmlEditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit HtmlEditorWidget(QWidget *parent = nullptr);
    ~HtmlEditorWidget() override;

    void setHtmlContent(const QString &html);
    QString getHtmlContent() const;

    CodeEditor* textEdit() const { return m_textEdit; }
    
    // Future settings toggles
    void setWordWrap(bool enable);
    void setDiagnostics(const QList<DiagnosticMessage>& messages);

signals:
    // 用户明确点击执行按钮时发出；普通文本编辑不会自动修改画布。
    void executeRequested();

    // 仅在用户直接编辑代码时发出；setHtmlContent() 的程序化同步不会触发。
    void contentEdited();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void positionExecuteButton();

    CodeEditor *m_textEdit;
    HtmlSyntaxHighlighter *m_highlighter;
    QPushButton *m_executeButton = nullptr;
    bool m_isInternalChange;
};

#endif // HTML_EDITOR_WIDGET_H
