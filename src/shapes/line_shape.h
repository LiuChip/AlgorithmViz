#ifndef LINE_SHAPE_H
#define LINE_SHAPE_H

#include "shape.h"

class LineShape : public Shape
{
protected:
    QPointF startPoint;
    QPointF endPoint;

    void syncGeometry(bool notify = true);

public:
    LineShape(QPointF start, QPointF end);

    QPointF getStartPoint() const;
    void setStartPoint(QPointF point);
    void setStartPoint(qreal x, qreal y);

    QPointF getEndPoint() const;
    void setEndPoint(QPointF point);
    void setEndPoint(qreal x, qreal y);

protected:
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
};

#endif // LINE_SHAPE_H
