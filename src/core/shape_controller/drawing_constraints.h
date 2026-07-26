#ifndef DRAWING_CONSTRAINTS_H
#define DRAWING_CONSTRAINTS_H

#include <QPointF>
#include <QRectF>

#include <cmath>
#include <algorithm>

// 绘图阶段的几何约束工具。
// 约束只作用于“鼠标拖拽创建/调整预览”的输入，不改变图元自身的几何主宰规则。
namespace DrawingConstraints {

inline bool isFinitePoint(const QPointF &point)
{
    return std::isfinite(point.x()) && std::isfinite(point.y());
}

// 将终点吸附到相对于 start 的最近 45 度方向，同时保持鼠标拖拽距离。
inline QPointF constrainLineEndpoint(const QPointF &start, const QPointF &rawEnd)
{
    const QPointF delta = rawEnd - start;
    const qreal length = std::hypot(delta.x(), delta.y());
    if (!std::isfinite(length) || length <= 1e-9 || !isFinitePoint(start) || !isFinitePoint(rawEnd)) {
        return rawEnd;
    }

    constexpr qreal pi = 3.1415926535897932384626433832795;
    constexpr qreal angleStep = pi / 4.0;
    const qreal angle = std::atan2(delta.y(), delta.x());
    const qreal snappedAngle = std::round(angle / angleStep) * angleStep;
    return start + QPointF(std::cos(snappedAngle) * length,
                           std::sin(snappedAngle) * length);
}

// 根据起点和鼠标终点生成拖拽矩形。
// constrained=true 时使用较长轴作为正方形边长，并保留拖拽象限。
inline QRectF makeDragRect(const QPointF &start, const QPointF &rawEnd, bool constrained)
{
    QPointF end = rawEnd;
    if (constrained) {
        const QPointF delta = rawEnd - start;
        const qreal absX = std::abs(delta.x());
        const qreal absY = std::abs(delta.y());
        const qreal side = std::max(absX, absY);

        if (side > 1e-9 && std::isfinite(side)) {
            // 鼠标严格垂直/水平时，使用另一个轴的方向补足象限，避免正方形随机翻转。
            const qreal signX = delta.x() < 0.0 ? -1.0 : (delta.x() > 0.0 ? 1.0 : (delta.y() < 0.0 ? -1.0 : 1.0));
            const qreal signY = delta.y() < 0.0 ? -1.0 : (delta.y() > 0.0 ? 1.0 : signX);
            end = start + QPointF(signX * side, signY * side);
        }
    }
    return QRectF(start, end).normalized();
}

} // namespace DrawingConstraints

#endif // DRAWING_CONSTRAINTS_H
