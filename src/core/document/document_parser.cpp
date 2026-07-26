#include "document_parser.h"
#include "project_metadata.h"
#include <QXmlStreamReader>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>

#include <algorithm>
#include <limits>

namespace {

int saturatedLineNumber(qsizetype line)
{
    return line > std::numeric_limits<int>::max()
        ? std::numeric_limits<int>::max()
        : static_cast<int>(line);
}

QColor parseColor(const QString& str, const QString& opacityStr = "") {
    if (str == "transparent" || str.isEmpty()) return Qt::transparent;
    
    QColor color;
    if (str.startsWith("rgba")) {
        // rgba(r, g, b, a)
        QRegularExpression re(R"(rgba\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*([\d\.]+)\s*\))");
        QRegularExpressionMatch match = re.match(str);
        if (match.hasMatch()) {
            color.setRgb(match.captured(1).toInt(),
                         match.captured(2).toInt(),
                         match.captured(3).toInt(),
                         static_cast<int>(match.captured(4).toDouble() * 255.0));
        }
    } else {
        QColor parsedColor = QColor::fromString(str);
        if (parsedColor.isValid()) {
            color = parsedColor;
        }
    }
    
    if (!opacityStr.isEmpty()) {
        const double opacity = std::clamp(opacityStr.toDouble(), 0.0, 1.0);
        color.setAlphaF(static_cast<float>(opacity));
    }
    
    return color;
}

void parseCommonAttributes(const QXmlStreamAttributes& attrs, CommonShapeData& common) {
    if (attrs.hasAttribute("id")) common.id = attrs.value("id").toInt();
    if (attrs.hasAttribute("data-name")) common.name = attrs.value("data-name").toString();
    if (attrs.hasAttribute("data-layer")) common.layer = attrs.value("data-layer").toInt();
    if (attrs.hasAttribute("data-locked")) common.locked = attrs.value("data-locked").toString() == "true";
    if (attrs.hasAttribute("data-visible")) common.visible = attrs.value("data-visible").toString() != "false";

    // Border
    if (attrs.hasAttribute("stroke")) {
        common.border.borderColor = parseColor(attrs.value("stroke").toString());
        if (attrs.hasAttribute("stroke-width")) {
            common.border.borderWidth = attrs.value("stroke-width").toDouble();
        }
        common.border.borderStyle = Qt::SolidLine;
        if (attrs.hasAttribute("stroke-dasharray")) {
            QString dash = attrs.value("stroke-dasharray").toString();
            if (dash.contains("5")) common.border.borderStyle = Qt::DashLine;
            else if (dash.contains("2")) common.border.borderStyle = Qt::DotLine;
        }
    } else {
        common.border.borderStyle = Qt::NoPen;
    }

    // Fill
    if (attrs.hasAttribute("fill")) {
        common.fillStyle.fillColor = parseColor(attrs.value("fill").toString(), attrs.value("fill-opacity").toString());
        common.fillStyle.fillOpacity = common.fillStyle.fillColor.alphaF();
    }

    // Text Style
    if (attrs.hasAttribute("color")) common.textStyle.textColor = parseColor(attrs.value("color").toString());
    if (attrs.hasAttribute("font-family")) common.textStyle.font.setFamily(attrs.value("font-family").toString());
    if (attrs.hasAttribute("font-size")) common.textStyle.font.setPointSizeF(attrs.value("font-size").toDouble());
    if (attrs.hasAttribute("font-weight") && attrs.value("font-weight").toString() == "bold") {
        common.textStyle.font.setBold(true);
    }
}

QPointF parsePoint(const QString& str) {
    QStringList parts = str.split(",");
    if (parts.size() == 2) {
        return QPointF(parts[0].toDouble(), parts[1].toDouble());
    }
    return QPointF();
}

} // namespace

LoadResult DocumentParser::parse(const QString& html, DiagramDocument& doc) {
    LoadResult result;
    result.success = true;

    // 事务性对象：当所有解析验证顺利结束后一次性转交给调用方
    DiagramDocument candidate;

    // 1. 分离项目 JSON。提取器兼容旧 ID，并保留脚本占用的换行以保证 XML 行号准确。
    const ProjectMetadataBlock metadata = ProjectMetadata::extract(html);
    if (metadata.found && !metadata.terminated) {
        result.success = false;
        result.diagnostics.append({Diagnostic::Error,
                                   QStringLiteral("Missing closing </script> tag for project metadata."),
                                   metadata.tagLine});
        return result;
    }
    const QString &xmlPart = metadata.xmlContent;
    const QString &jsonPart = metadata.jsonContent;

    // 严苛验证 JSON 快照
    if (!jsonPart.isEmpty()) {
        QJsonParseError jsonError;
        const QByteArray jsonBytes = jsonPart.toUtf8();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonBytes, &jsonError);
        if (jsonError.error != QJsonParseError::NoError) {
            result.success = false;
            const qsizetype errorOffset = std::clamp(
                                                     static_cast<qsizetype>(jsonError.offset),
                                                     qsizetype(0), jsonBytes.size());
            const qsizetype jsonLine = jsonBytes.left(errorOffset).count('\n') + 1;
            const int sourceLine = saturatedLineNumber(
                static_cast<qsizetype>(metadata.contentLine) + jsonLine - 1);
            result.diagnostics.append({Diagnostic::Error,
                                       QStringLiteral("Invalid project JSON metadata: %1").arg(jsonError.errorString()),
                                       sourceLine});
            return result;
        }
        if (!jsonDoc.isObject()) {
            result.success = false;
            result.diagnostics.append({Diagnostic::Error, QStringLiteral("Invalid project JSON metadata: root is not a JSON object")});
            return result;
        }

        QJsonObject root = jsonDoc.object();
        if (!root.contains("version") || root["version"].toString().isEmpty()) {
            result.success = false;
            result.diagnostics.append({Diagnostic::Error, QStringLiteral("Invalid project JSON metadata: missing or invalid version field")});
            return result;
        }

        if (root.contains("canvas") && root["canvas"].isObject()) {
            QJsonObject canvasObj = root["canvas"].toObject();
            candidate.canvasRect = QRectF(
                canvasObj["x"].toDouble(),
                canvasObj["y"].toDouble(),
                canvasObj["width"].toDouble(),
                canvasObj["height"].toDouble()
            );
        }
    } else {
        // 允许纯标签渲染（降级到缺省 canvas_rect，给出警告提示）
        result.diagnostics.append({Diagnostic::Warning, "No project JSON metadata found; using default canvas configurations."});
    }

    // Parse XML
    QXmlStreamReader xml(xmlPart);
    ShapeData* currentShape = nullptr;
    ConnectorData* currentConnector = nullptr;

    while (!xml.atEnd() && !xml.hasError()) {
        QXmlStreamReader::TokenType token = xml.readNext();
        
        if (token == QXmlStreamReader::StartElement) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            QString tagName = xml.name().toString();
#else
            QString tagName = xml.name().toString();
#endif
            QXmlStreamAttributes attrs = xml.attributes();
            
            if (tagName == "div") {
                continue;
            } else if (tagName == "connector") {
                ConnectorData c;
                parseCommonAttributes(attrs, c.common);
                
                if (attrs.hasAttribute("start")) {
                    c.startMode = ConnectorAnchor::Mode::Free;
                    c.startFreePoint = parsePoint(attrs.value("start").toString());
                } else if (attrs.hasAttribute("start-target")) {
                    c.startTargetId = attrs.value("start-target").toInt();
                    if (attrs.hasAttribute("start-fallback")) {
                        c.startFreePoint = parsePoint(attrs.value("start-fallback").toString());
                    }
                    if (attrs.hasAttribute("start-angle")) {
                        c.startMode = ConnectorAnchor::Mode::Boundary;
                        c.startBoundaryAngle = attrs.value("start-angle").toDouble();
                    } else if (attrs.hasAttribute("start-interior")) {
                        c.startMode = ConnectorAnchor::Mode::Interior;
                        c.startInteriorNormalized = parsePoint(attrs.value("start-interior").toString());
                    }
                }
                
                if (attrs.hasAttribute("end")) {
                    c.endMode = ConnectorAnchor::Mode::Free;
                    c.endFreePoint = parsePoint(attrs.value("end").toString());
                } else if (attrs.hasAttribute("end-target")) {
                    c.endTargetId = attrs.value("end-target").toInt();
                    if (attrs.hasAttribute("end-fallback")) {
                        c.endFreePoint = parsePoint(attrs.value("end-fallback").toString());
                    }
                    if (attrs.hasAttribute("end-angle")) {
                        c.endMode = ConnectorAnchor::Mode::Boundary;
                        c.endBoundaryAngle = attrs.value("end-angle").toDouble();
                    } else if (attrs.hasAttribute("end-interior")) {
                        c.endMode = ConnectorAnchor::Mode::Interior;
                        c.endInteriorNormalized = parsePoint(attrs.value("end-interior").toString());
                    }
                }
                
                if (attrs.hasAttribute("end-style")) {
                    QString style = attrs.value("end-style").toString();
                    if (style == "arrow") c.endStyle = Connector::EndStyle::Arrow;
                    else if (style == "dual-arrow") c.endStyle = Connector::EndStyle::DualArrow;
                    else if (style == "none") c.endStyle = Connector::EndStyle::None;
                }
                
                candidate.connectors.append(c);
                currentConnector = &candidate.connectors.last();
            } else {
                ShapeData s;
                parseCommonAttributes(attrs, s.common);
                
                if (tagName == "rect") {
                    s.type = "RectShape";
                    s.position = QPointF(attrs.value("x").toDouble(), attrs.value("y").toDouble());
                    s.size = QSizeF(attrs.value("width").toDouble(), attrs.value("height").toDouble());
                } else if (tagName == "circle") {
                    s.type = "EllipseShape";
                    s.position = QPointF(attrs.value("cx").toDouble(), attrs.value("cy").toDouble());
                    s.size = QSizeF(attrs.value("r").toDouble() * 2.0, attrs.value("r").toDouble() * 2.0);
                } else if (tagName == "ellipse") {
                    s.type = "EllipseShape";
                    s.position = QPointF(attrs.value("cx").toDouble(), attrs.value("cy").toDouble());
                    s.size = QSizeF(attrs.value("rx").toDouble() * 2.0, attrs.value("ry").toDouble() * 2.0);
                } else if (tagName == "diamond") {
                    s.type = "DiamondShape";
                    s.position = QPointF(attrs.value("x").toDouble(), attrs.value("y").toDouble());
                    s.size = QSizeF(attrs.value("width").toDouble(), attrs.value("height").toDouble());
                } else if (tagName == "line" || tagName == "arrow" || tagName == "dual_arrow") {
                    if (tagName == "arrow") s.type = "ArrowShape";
                    else if (tagName == "dual_arrow") s.type = "DualArrowShape";
                    else s.type = "LineShape";
                    
                    s.startPoint = QPointF(attrs.value("x1").toDouble(), attrs.value("y1").toDouble());
                    s.endPoint = QPointF(attrs.value("x2").toDouble(), attrs.value("y2").toDouble());
                } else if (tagName == "text") {
                    s.type = "TextLabel";
                    s.position = QPointF(attrs.value("x").toDouble(), attrs.value("y").toDouble());
                    if (attrs.hasAttribute("width") && attrs.hasAttribute("height")) {
                        s.textLayoutMode = TextLabel::TextLayoutMode::FixedSize;
                        s.size = QSizeF(attrs.value("width").toDouble(), attrs.value("height").toDouble());
                    } else {
                        s.textLayoutMode = TextLabel::TextLayoutMode::AutoSize;
                    }
                } else {
                    continue; // unknown
                }
                
                if (attrs.hasAttribute("transform")) {
                    QString transform = attrs.value("transform").toString();
                    QRegularExpression re(R"(rotate\(([\-\d\.]+)\))");
                    QRegularExpressionMatch match = re.match(transform);
                    if (match.hasMatch()) {
                        s.rotation = match.captured(1).toDouble();
                    }
                }
                
                candidate.shapes.append(s);
                currentShape = &candidate.shapes.last();
            }
        } else if (token == QXmlStreamReader::Characters) {
            QString text = xml.text().toString().trimmed();
            if (!text.isEmpty()) {
                if (currentShape) currentShape->common.textStyle.text = text;
                else if (currentConnector) currentConnector->common.textStyle.text = text;
            }
        } else if (token == QXmlStreamReader::EndElement) {
            currentShape = nullptr;
            currentConnector = nullptr;
        }
    }

    if (xml.hasError()) {
        result.success = false;
        result.diagnostics.append({Diagnostic::Error,
                                   "XML Parse Error: " + xml.errorString(),
                                   static_cast<int>(xml.lineNumber())});
        return result;
    }

    // 事务提交：只有当前毫无差错时才赋给目标 output
    if (result.success) {
        doc = std::move(candidate);
    }
    return result;
}
