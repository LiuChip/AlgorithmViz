#include "diamond_shape.h"

QRectF DiamondShape::boundingRect() const
{
    qreal halfPen = border.borderWidth / 2.0;
    return QRectF(-halfPen, -halfPen, width + border.borderWidth, height + border.borderWidth);
}

void DiamondShape::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QPolygonF diamond;
    diamond << QPointF(width / 2.0, 0)
            << QPointF(width, height / 2.0)
            << QPointF(width / 2.0, height)
            << QPointF(0, height / 2.0);

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

    if (!textStyle.text.isEmpty())
    {
        painter->setPen(textStyle.textColor);
        QFont font(textStyle.fontFamily, textStyle.fontSize, textStyle.fontWeight);
        painter->setFont(font);
        painter->drawText(QRectF(0, 0, width, height), textStyle.textAligh, textStyle.text);
    }
}
