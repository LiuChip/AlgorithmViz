#ifndef RECT_SHAPE_H
#define RECT_SHAPE_H

#include "shape.h"

class RectShape : public Shape
{
public:
    using Shape::Shape;

protected:
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
};

#endif // RECT_SHAPE_H
