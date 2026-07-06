#ifndef SHAPE_BASE_H
#define SHAPE_BASE_H

#include <QGraphicsItem>
#include <QPainter>

class ShapeBase : public QGraphicsItem
{
public:
    explicit ShapeBase(QGraphicsItem *parent = nullptr);
    virtual QString toSvg() const = 0;

    void setFillColor(const QColor &color);
    void setStrokeColor(const QColor &color);
    void setStrokeWidth(qreal width);

protected:
    QColor m_fillColor{Qt::white};
    QColor m_strokeColor{Qt::black};
    qreal m_strokeWidth{2.0};
};

#endif // SHAPE_BASE_H
