#include "line_shape.h"

LineShape::LineShape(QPointF start, QPointF end)
    : startPoint(start), endPoint(end)
{
    QRectF rect(start, end);
    rect = rect.normalized();  // 确保宽高为正
    x = rect.x();
    y = rect.y();
    width = rect.width();
    height = rect.height();
}

QPointF LineShape::getStartPoint()const{return startPoint;}
void LineShape::setStartPoint(QPointF point){startPoint=point;}
void LineShape::setStartPoint(qreal x,qreal y){startPoint=QPointF(x,y);}

QPointF LineShape::getEndPoint()const{return endPoint;}
void LineShape::setEndPoint(QPointF point){endPoint=point;}
void LineShape::setEndPoint(qreal x,qreal y){endPoint=QPointF(x,y);}


QRectF LineShape::boundingRect() const
{
    qreal halfPen = border.borderWidth / 2.0;
    return QRectF(-halfPen, -halfPen, width + border.borderWidth, height + border.borderWidth);
}

// paint — 画线，不用填充
void LineShape::paint(QPainter *painter,  const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // 只有边框，没有填充
    if (border.borderWidth > 0.0 && border.borderStyle != Qt::NoPen)
    {
        QPen pen(border.borderColor, border.borderWidth, border.borderStyle);
        painter->setPen(pen);
    }
    else
    {
        painter->setPen(Qt::NoPen);
    }

    painter->drawLine(startPoint, endPoint);
}