#ifndef DIAMOND_SHAPE_H
#define DIAMOND_SHAPE_H

#include "shape.h"

class DiamondShape : public Shape
{
public:
    using Shape::Shape;

protected:
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
};

#endif // DIAMOND_SHAPE_H
