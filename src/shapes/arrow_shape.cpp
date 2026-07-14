#include "arrow_shape.h"

#include <cmath>
#include <QLineF>
#include <QPolygonF>

QRectF ArrowShape::boundingRect() const
{
    const qreal margin = border.borderWidth / 2.0 + arrowSize + 2.0;
    return QRectF(0, 0, width, height).normalized().adjusted(-margin, -margin, margin, margin);
}

void ArrowShape::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    applyBorderStyle(painter);
    painter->drawLine(startPoint, endPoint);

    const QLineF line(startPoint, endPoint);
    if (line.length() <= 0.0) {
        return;
    }

    constexpr double pi = 3.14159265358979323846;
    const double angle = std::atan2(-line.dy(), line.dx());
    const QPointF arrowP1 = endPoint + QPointF(std::cos(angle + pi - pi / 6.0) * arrowSize, -std::sin(angle + pi - pi / 6.0) * arrowSize);
    const QPointF arrowP2 = endPoint + QPointF(std::cos(angle + pi + pi / 6.0) * arrowSize, -std::sin(angle + pi + pi / 6.0) * arrowSize);

    QPolygonF head;
    head << endPoint << arrowP1 << arrowP2;

    painter->setBrush(border.borderColor);
    painter->drawPolygon(head);
}

Shape *ArrowShape::clone() const
{
    auto *cloned = new ArrowShape(QPointF(), QPointF());
    copyConnectablePropertiesTo(*cloned);
    copyLineGeometryTo(*cloned);
    return cloned;
}
