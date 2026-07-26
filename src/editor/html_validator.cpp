#include "html_validator.h"

#include "core/document/project_metadata.h"

#include <QJsonDocument>
#include <QJsonParseError>
#include <QSet>
#include <QXmlStreamReader>

#include <algorithm>
#include <limits>

namespace {

int saturatedLineNumber(qsizetype line)
{
    return line > std::numeric_limits<int>::max()
        ? std::numeric_limits<int>::max()
        : static_cast<int>(line);
}

} // namespace

QList<DiagnosticMessage> HtmlValidator::validateHtmlCode(const QString &html)
{
    QList<DiagnosticMessage> messages;

    // 与 DocumentParser 共用同一元数据识别规则，避免编辑器预检和实际执行结果不一致。
    const ProjectMetadataBlock metadata = ProjectMetadata::extract(html);
    if (!metadata.found) {
        messages.append({DiagnosticMessage::Level::Warning, -1,
                         QStringLiteral("No project JSON metadata found. Is this a valid project file?")});
    } else if (!metadata.terminated) {
        messages.append({DiagnosticMessage::Level::Error, metadata.tagLine,
                         QStringLiteral("Missing closing </script> tag for metadata.")});
    } else {
        QJsonParseError jsonError;
        const QByteArray jsonBytes = metadata.jsonContent.toUtf8();
        QJsonDocument::fromJson(jsonBytes, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            const qsizetype errorOffset = std::clamp(
                                                     static_cast<qsizetype>(jsonError.offset),
                                                     qsizetype(0), jsonBytes.size());
            const qsizetype localErrorLine = jsonBytes.left(errorOffset).count('\n');
            messages.append({DiagnosticMessage::Level::Error,
                             saturatedLineNumber(
                                 static_cast<qsizetype>(metadata.contentLine)
                                 + localErrorLine),
                             jsonError.errorString()});
        }
    }

    // 未闭合的元数据会吞掉后续内容，此时 XML 验证只会产生误导性重复错误。
    if (metadata.found && !metadata.terminated)
        return messages;

    QXmlStreamReader xml(metadata.xmlContent);
    QSet<QString> seenIds;
    bool foundCanvas = false;

    while (!xml.atEnd() && !xml.hasError()) {
        const QXmlStreamReader::TokenType token = xml.readNext();
        if (token != QXmlStreamReader::StartElement)
            continue;

        const QStringView tagName = xml.name();
        const QXmlStreamAttributes attrs = xml.attributes();

        if (attrs.hasAttribute(QStringLiteral("id"))) {
            const QString id = attrs.value(QStringLiteral("id")).toString();
            if (seenIds.contains(id)) {
                messages.append({DiagnosticMessage::Level::Error,
                                 static_cast<int>(xml.lineNumber()),
                                 QStringLiteral("Duplicate ID found: %1").arg(id)});
            } else {
                seenIds.insert(id);
            }
        }

        if (tagName == QStringLiteral("div") &&
            attrs.value(QStringLiteral("id")) == QStringLiteral("canvas")) {
            foundCanvas = true;
        }

        if (tagName == QStringLiteral("rect")) {
            if (!attrs.hasAttribute(QStringLiteral("x")) ||
                !attrs.hasAttribute(QStringLiteral("y"))) {
                messages.append({DiagnosticMessage::Level::Warning,
                                 static_cast<int>(xml.lineNumber()),
                                 QStringLiteral("<rect> should have 'x' and 'y' attributes.")});
            }
        } else if (tagName == QStringLiteral("circle")) {
            if (!attrs.hasAttribute(QStringLiteral("cx")) ||
                !attrs.hasAttribute(QStringLiteral("cy"))) {
                messages.append({DiagnosticMessage::Level::Warning,
                                 static_cast<int>(xml.lineNumber()),
                                 QStringLiteral("<circle> should have 'cx' and 'cy' attributes.")});
            }
        }
    }

    if (xml.hasError()) {
        messages.append({DiagnosticMessage::Level::Error,
                         static_cast<int>(xml.lineNumber()), xml.errorString()});
    } else if (!foundCanvas) {
        messages.append({DiagnosticMessage::Level::Warning, -1,
                         QStringLiteral("Missing <div id=\"canvas\"> as the main container.")});
    }

    return messages;
}
