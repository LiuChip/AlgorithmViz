
#include "anchor_resolver.h"
#include <QLineF>
#include <QPolygonF>
#include <cmath>
#include <limits>
#include <algorithm>

ConnectorAnchor AnchorResolver::resolve(const ResolveOptions& options)
{
    if (options.ctrlPressed || !options.scene ||
        !std::isfinite(options.scenePoint.x()) || !std::isfinite(options.scenePoint.y()) ||
        !std::isfinite(options.viewScale) || qFuzzyIsNull(options.viewScale) || options.viewScale <= 0.0 ||
        !std::isfinite(options.screenTolerancePx) || options.screenTolerancePx < 0.0) {
        // Ctrl 自由模式或缩放/坐标参数非法时，强制返回安全自由点
        return ConnectorAnchor::createFree(std::isfinite(options.scenePoint.x()) && std::isfinite(options.scenePoint.y()) ? options.scenePoint : QPointF(0, 0));
    }

    // 鼠标在图形外部
    // 遍历场景中的所有 ConnectableShape，寻找最近的边界锚点
    qreal minDistance = std::numeric_limits<qreal>::max();
    ConnectorAnchor closestAnchor;
    bool findBoundary = false;
    qreal tol = options.screenTolerancePx / options.viewScale;
    // 防止除法结果溢出为 Inf，同时限制容差最大值避免全场景搜索
    constexpr qreal kMaxSceneTolerance = 10000.0;
    if (!std::isfinite(tol) || tol > kMaxSceneTolerance) {
        return ConnectorAnchor::createFree(options.scenePoint);
    }
    QRectF searchBox(options.scenePoint.x() - tol, options.scenePoint.y() - tol, tol * 2, tol * 2);
    QList<QGraphicsItem*> nearbyItems = options.scene->items(searchBox);

    for (QGraphicsItem* item : nearbyItems) {
        // 尝试安全转成可吸附目标
        auto* target = dynamic_cast<ConnectableShape*>(item);
        if (!target || !target->isConnectableTarget() || target == options.ignoreShape || !target->isVisible()) {
            continue; // 如果转化失败（说明它是连线自己、普通线段、不可吸附或隐藏图元），直接跳过！
        }

        qreal angle;
        if (isNearBoundary(target, options.scenePoint, tol, &angle)) {
            QPointF boundaryPt = target->mapToScene(target->boundaryPointAtAngle(angle));
            qreal distance = QLineF(options.scenePoint, boundaryPt).length();
            if (distance <= tol && distance < minDistance) {
                minDistance = distance;
                closestAnchor = ConnectorAnchor::createBoundary(target, angle, boundaryPt);
                findBoundary = true;
            }
        }
    }
    if (findBoundary) return closestAnchor;

    // 鼠标在图形内部
    QList<QGraphicsItem*> underMouseItems = options.scene->items(options.scenePoint);
    for (QGraphicsItem* item : underMouseItems) {
        auto* target = dynamic_cast<ConnectableShape*>(item);
        if (!target || !target->isConnectableTarget() || target == options.ignoreShape || !target->isVisible()) {
            continue;
        }
        QPointF localPt = target->mapFromScene(options.scenePoint);
        if (target->localGeometryPath().contains(localPt)) {
            // 计算归一化坐标
            QRectF bounds = target->localGeometryPath().boundingRect();
            qreal u = (bounds.width() > 1e-6) ? (localPt.x() - bounds.left()) / bounds.width() : 0.5;
            qreal v = (bounds.height() > 1e-6) ? (localPt.y() - bounds.top()) / bounds.height() : 0.5;
            return ConnectorAnchor::createInterior(target, QPointF(u, v), options.scenePoint);
        }
    }

    // 画布或搜索范围为空
    return ConnectorAnchor::createFree(options.scenePoint);
}

bool AnchorResolver::isNearBoundary(ConnectableShape* target,
                                    const QPointF& scenePoint,
                                    qreal sceneTolerance,
                                    qreal* outAngle)
{
    if (!target || !std::isfinite(scenePoint.x()) || !std::isfinite(scenePoint.y()) ||
        !std::isfinite(sceneTolerance) || sceneTolerance < 0.0) {
        return false;
    }

    QPainterPath path = target->localGeometryPath();
    if (path.isEmpty()) {
        return false;
    }

    QRectF geomRect = path.boundingRect();
    QPointF center = geomRect.center();

    // 离散化路径为多边形线段，并将所有点映射至场景坐标系下直接计算真正距离最近的场景边界点
    QList<QPolygonF> polygons = path.toSubpathPolygons();
    if (polygons.isEmpty()) {
        return false;
    }

    qreal minDistSq = std::numeric_limits<qreal>::max();
    QPointF nearestScenePt = target->mapToScene(center);
    QPointF nearestLocalPt = center;

    for (const QPolygonF& poly : polygons) {
        qsizetype count = poly.size();
        if (count == 0) continue;
        if (count == 1) {
            QPointF pScene = target->mapToScene(poly.first());
            QPointF diff = scenePoint - pScene;
            qreal distSq = diff.x() * diff.x() + diff.y() * diff.y();
            if (distSq < minDistSq) {
                minDistSq = distSq;
                nearestScenePt = pScene;
                nearestLocalPt = poly.first();
            }
            continue;
        }
        bool closed = poly.isClosed();
        qsizetype edges = closed ? (count - 1) : count;
        for (qsizetype i = 0; i < edges; ++i) {
            if (!closed && i == count - 1) break; // 如果未闭合则不将尾顶点连接回首顶点
            QPointF p1Local = poly[i];
            QPointF p2Local = poly[(i + 1) % count];
            QPointF p1Scene = target->mapToScene(p1Local);
            QPointF p2Scene = target->mapToScene(p2Local);
            QPointF segScene = p2Scene - p1Scene;
            qreal segLenSq = segScene.x() * segScene.x() + segScene.y() * segScene.y();
            QPointF closestScene;
            qreal t = 0.0;
            if (segLenSq < 1e-12) {
                closestScene = p1Scene;
                t = 0.0;
            } else {
                t = ((scenePoint.x() - p1Scene.x()) * segScene.x() + (scenePoint.y() - p1Scene.y()) * segScene.y()) / segLenSq;
                t = std::max<qreal>(0.0, std::min<qreal>(1.0, t));
                closestScene = p1Scene + t * segScene;
            }
            QPointF diff = scenePoint - closestScene;
            qreal distSq = diff.x() * diff.x() + diff.y() * diff.y();
            if (distSq < minDistSq) {
                minDistSq = distSq;
                nearestScenePt = closestScene;
                nearestLocalPt = p1Local + t * (p2Local - p1Local);
            }
        }
    }

    qreal distance = QLineF(scenePoint, nearestScenePt).length();
    if (distance <= sceneTolerance) {
        if (outAngle) {
            *outAngle = std::atan2(nearestLocalPt.y() - center.y(), nearestLocalPt.x() - center.x());
        }
        return true;
    }
    return false;
}