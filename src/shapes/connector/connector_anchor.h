#ifndef CONNECTOR_ANCHOR_H
#define CONNECTOR_ANCHOR_H


#include <QPointF>
#include <QPointer> // 建议引入，配合稍后说明的安全机制

#include "../connectable_shape.h"

class ConnectorAnchor {
public:
    enum class Mode {
        Free,     // 自由端点（不绑任何图形）
        Boundary, // 绑在图形边界
        Interior  // 绑在图形内部
    };

    ConnectorAnchor() = default;

    // 推荐给 UI/控制层调用的三个静态构造工厂方法
    static ConnectorAnchor createFree(QPointF scenePoint);
    static ConnectorAnchor createBoundary(ConnectableShape* target, qreal angleRadians, QPointF fallbackScenePoint = QPointF());
    static ConnectorAnchor createInterior(ConnectableShape* target, QPointF normalizedPosition, QPointF fallbackScenePoint = QPointF());

    // 核心方法：算出此端点当前在画布场景中的绝对坐标
    QPointF resolveScenePoint() const;

    // 状态与属性查询访问器
    Mode getMode() const { return m_mode; }
    Mode mode() const { return m_mode; }
    ConnectableShape* getTargetShape() const { return m_targetShape.data(); }
    ConnectableShape* targetShape() const { return m_targetShape.data(); }
    qreal getBoundaryAngle() const { return m_boundaryAngle; }
    QPointF getInteriorNormalized() const { return m_interiorNormalized; }
    QPointF getFreeScenePoint() const { return m_freeScenePoint; }
    QPointF getFallbackScenePoint() const { return m_fallbackScenePoint; }
    QPointF fallbackScenePoint() const { return m_fallbackScenePoint; }

    // 将锚点降级为 Free 模式并采用当前安全气囊坐标作为绝对位置
    void degradeToFree() {
        m_mode = Mode::Free;
        m_freeScenePoint = m_fallbackScenePoint;
        m_targetShape.clear();
    }

private:
    Mode m_mode = Mode::Free;
    QPointF m_freeScenePoint;                 // Free 模式下的固定坐标
    QPointer<ConnectableShape> m_targetShape; // 绑定的目标图形
    qreal m_boundaryAngle = 0.0;              // Boundary 模式下的偏转角度
    QPointF m_interiorNormalized;             // Interior 模式下的归一化坐标(如中心为 0.5, 0.5)
    mutable QPointF m_fallbackScenePoint;     // 实时成功解析出的场景物理坐标缓存（安全气囊）

    friend class Connector;
};

#endif // CONNECTOR_ANCHOR_H
