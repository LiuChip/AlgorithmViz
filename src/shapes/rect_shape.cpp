#include "rect_shape.h"

RectShape::RectShape() : Shape() {}
RectShape::RectShape(qreal x, qreal y, qreal width, qreal height)
    : Shape(x, y, width, height) {}
RectShape::RectShape(QPointF point, QSizeF size)
    : Shape(point, size) {}

QRectF RectShape::boundingRect() const
{
    qreal halfPen = border.borderWidth / 2.0;
    return QRectF(-halfPen, -halfPen, width + border.borderWidth, height + border.borderWidth);
}

void RectShape::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QRectF rect(0, 0, width, height);

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

    painter->drawRect(rect);

    // Text
    if (!textStyle.text.isEmpty())
    {
        painter->setPen(textStyle.textColor);
        QFont font(textStyle.fontFamily, textStyle.fontSize, textStyle.fontWeight);
        painter->setFont(font);
        painter->drawText(rect, textStyle.textAligh, textStyle.text);
    }
}