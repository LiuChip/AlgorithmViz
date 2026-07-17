#include "arrow_shape.h"

#include <cmath>
#include <QLineF>
#include <QPolygonF>
#include <QPainterPathStroker>

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
    const QLineF line(startPoint, endPoint);
    if (line.length() <= 0.0 || border.borderWidth <= 0.0 || border.borderStyle == Qt::NoPen) {
        return;
    }
    painter->drawLine(startPoint, endPoint);

    constexpr double pi = 3.14159265358979323846;
    const double angle = std::atan2(-line.dy(), line.dx());
    const QPointF arrowP1 = endPoint + QPointF(std::cos(angle + pi - pi / 6.0) * arrowSize, -std::sin(angle + pi - pi / 6.0) * arrowSize);
    const QPointF arrowP2 = endPoint + QPointF(std::cos(angle + pi + pi / 6.0) * arrowSize, -std::sin(angle + pi + pi / 6.0) * arrowSize);

    QPolygonF head;
    head << endPoint << arrowP1 << arrowP2;

    painter->setBrush(border.borderColor);
    painter->drawPolygon(head);
}

QPainterPath ArrowShape::visualPath() const
{
    QPainterPath path = localGeometryPath();
    const QLineF line(startPoint, endPoint);
    if (line.length() > 0.0 && border.borderWidth > 0.0 && border.borderStyle != Qt::NoPen) {
        constexpr double pi = 3.14159265358979323846;
        const double angle = std::atan2(-line.dy(), line.dx());
        const QPointF arrowP1 = endPoint + QPointF(std::cos(angle + pi - pi / 6.0) * arrowSize, -std::sin(angle + pi - pi / 6.0) * arrowSize);
        const QPointF arrowP2 = endPoint + QPointF(std::cos(angle + pi + pi / 6.0) * arrowSize, -std::sin(angle + pi + pi / 6.0) * arrowSize);
        QPolygonF head;
        head << endPoint << arrowP1 << arrowP2;
        path.addPolygon(head);
    }
    return path;
}

QPainterPath ArrowShape::shape() const
{
    QPainterPathStroker stroker;
    qreal hitWidth = std::max<qreal>(border.borderWidth, 10.0);
    stroker.setWidth(hitWidth);
    QPainterPath vPath = visualPath();
    return vPath.united(stroker.createStroke(vPath));
}

Shape *ArrowShape::clone() const
{
    auto *cloned = new ArrowShape(QPointF(), QPointF());
    copyConnectablePropertiesTo(*cloned);
    copyLineGeometryTo(*cloned);
    return cloned;
}
