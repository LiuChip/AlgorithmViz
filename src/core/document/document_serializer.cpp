#include "document_serializer.h"
#include <QXmlStreamWriter>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QBuffer>

namespace {

QString colorToString(const QColor& color) {
    if (color == Qt::transparent) return "transparent";
    if (color.alpha() == 255) {
        return color.name(QColor::HexRgb);
    }
    return QString("rgba(%1, %2, %3, %4)")
        .arg(color.red()).arg(color.green()).arg(color.blue())
        .arg(color.alphaF(), 0, 'f', 2);
}

void writeCommonAttributes(QXmlStreamWriter& xml, const CommonShapeData& common) {
    xml.writeAttribute("id", QString::number(common.id));
    if (!common.name.isEmpty()) xml.writeAttribute("data-name", common.name);
    if (common.layer != 0) xml.writeAttribute("data-layer", QString::number(common.layer));
    if (common.locked) xml.writeAttribute("data-locked", "true");
    if (!common.visible) xml.writeAttribute("data-visible", "false");

    // Border
    if (common.border.borderStyle != Qt::NoPen && common.border.borderColor != Qt::transparent) {
        xml.writeAttribute("stroke", colorToString(common.border.borderColor));
        xml.writeAttribute("stroke-width", QString::number(common.border.borderWidth));
        if (common.border.borderStyle == Qt::DashLine) {
            xml.writeAttribute("stroke-dasharray", "5,5");
        } else if (common.border.borderStyle == Qt::DotLine) {
            xml.writeAttribute("stroke-dasharray", "2,2");
        }
    }

    // Fill
    if (common.fillStyle.fillColor != Qt::transparent) {
        xml.writeAttribute("fill", colorToString(common.fillStyle.fillColor));
        if (common.fillStyle.fillOpacity < 1.0) {
            xml.writeAttribute("fill-opacity", QString::number(common.fillStyle.fillOpacity, 'f', 2));
        }
    }

    // Text Style
    if (!common.textStyle.text.isEmpty()) {
        xml.writeAttribute("color", colorToString(common.textStyle.textColor));
        xml.writeAttribute("font-family", common.textStyle.font.family());
        xml.writeAttribute("font-size", QString::number(common.textStyle.font.pointSizeF()));
        if (common.textStyle.font.bold()) xml.writeAttribute("font-weight", "bold");
    }
}

} // namespace

QString DocumentSerializer::serialize(const DiagramDocument& doc) {
    QString output;
    QXmlStreamWriter xml(&output);
    xml.setAutoFormatting(true);
    xml.setAutoFormattingIndent(4);

    xml.writeStartElement("div");
    xml.writeAttribute("id", "canvas");
    xml.writeAttribute("class", "viz-container");

    // Serialize Shapes
    for (const ShapeData& s : doc.shapes) {
        QString tag;
        if (s.type == "RectShape") tag = "rect";
        else if (s.type == "EllipseShape") tag = "ellipse";
        else if (s.type == "DiamondShape") tag = "diamond";
        else if (s.type == "LineShape" || s.type == "ArrowShape" || s.type == "DualArrowShape") {
            if (s.type == "ArrowShape") tag = "arrow";
            else if (s.type == "DualArrowShape") tag = "dual_arrow";
            else tag = "line";
        }
        else if (s.type == "TextLabel") tag = "text";
        else continue;

        xml.writeStartElement(tag);
        writeCommonAttributes(xml, s.common);

        if (tag == "rect" || tag == "diamond") {
            xml.writeAttribute("x", QString::number(s.position.x()));
            xml.writeAttribute("y", QString::number(s.position.y()));
            xml.writeAttribute("width", QString::number(s.size.width()));
            xml.writeAttribute("height", QString::number(s.size.height()));
        } else if (tag == "ellipse") {
            xml.writeAttribute("cx", QString::number(s.position.x()));
            xml.writeAttribute("cy", QString::number(s.position.y()));
            xml.writeAttribute("rx", QString::number(s.size.width() / 2.0));
            xml.writeAttribute("ry", QString::number(s.size.height() / 2.0));
        } else if (tag == "line" || tag == "arrow" || tag == "dual_arrow") {
            xml.writeAttribute("x1", QString::number(s.startPoint.x()));
            xml.writeAttribute("y1", QString::number(s.startPoint.y()));
            xml.writeAttribute("x2", QString::number(s.endPoint.x()));
            xml.writeAttribute("y2", QString::number(s.endPoint.y()));
        } else if (tag == "text") {
            xml.writeAttribute("x", QString::number(s.position.x()));
            xml.writeAttribute("y", QString::number(s.position.y()));
            if (s.textLayoutMode == TextLabel::TextLayoutMode::FixedSize) {
                xml.writeAttribute("width", QString::number(s.size.width()));
                xml.writeAttribute("height", QString::number(s.size.height()));
            }
        }
        
        if (s.rotation != 0.0) {
            xml.writeAttribute("transform", QString("rotate(%1)").arg(s.rotation));
        }

        if (!s.common.textStyle.text.isEmpty()) {
            xml.writeCharacters(s.common.textStyle.text);
        }

        xml.writeEndElement();
    }

    // Serialize Connectors
    for (const ConnectorData& c : doc.connectors) {
        xml.writeStartElement("connector");
        writeCommonAttributes(xml, c.common);

        if (c.startMode == ConnectorAnchor::Mode::Free) {
            xml.writeAttribute("start", QString("%1,%2").arg(c.startFreePoint.x()).arg(c.startFreePoint.y()));
        } else {
            xml.writeAttribute("start-target", QString::number(c.startTargetId));
            xml.writeAttribute("start-fallback", QString("%1,%2").arg(c.startFreePoint.x()).arg(c.startFreePoint.y()));
            if (c.startMode == ConnectorAnchor::Mode::Boundary) {
                xml.writeAttribute("start-angle", QString::number(c.startBoundaryAngle));
            } else if (c.startMode == ConnectorAnchor::Mode::Interior) {
                xml.writeAttribute("start-interior", QString("%1,%2").arg(c.startInteriorNormalized.x()).arg(c.startInteriorNormalized.y()));
            }
        }

        if (c.endMode == ConnectorAnchor::Mode::Free) {
            xml.writeAttribute("end", QString("%1,%2").arg(c.endFreePoint.x()).arg(c.endFreePoint.y()));
        } else {
            xml.writeAttribute("end-target", QString::number(c.endTargetId));
            xml.writeAttribute("end-fallback", QString("%1,%2").arg(c.endFreePoint.x()).arg(c.endFreePoint.y()));
            if (c.endMode == ConnectorAnchor::Mode::Boundary) {
                xml.writeAttribute("end-angle", QString::number(c.endBoundaryAngle));
            } else if (c.endMode == ConnectorAnchor::Mode::Interior) {
                xml.writeAttribute("end-interior", QString("%1,%2").arg(c.endInteriorNormalized.x()).arg(c.endInteriorNormalized.y()));
            }
        }

        if (c.endStyle == Connector::EndStyle::Arrow) {
            xml.writeAttribute("end-style", "arrow");
        } else if (c.endStyle == Connector::EndStyle::DualArrow) {
            xml.writeAttribute("end-style", "dual-arrow");
        } else {
            xml.writeAttribute("end-style", "none");
        }

        if (!c.common.textStyle.text.isEmpty()) {
            xml.writeCharacters(c.common.textStyle.text);
        }

        xml.writeEndElement(); // </connector>
    }

    xml.writeEndElement(); // </div>

    // Append JSON metadata block manually, since QXmlStreamWriter escapes it as text
    QJsonObject meta;
    meta["version"] = "1.0";
    QJsonObject canvasObj;
    canvasObj["x"] = doc.canvasRect.x();
    canvasObj["y"] = doc.canvasRect.y();
    canvasObj["width"] = doc.canvasRect.width();
    canvasObj["height"] = doc.canvasRect.height();
    meta["canvas"] = canvasObj;

    QJsonDocument jsonDoc(meta);
    QString jsonStr = jsonDoc.toJson(QJsonDocument::Indented);

    output += "\n<script type=\"application/json\" id=\"algorithmviz-project-data\">\n";
    output += jsonStr;
    output += "</script>\n";

    return output;
}
