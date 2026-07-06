#include "shape_base.h"

ShapeBase::ShapeBase(QGraphicsItem *parent)
    : QGraphicsItem(parent)
{
    setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
}

void ShapeBase::setFillColor(const QColor &color) { m_fillColor = color; update(); }
void ShapeBase::setStrokeColor(const QColor &color) { m_strokeColor = color; update(); }
void ShapeBase::setStrokeWidth(qreal width) { m_strokeWidth = width; update(); }
