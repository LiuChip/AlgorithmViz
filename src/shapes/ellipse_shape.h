#ifndef ELLIPSE_SHAPE_H
#define ELLIPSE_SHAPE_H

#include "shape.h"

class EllipseShape : public Shape
{
public:
    EllipseShape();
    EllipseShape(qreal x, qreal y, qreal width, qreal height);
    EllipseShape(QPointF point, QSizeF size);

protected:
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
};

#endif // ELLIPSE_SHAPE_H