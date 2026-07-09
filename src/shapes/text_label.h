#ifndef TEXT_LABEL_H
#define TEXT_LABEL_H

#include "shape.h"

class TextLabel : public Shape
{
public:
    TextLabel();
    TextLabel(qreal x, qreal y, const QString &text);
    TextLabel(QPointF point, const QString &text);

    void setText(const QString &text);
    QString getText() const;

protected:
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

private:
    QSizeF calculateTextSize() const;
};

#endif // TEXT_LABEL_H