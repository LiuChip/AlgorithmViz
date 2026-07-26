#include "html_editor_widget.h"
#include <QVBoxLayout>
#include <QFontDatabase>
#include <QPainter>
#include <QTextBlock>
#include <QCompleter>
#include <QKeyEvent>
#include <QAbstractItemView>
#include <QScrollBar>
#include <QStringListModel>

CodeEditor::CodeEditor(QWidget *parent) : QPlainTextEdit(parent)
{
    lineNumberArea = new LineNumberArea(this);

    connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

CodeEditor::~CodeEditor() = default;

void CodeEditor::setCompleter(QCompleter *completer)
{
    if (c)
        QObject::disconnect(c, 0, this, 0);

    c = completer;

    if (!c)
        return;

    c->setWidget(this);
    c->setCompletionMode(QCompleter::PopupCompletion);
    c->setCaseSensitivity(Qt::CaseInsensitive);
    QObject::connect(c, QOverload<const QString &>::of(&QCompleter::activated),
                     this, &CodeEditor::insertCompletion);
}

QCompleter *CodeEditor::completer() const
{
    return c;
}

void CodeEditor::insertCompletion(const QString &completion)
{
    if (c->widget() != this)
        return;
    QTextCursor tc = textCursor();
    const qsizetype extra = completion.length() - c->completionPrefix().length();
    if (extra > 0)
        tc.insertText(completion.right(extra));
}

QString CodeEditor::textUnderCursor() const
{
    QTextCursor tc = textCursor();
    tc.select(QTextCursor::WordUnderCursor);
    return tc.selectedText();
}

void CodeEditor::focusInEvent(QFocusEvent *e)
{
    if (c)
        c->setWidget(this);
    QPlainTextEdit::focusInEvent(e);
}

void CodeEditor::keyPressEvent(QKeyEvent *e)
{
    if (c && c->popup()->isVisible()) {
        switch (e->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_Escape:
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            e->ignore();
            return;
        default:
            break;
        }
    }

    bool isShortcut = ((e->modifiers() & Qt::ControlModifier) && e->key() == Qt::Key_Space); 
    if (!c || !isShortcut)
        QPlainTextEdit::keyPressEvent(e);

    const bool ctrlOrShift = e->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier);
    if (!c || (ctrlOrShift && e->text().isEmpty()))
        return;

    static QString eow("~!@#$%^&*()_+{}|:\"<>?,./;'[]\\-="); // end of word
    bool hasModifier = (e->modifiers() != Qt::NoModifier) && !ctrlOrShift;
    QString completionPrefix = textUnderCursor();

    if (!isShortcut && (hasModifier || e->text().isEmpty() || completionPrefix.length() < 1
                      || eow.contains(e->text().right(1)))) {
        c->popup()->hide();
        return;
    }

    if (completionPrefix != c->completionPrefix()) {
        c->setCompletionPrefix(completionPrefix);
        c->popup()->setCurrentIndex(c->completionModel()->index(0, 0));
    }
    QRect cr = cursorRect();
    cr.setWidth(c->popup()->sizeHintForColumn(0)
                + c->popup()->verticalScrollBar()->sizeHint().width());
    c->complete(cr); 
}

int CodeEditor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int glyphMargin = fontMetrics().height();
    int space = glyphMargin + 8 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void CodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0) {
            zoomIn(1);
        } else {
            zoomOut(1);
        }
        event->accept();
    } else {
        QPlainTextEdit::wheelEvent(event);
    }
}

void CodeEditor::setDiagnostics(const QList<DiagnosticMessage>& messages)
{
    m_diagnostics = messages;
    highlightCurrentLine(); // Re-highlight lines based on new diagnostics
    lineNumberArea->update(); // Re-paint line numbers for icons
}

void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    // 1. Current line highlight
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        QColor lineColor = QColor("#f8f8f8"); // VS Code style extremely faint gray
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    // 2. Error and Warning Highlights
    for (const auto& msg : m_diagnostics) {
        if (msg.lineNumber > 0) {
            QTextEdit::ExtraSelection selection;
            QColor bgColor;
            if (msg.level == DiagnosticMessage::Level::Error) {
                bgColor = QColor("#ffe6e6"); // VS Code style light red
            } else if (msg.level == DiagnosticMessage::Level::Warning) {
                bgColor = QColor("#fff2e5"); // VS Code style light orange
            } else {
                continue;
            }
            selection.format.setBackground(bgColor);
            selection.format.setProperty(QTextFormat::FullWidthSelection, true);
            
            // Move cursor to the specific line
            QTextCursor cursor(document());
            cursor.movePosition(QTextCursor::Start);
            cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, msg.lineNumber - 1);
            selection.cursor = cursor;
            selection.cursor.clearSelection();
            
            extraSelections.append(selection);
        }
    }

    setExtraSelections(extraSelections);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), QColor("#f3f3f3")); // VS Code style light gray background

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            
            // Draw Icon if there's an error on this line
            bool hasError = false;
            bool hasWarning = false;
            for (const auto& msg : m_diagnostics) {
                if (msg.lineNumber == blockNumber + 1) {
                    if (msg.level == DiagnosticMessage::Level::Error) hasError = true;
                    if (msg.level == DiagnosticMessage::Level::Warning) hasWarning = true;
                }
            }
            
            if (hasError || hasWarning) {
                painter.save();
                painter.setRenderHint(QPainter::Antialiasing);
                int iconSize = fontMetrics().height() - 4;
                QRect iconRect(2, top + 2, iconSize, iconSize);
                
                if (hasError) {
                    // Draw red cross circle
                    painter.setBrush(Qt::red);
                    painter.setPen(Qt::NoPen);
                    painter.drawEllipse(iconRect);
                    painter.setPen(QPen(Qt::white, 2));
                    painter.drawLine(iconRect.center().x() - 2, iconRect.center().y() - 2, iconRect.center().x() + 2, iconRect.center().y() + 2);
                    painter.drawLine(iconRect.center().x() - 2, iconRect.center().y() + 2, iconRect.center().x() + 2, iconRect.center().y() - 2);
                } else {
                    // Draw yellow warning triangle
                    painter.setBrush(QColor("#cca700")); // dark yellow
                    painter.setPen(Qt::NoPen);
                    QPolygon triangle;
                    triangle << QPoint(iconRect.center().x(), iconRect.top())
                             << QPoint(iconRect.left(), iconRect.bottom())
                             << QPoint(iconRect.right(), iconRect.bottom());
                    painter.drawPolygon(triangle);
                    painter.setPen(Qt::white);
                    painter.drawText(iconRect, Qt::AlignCenter, "!");
                }
                painter.restore();
            }

            painter.setPen(QColor("#2b91af")); // VS Code style teal/gray text
            painter.drawText(0, top, lineNumberArea->width() - 4, fontMetrics().height(),
                             Qt::AlignRight | Qt::AlignVCenter, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

// ------------------------------------------------------------- //

HtmlEditorWidget::HtmlEditorWidget(QWidget *parent)
    : QWidget(parent)
    , m_isInternalChange(false)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_textEdit = new CodeEditor(this);

    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(12);
    m_textEdit->setFont(font);
    m_textEdit->setLineWrapMode(QPlainTextEdit::NoWrap);

    const int tabStop = 4;
    m_textEdit->setTabStopDistance(
        QFontMetrics(m_textEdit->font()).horizontalAdvance(' ') * tabStop);

    m_highlighter = new HtmlSyntaxHighlighter(m_textEdit->document());

    QStringList words;
    words << "html" << "head" << "body" << "div" << "script" << "style"
          << "rect" << "circle" << "ellipse" << "line" << "diamond" << "arrow" << "dual_arrow" << "text"
          << "connector" << "anchor"
          << "class" << "id" << "x" << "y" << "width" << "height" << "cx" << "cy" << "r" << "fill" << "stroke"
          << "function" << "var" << "let" << "const" << "if" << "else" << "return" << "true" << "false"
          << "console" << "log" << "alert";

    QCompleter *completer = new QCompleter(new QStringListModel(words, this), this);
    completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setWrapAround(false);
    m_textEdit->setCompleter(completer);

    layout->addWidget(m_textEdit);

    // 代码到画布只能由用户明确执行。按钮覆盖在编辑器右下角，不占用正文布局空间。
    m_executeButton = new QPushButton(QStringLiteral("▶"), this);
    m_executeButton->setObjectName(QStringLiteral("HtmlExecuteButton"));
    m_executeButton->setToolTip(QStringLiteral("执行代码并更新画布"));
    m_executeButton->setAccessibleName(QStringLiteral("执行代码"));
    m_executeButton->setFixedSize(42, 42);
    m_executeButton->setCursor(Qt::PointingHandCursor);
    m_executeButton->setStyleSheet(
        "QPushButton { background: #409eff; color: white; border: none; border-radius: 21px; font-size: 17px; padding-left: 2px; }"
        "QPushButton:hover { background: #66b1ff; }"
        "QPushButton:pressed { background: #337ecc; }");
    connect(m_executeButton, &QPushButton::clicked,
            this, &HtmlEditorWidget::executeRequested);
    connect(m_textEdit, &QPlainTextEdit::textChanged, this, [this]() {
        if (!m_isInternalChange)
            emit contentEdited();
    });
    positionExecuteButton();
}

HtmlEditorWidget::~HtmlEditorWidget() = default;

void HtmlEditorWidget::setHtmlContent(const QString &html)
{
    if (m_textEdit->toPlainText() == html)
        return;

    m_isInternalChange = true;
    m_textEdit->setPlainText(html);
    m_isInternalChange = false;
}

QString HtmlEditorWidget::getHtmlContent() const
{
    return m_textEdit->toPlainText();
}

void HtmlEditorWidget::setWordWrap(bool enable)
{
    m_textEdit->setLineWrapMode(enable ? QPlainTextEdit::WidgetWidth
                                       : QPlainTextEdit::NoWrap);
}

void HtmlEditorWidget::setDiagnostics(const QList<DiagnosticMessage>& messages)
{
    m_textEdit->setDiagnostics(messages);
}

void HtmlEditorWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    positionExecuteButton();
}

void HtmlEditorWidget::positionExecuteButton()
{
    if (!m_executeButton)
        return;

    constexpr int margin = 16;
    m_executeButton->move(qMax(0, width() - m_executeButton->width() - margin),
                          qMax(0, height() - m_executeButton->height() - margin));
    m_executeButton->raise();
}
