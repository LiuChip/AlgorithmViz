#include "connector_anchor.h"
#include "../connectable_shape.h"
#include <cmath>

ConnectorAnchor ConnectorAnchor::createFree(QPointF scenePoint) {
    if (!std::isfinite(scenePoint.x()) || !std::isfinite(scenePoint.y())) {
        scenePoint = QPointF(0, 0);
    }
    ConnectorAnchor anchor;
    anchor.m_mode = Mode::Free;
    anchor.m_freeScenePoint = scenePoint;
    anchor.m_fallbackScenePoint = scenePoint;
    return anchor;
}

ConnectorAnchor ConnectorAnchor::createBoundary(ConnectableShape* targetShape, qreal angleRadians, QPointF fallbackScenePoint) {
    if (!targetShape || !std::isfinite(angleRadians)) {
        return createFree(fallbackScenePoint);
    }
    if (!std::isfinite(fallbackScenePoint.x()) || !std::isfinite(fallbackScenePoint.y())) {
        fallbackScenePoint = QPointF(0, 0);
    }
    ConnectorAnchor anchor;
    anchor.m_mode = Mode::Boundary;
    anchor.m_targetShape = targetShape;
    anchor.m_boundaryAngle = angleRadians;
    anchor.m_fallbackScenePoint = fallbackScenePoint;
    anchor.resolveScenePoint(); // 只要被连目标图形存在，立刻无条件基于当前目标计算真实交点并 prime 安全气囊 fallbackScenePoint！
    return anchor;
}

ConnectorAnchor ConnectorAnchor::createInterior(ConnectableShape* targetShape, QPointF normalizedPosition, QPointF fallbackScenePoint) {
    if (!targetShape || !std::isfinite(normalizedPosition.x()) || !std::isfinite(normalizedPosition.y())) {
        return createFree(fallbackScenePoint);
    }
    if (!std::isfinite(fallbackScenePoint.x()) || !std::isfinite(fallbackScenePoint.y())) {
        fallbackScenePoint = QPointF(0, 0);
    }
    qreal nx = std::max<qreal>(0.0, std::min<qreal>(1.0, normalizedPosition.x()));
    qreal ny = std::max<qreal>(0.0, std::min<qreal>(1.0, normalizedPosition.y()));
    ConnectorAnchor anchor;
    anchor.m_mode = Mode::Interior;
    anchor.m_targetShape = targetShape;
    anchor.m_interiorNormalized = QPointF(nx, ny);
    anchor.m_fallbackScenePoint = fallbackScenePoint;
    anchor.resolveScenePoint(); // 只要被连目标图形存在，立刻无条件基于当前目标和归一化点求出真实场景物理坐标并 prime 安全气囊！
    return anchor;
}

QPointF ConnectorAnchor::resolveScenePoint() const {
    // 1. 如果是 Free 模式，直接拿回 Free 点坐标并同步更新 fallback
    if (m_mode == Mode::Free) {
        m_fallbackScenePoint = m_freeScenePoint;
        return m_freeScenePoint;
    }

    // 2. 如果目标指针为空（说明目标图形在刚才被删了或最初未绑定成功），拿出安全气囊 fallback 坐标
    if (!m_targetShape) {
        return m_fallbackScenePoint;
    }

    // 3. 如果是 Boundary 模式，计算目标图形本地边界射线交点并转换到场景中
    if (m_mode == Mode::Boundary) {
        QPointF localPt = m_targetShape->boundaryPointAtAngle(m_boundaryAngle);
        QPointF scenePt = m_targetShape->mapToScene(localPt);
        // 只有解析结果合法时才更新安全气囊，否则沿用上一次有效值
        if (std::isfinite(scenePt.x()) && std::isfinite(scenePt.y())) {
            m_fallbackScenePoint = scenePt;
        }
        return m_fallbackScenePoint;
    }

    // 4. 如果是 Interior 模式，根据目标多边形路径外包矩形计算内部归一化对应点
    if (m_mode == Mode::Interior) {
        const QRectF bounds = m_targetShape->localGeometryPath().boundingRect();
        QPointF localPt(bounds.left() + m_interiorNormalized.x() * bounds.width(),
                        bounds.top() + m_interiorNormalized.y() * bounds.height());
        QPointF scenePt = m_targetShape->mapToScene(localPt);
        // 只有解析结果合法时才更新安全气囊，否则沿用上一次有效值
        if (std::isfinite(scenePt.x()) && std::isfinite(scenePt.y())) {
            m_fallbackScenePoint = scenePt;
        }
        return m_fallbackScenePoint;
    }

    return m_fallbackScenePoint;
}