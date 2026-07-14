#ifndef ARROW_SHAPE_H
#define ARROW_SHAPE_H

#include "line_shape.h"

class ArrowShape : public LineShape
{
public:
    using LineShape::LineShape;
    Shape *clone() const override;

protected:
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

private:
    qreal arrowSize = 10.0;
};

#endif // ARROW_SHAPE_H
