#include "text_label.h"

#include <QFontMetricsF>

TextLabel::TextLabel(qreal x, qreal y, const QString &text)
    : Shape(x, y, 0.0, 0.0)
{
    setText(text);
}

TextLabel::TextLabel(QPointF point, const QString &text)
    : TextLabel(point.x(), point.y(), text)
{
}

void TextLabel::refreshTextGeometry()
{
    prepareGeometryChange();
    const QSizeF size = calculateTextSize();
    width = size.width();
    height = size.height();
    update();
}

void TextLabel::setText(const QString &text)
{
    textStyle.text = text;
    refreshTextGeometry();
}

void TextLabel::setTextInfo(TextStyle textStyle)
{
    Shape::setTextInfo(textStyle);
    refreshTextGeometry();
}

void TextLabel::setTextInfo()
{
    Shape::setTextInfo();
    refreshTextGeometry();
}

QString TextLabel::getText() const
{
    return textStyle.text;
}

QSizeF TextLabel::calculateTextSize() const
{
    QFont font(textStyle.fontFamily, textStyle.fontSize, textStyle.fontWeight);
    QFontMetricsF metrics(font);
    const qreal textWidth = metrics.horizontalAdvance(textStyle.text);
    const qreal textHeight = metrics.height();
    return QSizeF(textWidth + 10, textHeight + 6);
}

QRectF TextLabel::boundingRect() const
{
    qreal halfPen = border.borderWidth / 2.0;
    return QRectF(-halfPen, -halfPen, width + border.borderWidth, height + border.borderWidth);
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

Shape *TextLabel::clone() const
{
    auto *cloned = new TextLabel(x(), y(), textStyle.text);
    this->copyPropertiesTo(cloned);
    return cloned;
}
