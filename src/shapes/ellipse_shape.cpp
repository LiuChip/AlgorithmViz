#include "ellipse_shape.h"

#include <cmath>

QRectF EllipseShape::boundingRect() const
{
    qreal halfPen = border.borderWidth / 2.0;
    return QRectF(-halfPen, -halfPen, width + border.borderWidth, height + border.borderWidth);
}

QPainterPath EllipseShape::localGeometryPath() const
{
    QPainterPath path;
    path.addEllipse(QRectF(0, 0, width, height));
    return path;
}

QPointF EllipseShape::boundaryPointAtAngle(qreal angleRadians) const
{
    const QPointF center(width / 2.0, height / 2.0);
    const qreal radiusX = width / 2.0;
    const qreal radiusY = height / 2.0;
    const qreal cosine = std::cos(angleRadians);
    const qreal sine = std::sin(angleRadians);
    const qreal denominator =
        (radiusX > 0.0 ? (cosine * cosine) / (radiusX * radiusX) : 0.0) +
        (radiusY > 0.0 ? (sine * sine) / (radiusY * radiusY) : 0.0);

    if (denominator <= 0.0) {
        return center;
    }

    const qreal distance = 1.0 / std::sqrt(denominator);
    return center + QPointF(cosine * distance, sine * distance);
}

void EllipseShape::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QRectF rect(0, 0, width, height);

    applyFillStyle(painter);
    applyBorderStyle(painter);

    painter->drawEllipse(rect);
    drawText(painter, rect);
    drawSelectionOutline(painter);
}

Shape *EllipseShape::clone() const
{
    auto *cloned = new EllipseShape(x(), y(), width, height);
    this->copyConnectablePropertiesTo(*cloned);
    return cloned;
}
