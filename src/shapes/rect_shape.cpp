#include "rect_shape.h"

#include <cmath>
#include <limits>

QRectF RectShape::boundingRect() const
{
    qreal halfPen = border.borderWidth / 2.0;
    return QRectF(-halfPen, -halfPen, width + border.borderWidth, height + border.borderWidth);
}

QPainterPath RectShape::localGeometryPath() const
{
    QPainterPath path;
    path.addRect(QRectF(0, 0, width, height));
    return path;
}

QPointF RectShape::boundaryPointAtAngle(qreal angleRadians) const
{
    const QPointF center(width / 2.0, height / 2.0);
    const QPointF direction(std::cos(angleRadians), std::sin(angleRadians));
    constexpr qreal epsilon = 1e-12;
    qreal distance = std::numeric_limits<qreal>::max();

    auto consider = [&](qreal candidate) {
        if (candidate <= epsilon || candidate >= distance) {
            return;
        }
        const QPointF point = center + direction * candidate;
        if (point.x() >= -epsilon && point.x() <= width + epsilon &&
            point.y() >= -epsilon && point.y() <= height + epsilon) {
            distance = candidate;
        }
    };

    if (std::abs(direction.x()) > epsilon) {
        consider((0.0 - center.x()) / direction.x());
        consider((width - center.x()) / direction.x());
    }
    if (std::abs(direction.y()) > epsilon) {
        consider((0.0 - center.y()) / direction.y());
        consider((height - center.y()) / direction.y());
    }

    if (distance == std::numeric_limits<qreal>::max()) {
        return center;
    }
    return center + direction * distance;
}

void RectShape::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QRectF rect(0, 0, width, height);

    applyFillStyle(painter);
    applyBorderStyle(painter);

    painter->drawRect(rect);
    drawText(painter, rect);
    drawSelectionOutline(painter);
}

Shape *RectShape::clone() const
{
    auto *cloned = new RectShape(x(), y(), width, height);
    this->copyConnectablePropertiesTo(*cloned);
    return cloned;
}
