#ifndef CONNECTOR_ANCHOR_H
#define CONNECTOR_ANCHOR_H


#include <QPointF>
#include <QPointer> // 建议引入，配合稍后说明的安全机制

#include "../connectable_shape.h"

struct ConnectorAnchor {
    enum class Mode {
        Free,     // 自由端点（不绑任何图形）
        Boundary, // 绑在图形边界
        Interior  // 绑在图形内部
    };

    Mode mode = Mode::Free;
    QPointF freeScenePoint;                 // Free 模式下的固定坐标
    QPointer<ConnectableShape> targetShape; // 绑定的目标图形
    qreal boundaryAngle = 0.0;              // Boundary 模式下的偏转角度
    QPointF interiorNormalized;             // Interior 模式下的归一化坐标(如中心为 0.5, 0.5)
    mutable QPointF fallbackScenePoint;     // 实时成功解析出的场景物理坐标缓存（安全气囊）

    // 核心方法：算出此端点当前在画布场景中的绝对坐标
    QPointF resolveScenePoint() const;

    // 推荐给 UI/控制层调用的三个静态构造工厂方法
    static ConnectorAnchor createFree(QPointF scenePoint);
    static ConnectorAnchor createBoundary(ConnectableShape* target, qreal angleRadians, QPointF fallbackScenePoint = QPointF());
    static ConnectorAnchor createInterior(ConnectableShape* target, QPointF normalizedPosition, QPointF fallbackScenePoint = QPointF());
};

#endif // CONNECTOR_ANCHOR_H
