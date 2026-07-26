#ifndef HTML_SYNTAX_HIGHLIGHTER_H
#define HTML_SYNTAX_HIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QTextCharFormat>

class HtmlSyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit HtmlSyntaxHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    QRegularExpression commentStartExpression;
    QRegularExpression commentEndExpression;
    QRegularExpression scriptStartExpression;
    QRegularExpression scriptEndExpression;

    QTextCharFormat tagFormat;
    QTextCharFormat attributeFormat;
    QTextCharFormat valueFormat;
    QTextCharFormat commentFormat;
    QTextCharFormat jsKeywordFormat;
    QTextCharFormat jsFunctionFormat;
    QTextCharFormat jsNumberFormat;
};

#endif // HTML_SYNTAX_HIGHLIGHTER_H
