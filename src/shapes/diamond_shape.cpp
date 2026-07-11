#include "diamond_shape.h"

DiamondShape::DiamondShape() : Shape() {}
DiamondShape::DiamondShape(qreal x, qreal y, qreal width, qreal height)
    : Shape(x, y, width, height) {}
DiamondShape::DiamondShape(QPointF point, QSizeF size)
    : Shape(point, size) {}

QRectF DiamondShape::boundingRect() const
{
    qreal halfPen = border.borderWidth / 2.0;
    return QRectF(-halfPen, -halfPen, width + border.borderWidth, height + border.borderWidth);
}

void DiamondShape::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // Build diamond polygon: top, right, bottom, left
    QPolygonF diamond;
    diamond << QPointF(width / 2.0, 0)          // top center
            << QPointF(width, height / 2.0)      // right center
            << QPointF(width / 2.0, height)      // bottom center
            << QPointF(0, height / 2.0);         // left center

    // Fill
    if (fillStyle.fillColor != Qt::transparent)
    {
        QColor fill = fillStyle.fillColor;
        fill.setAlphaF(fillStyle.fillOpacity);
        painter->setBrush(fill);
    }
    else
    {
        painter->setBrush(Qt::NoBrush);
    }

    // Border
    if (border.borderWidth > 0.0 && border.borderStyle != Qt::NoPen)
    {
        QPen pen(border.borderColor, border.borderWidth, border.borderStyle);
        painter->setPen(pen);
    }
    else
    {
        painter->setPen(Qt::NoPen);
    }

    painter->drawPolygon(diamond);

    // Text
    if (!textStyle.text.isEmpty())
    {
        painter->setPen(textStyle.textColor);
        QFont font(textStyle.fontFamily, textStyle.fontSize, textStyle.fontWeight);
        painter->setFont(font);
        painter->drawText(QRectF(0, 0, width, height), textStyle.textAligh, textStyle.text);
    }
}