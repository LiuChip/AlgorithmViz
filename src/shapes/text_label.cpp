#include "text_label.h"
#include <QFontMetricsF>

TextLabel::TextLabel() : Shape() {}

TextLabel::TextLabel(qreal x, qreal y, const QString &text)
    : Shape()
{
    this->x = x;
    this->y = y;
    this->textStyle.text = text;
    QSizeF size = calculateTextSize();
    this->width = size.width();
    this->height = size.height();
}

TextLabel::TextLabel(QPointF point, const QString &text)
    : TextLabel(point.x(), point.y(), text) {}

void TextLabel::setText(const QString &text)
{
    this->textStyle.text = text;
    QSizeF size = calculateTextSize();
    this->width = size.width();
    this->height = size.height();
    update();
}

QString TextLabel::getText() const
{
    return textStyle.text;
}

QSizeF TextLabel::calculateTextSize() const
{
    QFont font(textStyle.fontFamily, textStyle.fontSize, textStyle.fontWeight);
    QFontMetricsF metrics(font);
    qreal textWidth = metrics.horizontalAdvance(textStyle.text);
    qreal textHeight = metrics.height();
    return QSizeF(textWidth + 10, textHeight + 6);
}

QRectF TextLabel::boundingRect() const
{
    return QRectF(0, 0, width, height);
}

void TextLabel::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setPen(textStyle.textColor);
    QFont font(textStyle.fontFamily, textStyle.fontSize, textStyle.fontWeight);
    painter->setFont(font);

    QRectF rect(0, 0, width, height);
    painter->drawText(rect, textStyle.textAligh, textStyle.text);
}