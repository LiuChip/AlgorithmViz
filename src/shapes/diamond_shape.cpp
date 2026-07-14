#include "diamond_shape.h"

#include <cmath>

QRectF DiamondShape::boundingRect() const
{
    qreal halfPen = border.borderWidth / 2.0;
    return QRectF(-halfPen, -halfPen, width + border.borderWidth, height + border.borderWidth);
}

QPainterPath DiamondShape::localGeometryPath() const
{
    QPainterPath path;
    QPolygonF diamond;
    diamond << QPointF(width / 2.0, 0)
            << QPointF(width, height / 2.0)
            << QPointF(width / 2.0, height)
            << QPointF(0, height / 2.0);
    path.addPolygon(diamond);
    path.closeSubpath();
    return path;
}

QPointF DiamondShape::boundaryPointAtAngle(qreal angleRadians) const
{
    const QPointF center(width / 2.0, height / 2.0);
    const qreal radiusX = width / 2.0;
    const qreal radiusY = height / 2.0;
    const qreal cosine = std::cos(angleRadians);
    const qreal sine = std::sin(angleRadians);
    const qreal denominator =
        (radiusX > 0.0 ? std::abs(cosine) / radiusX : 0.0) +
        (radiusY > 0.0 ? std::abs(sine) / radiusY : 0.0);

    if (denominator <= 0.0) {
        return center;
    }

    const qreal distance = 1.0 / denominator;
    return center + QPointF(cosine * distance, sine * distance);
}

void DiamondShape::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QPolygonF diamond;
    diamond << QPointF(width / 2.0, 0)
            << QPointF(width, height / 2.0)
            << QPointF(width / 2.0, height)
            << QPointF(0, height / 2.0);

    applyFillStyle(painter);
    applyBorderStyle(painter);

    painter->drawPolygon(diamond);
    drawText(painter, QRectF(0, 0, width, height));
}

Shape *DiamondShape::clone() const
{
    auto *cloned = new DiamondShape(x(), y(), width, height);
    this->copyConnectablePropertiesTo(*cloned);
    return cloned;
}
