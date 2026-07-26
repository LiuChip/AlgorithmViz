#include "project_metadata.h"

#include <QRegularExpression>

#include <limits>

namespace {

const QRegularExpression &metadataStartExpression()
{
    static const QRegularExpression expression(
        QStringLiteral(
            R"(<script\b(?=[^>]*\btype\s*=\s*(?:"application/json"|'application/json'))(?=[^>]*\bid\s*=\s*(?:"algorithmviz-(?:project-)?data"|'algorithmviz-(?:project-)?data'))[^>]*>)"),
        QRegularExpression::CaseInsensitiveOption);
    return expression;
}

const QRegularExpression &scriptEndExpression()
{
    static const QRegularExpression expression(
        QStringLiteral(R"(</script\s*>)"),
        QRegularExpression::CaseInsensitiveOption);
    return expression;
}

QString blankPreservingLines(QString text)
{
    for (QChar &character : text) {
        if (character != QLatin1Char('\n') && character != QLatin1Char('\r'))
            character = QLatin1Char(' ');
    }
    return text;
}

int lineNumberAt(const QString &text, qsizetype position)
{
    const qsizetype line = text.left(position).count(QLatin1Char('\n')) + 1;
    return line > std::numeric_limits<int>::max()
        ? std::numeric_limits<int>::max()
        : static_cast<int>(line);
}

} // namespace

ProjectMetadataBlock ProjectMetadata::extract(const QString &html)
{
    ProjectMetadataBlock result;
    result.xmlContent = html;

    const QRegularExpressionMatch startMatch = metadataStartExpression().match(html);
    if (!startMatch.hasMatch())
        return result;

    result.found = true;
    result.tagStart = startMatch.capturedStart();
    result.contentStart = startMatch.capturedEnd();
    result.tagLine = lineNumberAt(html, result.tagStart);
    result.contentLine = lineNumberAt(html, result.contentStart);

    const QRegularExpressionMatch endMatch = scriptEndExpression().match(html, result.contentStart);
    if (!endMatch.hasMatch()) {
        result.terminated = false;
        return result;
    }

    result.blockEnd = endMatch.capturedEnd();
    result.jsonContent = html.mid(result.contentStart,
                                  endMatch.capturedStart() - result.contentStart);

    const qsizetype blockLength = result.blockEnd - result.tagStart;
    result.xmlContent.replace(result.tagStart, blockLength,
                              blankPreservingLines(html.mid(result.tagStart, blockLength)));
    return result;
}
