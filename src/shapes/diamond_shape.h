#ifndef DIAMOND_SHAPE_H
#define DIAMOND_SHAPE_H

#include "shape.h"

class DiamondShape : public Shape
{
public:
    DiamondShape();
    DiamondShape(qreal x, qreal y, qreal width, qreal height);
    DiamondShape(QPointF point, QSizeF size);

protected:
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
};

#endif // DIAMOND_SHAPE_H