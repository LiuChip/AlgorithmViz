#include "html_syntax_highlighter.h"

HtmlSyntaxHighlighter::HtmlSyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    // Element tag names (VS Code dark red)
    tagFormat.setForeground(QColor("#800000"));

    // Attributes (VS Code bright red)
    attributeFormat.setForeground(QColor("#ff0000"));

    // Values in quotes (VS Code blue)
    valueFormat.setForeground(QColor("#0000ff"));

    // Comments (VS Code green)
    commentFormat.setForeground(QColor("#008000"));

    // JS Keywords (VS Code purple)
    jsKeywordFormat.setForeground(QColor("#af00db"));

    // JS Functions (VS Code brown/yellowish)
    jsFunctionFormat.setForeground(QColor("#795e26"));

    // JS Numbers
    jsNumberFormat.setForeground(QColor("#098658"));

    // Element Tags like <div ...> or </div>
    rule.pattern = QRegularExpression(QStringLiteral("<[/]?[A-Za-z0-9_\\-]+(?=[\\s>])"));
    rule.format = tagFormat;
    highlightingRules.append(rule);

    // Attributes like id= or class=
    rule.pattern = QRegularExpression(QStringLiteral("\\b[A-Za-z0-9_\\-]+(?=\\=)"));
    rule.format = attributeFormat;
    highlightingRules.append(rule);

    // JS Keywords
    QStringList keywordPatterns = {
        "\\bfunction\\b", "\\bvar\\b", "\\blet\\b", "\\bconst\\b",
        "\\bif\\b", "\\belse\\b", "\\bfor\\b", "\\bwhile\\b", "\\breturn\\b",
        "\\btrue\\b", "\\bfalse\\b", "\\bnull\\b", "\\bnew\\b", "\\bthis\\b"
    };
    for (const QString &pattern : keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = jsKeywordFormat;
        highlightingRules.append(rule);
    }

    // JS Functions (word followed by "(")
    rule.pattern = QRegularExpression(QStringLiteral("\\b[A-Za-z0-9_]+(?=\\s*\\()"));
    rule.format = jsFunctionFormat;
    highlightingRules.append(rule);

    // JS Numbers
    rule.pattern = QRegularExpression(QStringLiteral("\\b[0-9]+(?:\\.[0-9]+)?\\b"));
    rule.format = jsNumberFormat;
    highlightingRules.append(rule);

    // Quoted attribute values and strings (HTML and JS strings)
    rule.pattern = QRegularExpression(QStringLiteral("[\"'][^\"']*[\"']"));
    rule.format = valueFormat;
    highlightingRules.append(rule);

    commentStartExpression = QRegularExpression(QStringLiteral("<!--"));
    commentEndExpression = QRegularExpression(QStringLiteral("-->"));
    
    scriptStartExpression = QRegularExpression(QStringLiteral("<script\\b[^>]*>"));
    scriptEndExpression = QRegularExpression(QStringLiteral("</script>"));
}

void HtmlSyntaxHighlighter::highlightBlock(const QString &text)
{
    // 先应用普通的单行规则。QTextDocument 的单个文本块使用 int 位置，
    // 因此这里在调用 QSyntaxHighlighter API 时显式转换 Qt 6 的 qsizetype。
    for (const auto &rule : highlightingRules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            const QRegularExpressionMatch match = matchIterator.next();
            setFormat(static_cast<int>(match.capturedStart()),
                      static_cast<int>(match.capturedLength()), rule.format);
        }
    }

    // 状态：0 为普通 HTML，1 为 HTML 注释，2 为 script 内容。
    // 使用单一游标依次处理同一行中的多个状态切换，避免结束一个区段后
    // 覆盖搜索起点，从而漏掉后续的注释或 script 标签。
    int state = previousBlockState();
    if (state != 1 && state != 2)
        state = 0;

    qsizetype cursor = 0;
    while (cursor < text.size()) {
        if (state == 1) {
            const QRegularExpressionMatch endMatch = commentEndExpression.match(text, cursor);
            const qsizetype end = endMatch.hasMatch()
                ? endMatch.capturedEnd()
                : text.size();
            setFormat(static_cast<int>(cursor), static_cast<int>(end - cursor),
                      commentFormat);
            if (!endMatch.hasMatch()) {
                setCurrentBlockState(1);
                return;
            }
            cursor = end;
            state = 0;
            continue;
        }

        if (state == 2) {
            const QRegularExpressionMatch endMatch = scriptEndExpression.match(text, cursor);
            if (!endMatch.hasMatch()) {
                setCurrentBlockState(2);
                return;
            }
            cursor = endMatch.capturedEnd();
            state = 0;
            continue;
        }

        const QRegularExpressionMatch commentMatch = commentStartExpression.match(text, cursor);
        const QRegularExpressionMatch scriptMatch = scriptStartExpression.match(text, cursor);
        const qsizetype commentStart = commentMatch.hasMatch()
            ? commentMatch.capturedStart()
            : -1;
        const qsizetype scriptStart = scriptMatch.hasMatch()
            ? scriptMatch.capturedStart()
            : -1;

        if (commentStart < 0 && scriptStart < 0)
            break;

        if (commentStart >= 0 && (scriptStart < 0 || commentStart < scriptStart)) {
            cursor = commentStart;
            state = 1;
        } else {
            // 起始标签已经由普通 HTML 规则着色；script 状态从其内容开始。
            cursor = scriptMatch.capturedEnd();
            state = 2;
        }
    }

    setCurrentBlockState(state);
}
