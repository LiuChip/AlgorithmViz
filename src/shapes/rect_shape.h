#ifndef RECT_SHAPE_H
#define RECT_SHAPE_H

#include "shape.h"

class RectShape : public Shape
{
public:
    RectShape();
    RectShape(qreal x, qreal y, qreal width, qreal height);
    RectShape(QPointF point, QSizeF size);

protected:
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
};

#endif // RECT_SHAPE_H