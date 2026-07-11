#ifndef DUAL_ARROW_SHAPE_H
#define DUAL_ARROW_SHAPE_H

#include "line_shape.h"

class DualArrowShape : public LineShape
{
public:
    using LineShape::LineShape;

protected:
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

private:
    qreal arrowSize = 10.0;
};

#endif // DUAL_ARROW_SHAPE_H
