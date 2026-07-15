#include "connector_anchor.h"
#include "../connectable_shape.h"

ConnectorAnchor ConnectorAnchor::createFree(QPointF scenePoint) {
    ConnectorAnchor anchor;
    anchor.mode = Mode::Free;
    anchor.freeScenePoint = scenePoint;
    anchor.fallbackScenePoint = scenePoint;
    return anchor;
}

ConnectorAnchor ConnectorAnchor::createBoundary(ConnectableShape* targetShape, qreal angleRadians, QPointF fallbackScenePoint) {
    if (!targetShape) {
        return createFree(fallbackScenePoint);
    }
    ConnectorAnchor anchor;
    anchor.mode = Mode::Boundary;
    anchor.targetShape = targetShape;
    anchor.boundaryAngle = angleRadians;
    anchor.fallbackScenePoint = fallbackScenePoint;
    anchor.resolveScenePoint(); // 只要被连目标图形存在，立刻无条件基于当前目标计算真实交点并 prime 安全气囊 fallbackScenePoint！
    return anchor;
}

ConnectorAnchor ConnectorAnchor::createInterior(ConnectableShape* targetShape, QPointF normalizedPosition, QPointF fallbackScenePoint) {
    if (!targetShape) {
        return createFree(fallbackScenePoint);
    }
    ConnectorAnchor anchor;
    anchor.mode = Mode::Interior;
    anchor.targetShape = targetShape;
    anchor.interiorNormalized = normalizedPosition;
    anchor.fallbackScenePoint = fallbackScenePoint;
    anchor.resolveScenePoint(); // 只要被连目标图形存在，立刻无条件基于当前目标和归一化点求出真实场景物理坐标并 prime 安全气囊！
    return anchor;
}

QPointF ConnectorAnchor::resolveScenePoint() const {
    // 1. 如果是 Free 模式，直接拿回 Free 点坐标并同步更新 fallback
    if (mode == Mode::Free) {
        fallbackScenePoint = freeScenePoint;
        return freeScenePoint;
    }

    // 2. 如果目标指针为空（说明目标图形在刚才被删了或最初未绑定成功），拿出安全气囊 fallback 坐标
    if (!targetShape) {
        return fallbackScenePoint;
    }

    // 3. 如果是 Boundary 模式，计算目标图形本地边界射线交点并转换到场景中
    if (mode == Mode::Boundary) {
        QPointF localPt = targetShape->boundaryPointAtAngle(boundaryAngle);
        fallbackScenePoint = targetShape->mapToScene(localPt); // 【安全气囊：更新有效物理点缓存】
        return fallbackScenePoint;
    }

    // 4. 如果是 Interior 模式，根据目标多边形路径外包矩形计算内部归一化对应点
    if (mode == Mode::Interior) {
        const QRectF bounds = targetShape->localGeometryPath().boundingRect();
        QPointF localPt(bounds.left() + interiorNormalized.x() * bounds.width(),
                        bounds.top() + interiorNormalized.y() * bounds.height());
        fallbackScenePoint = targetShape->mapToScene(localPt); // 【安全气囊：更新有效物理点缓存】
        return fallbackScenePoint;
    }

    return fallbackScenePoint;
}