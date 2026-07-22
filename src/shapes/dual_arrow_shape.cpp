#include "dual_arrow_shape.h"

#include <cmath>
#include <QLineF>
#include <QPolygonF>
#include <QPainterPathStroker>

QRectF DualArrowShape::boundingRect() const
{
    const qreal margin = border.borderWidth / 2.0 + arrowSize + 2.0;
    return QRectF(0, 0, width, height).normalized().adjusted(-margin, -margin, margin, margin);
}

void DualArrowShape::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
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
    const QPointF startHeadP1 = startPoint + QPointF(std::cos(angle + pi / 6.0) * arrowSize, -std::sin(angle + pi / 6.0) * arrowSize);
    const QPointF startHeadP2 = startPoint + QPointF(std::cos(angle - pi / 6.0) * arrowSize, -std::sin(angle - pi / 6.0) * arrowSize);
    const QPointF endHeadP1 = endPoint + QPointF(std::cos(angle + pi - pi / 6.0) * arrowSize, -std::sin(angle + pi - pi / 6.0) * arrowSize);
    const QPointF endHeadP2 = endPoint + QPointF(std::cos(angle + pi + pi / 6.0) * arrowSize, -std::sin(angle + pi + pi / 6.0) * arrowSize);

    QPolygonF startHead;
    startHead << startPoint << startHeadP1 << startHeadP2;
    QPolygonF endHead;
    endHead << endPoint << endHeadP1 << endHeadP2;

    painter->setBrush(border.borderColor);
    painter->drawPolygon(startHead);
    painter->drawPolygon(endHead);
    drawSelectionOutline(painter);
}

QPainterPath DualArrowShape::visualPath() const
{
    QPainterPath path = localGeometryPath();
    const QLineF line(startPoint, endPoint);
    if (line.length() > 0.0 && border.borderWidth > 0.0 && border.borderStyle != Qt::NoPen) {
        constexpr double pi = 3.14159265358979323846;
        const double angle = std::atan2(-line.dy(), line.dx());
        const QPointF startHeadP1 = startPoint + QPointF(std::cos(angle + pi / 6.0) * arrowSize, -std::sin(angle + pi / 6.0) * arrowSize);
        const QPointF startHeadP2 = startPoint + QPointF(std::cos(angle - pi / 6.0) * arrowSize, -std::sin(angle - pi / 6.0) * arrowSize);
        const QPointF endHeadP1 = endPoint + QPointF(std::cos(angle + pi - pi / 6.0) * arrowSize, -std::sin(angle + pi - pi / 6.0) * arrowSize);
        const QPointF endHeadP2 = endPoint + QPointF(std::cos(angle + pi + pi / 6.0) * arrowSize, -std::sin(angle + pi + pi / 6.0) * arrowSize);

        QPolygonF startHead;
        startHead << startPoint << startHeadP1 << startHeadP2;
        QPolygonF endHead;
        endHead << endPoint << endHeadP1 << endHeadP2;

        path.addPolygon(startHead);
        path.addPolygon(endHead);
    }
    return path;
}

QPainterPath DualArrowShape::shape() const
{
    QPainterPathStroker stroker;
    qreal hitWidth = std::max<qreal>(border.borderWidth, 10.0);
    stroker.setWidth(hitWidth);
    QPainterPath vPath = visualPath();
    return vPath.united(stroker.createStroke(vPath));
}

Shape *DualArrowShape::clone() const
{
    auto *cloned = new DualArrowShape(QPointF(), QPointF());
    copyConnectablePropertiesTo(*cloned);
    copyLineGeometryTo(*cloned);
    return cloned;
}
