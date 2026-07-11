#include "dual_arrow_shape.h"

#include <cmath>
#include <QLineF>
#include <QPolygonF>

QRectF DualArrowShape::boundingRect() const
{
    const qreal margin = border.borderWidth / 2.0 + arrowSize + 2.0;
    return QRectF(0, 0, width, height).normalized().adjusted(-margin, -margin, margin, margin);
}

void DualArrowShape::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (border.borderWidth > 0.0 && border.borderStyle != Qt::NoPen)
    {
        QPen pen(border.borderColor, border.borderWidth, border.borderStyle);
        painter->setPen(pen);
    }
    else
    {
        painter->setPen(Qt::NoPen);
    }

    const QPointF localStart = startPoint - pos();
    const QPointF localEnd = endPoint - pos();
    painter->drawLine(localStart, localEnd);

    const QLineF line(localStart, localEnd);
    if (line.length() <= 0.0) {
        return;
    }

    constexpr double pi = 3.14159265358979323846;
    const double angle = std::atan2(-line.dy(), line.dx());
    const QPointF startHeadP1 = localStart + QPointF(std::cos(angle + pi / 6.0) * arrowSize, -std::sin(angle + pi / 6.0) * arrowSize);
    const QPointF startHeadP2 = localStart + QPointF(std::cos(angle - pi / 6.0) * arrowSize, -std::sin(angle - pi / 6.0) * arrowSize);
    const QPointF endHeadP1 = localEnd + QPointF(std::cos(angle + pi - pi / 6.0) * arrowSize, -std::sin(angle + pi - pi / 6.0) * arrowSize);
    const QPointF endHeadP2 = localEnd + QPointF(std::cos(angle + pi + pi / 6.0) * arrowSize, -std::sin(angle + pi + pi / 6.0) * arrowSize);

    QPolygonF startHead;
    startHead << localStart << startHeadP1 << startHeadP2;
    QPolygonF endHead;
    endHead << localEnd << endHeadP1 << endHeadP2;

    painter->setBrush(border.borderColor);
    painter->drawPolygon(startHead);
    painter->drawPolygon(endHead);
}
