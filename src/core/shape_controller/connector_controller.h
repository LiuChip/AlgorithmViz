#ifndef CONNECTOR_CONTROLLER_H
#define CONNECTOR_CONTROLLER_H

#include "shapes/connector/connector.h"
#include "shapes/connector/connector_anchor.h"
#include "anchor_resolver.h"

#include <QObject>
#include <QPointF>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QPointer>
#include <QGraphicsObject>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QRectF>

class Canvas; // 前置声明画布类

class SnapIndicator : public QGraphicsObject {
    Q_OBJECT
public:
    explicit SnapIndicator(QGraphicsItem* parent = nullptr) : QGraphicsObject(parent) {
        setAcceptedMouseButtons(Qt::NoButton);
        setZValue(99999);
        setData(0, "SNAP_INDICATOR");
    }

    QRectF boundingRect() const override {
        return m_rect;
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override {
        (void)option;
        (void)widget;
        painter->setPen(m_pen);
        painter->setBrush(m_brush);
        painter->drawEllipse(m_rect);
    }

    void setPen(const QPen& pen) {
        m_pen = pen;
        update();
    }

    void setBrush(const QBrush& brush) {
        m_brush = brush;
        update();
    }

    void setRect(qreal x, qreal y, qreal w, qreal h) {
        QRectF newRect(x, y, w, h);
        if (m_rect != newRect) {
            prepareGeometryChange();
            m_rect = newRect;
            update();
        }
    }

private:
    QRectF m_rect;
    QPen m_pen;
    QBrush m_brush;
};

class ConnectorController : public QObject {
    Q_OBJECT

public:
    enum class State {
        Idle,             // 空闲状态
        Creating,         // 创建中（同时兼容拖拽创建 & 两点点击创建）
        DraggingEndpoint  // 正在拖拽既有连线的端点手柄
    };

    enum class EndpointType {
        None,
        Start,
        End
    };

    explicit ConnectorController(Canvas* canvas, QObject* parent = nullptr);
    ~ConnectorController() override;

    void setCreateModeActive(bool active);
    bool isCreateModeActive() const { return m_createModeActive; }
    State state() const { return m_state; }

    // 单元测试与状态安全查询辅助接口
    bool snapIndicatorIsNull() const { return m_snapIndicator.isNull(); }
    SnapIndicator* snapIndicator() const { return m_snapIndicator.data(); }

    // 原生事件对象捕获接口（供现有或基于事件对象的场景调用）
    bool handleMousePressEvent(QGraphicsSceneMouseEvent* event);
    bool handleMouseMoveEvent(QGraphicsSceneMouseEvent* event);
    bool handleMouseReleaseEvent(QGraphicsSceneMouseEvent* event);
    bool handleKeyPressEvent(QKeyEvent* event);
    bool handleKeyReleaseEvent(QKeyEvent* event);

    // 推荐的数据解耦型操作路由接口（供 Canvas::mousePressEvent 等不依赖具体事件类型的场景直接调用）
    bool handleMousePress(const QPointF& scenePos, Qt::MouseButton button, Qt::KeyboardModifiers modifiers);
    bool handleMouseMove(const QPointF& scenePos, Qt::KeyboardModifiers modifiers);
    bool handleMouseRelease(const QPointF& scenePos, Qt::MouseButton button, Qt::KeyboardModifiers modifiers);
    bool handleKeyPress(int key, Qt::KeyboardModifiers modifiers);
    bool handleKeyRelease(int key, Qt::KeyboardModifiers modifiers);

    void cancelCurrentOperation();
    void resetOperationState();

signals:
    void connectorCreated(Connector* connector);
    void connectorModified(Connector* connector);

private:
    void startCreating(const QPointF& scenePos, bool ctrlPressed);
    void updateCreating(const QPointF& scenePos, bool ctrlPressed);
    void finishCreating(const QPointF& scenePos, bool ctrlPressed);
    void abortCreating();

    void startDraggingEndpoint(Connector* connector, EndpointType endpoint, const QPointF& scenePos, bool ctrlPressed);
    void updateDraggingEndpoint(const QPointF& scenePos, bool ctrlPressed);
    void finishDraggingEndpoint(const QPointF& scenePos, bool ctrlPressed);

    void updateSnapIndicator(const ConnectorAnchor& anchor);
    void hideSnapIndicator();
    void destroySnapIndicator();

    qreal getSafeViewScale() const;
    bool hitTestConnectorEndpoint(const QPointF& scenePos, Connector*& outConnector, EndpointType& outType) const;
    bool isSelfConnection(const ConnectorAnchor& start, const ConnectorAnchor& end) const;

private:
    QPointer<Canvas> m_canvas;
    State m_state = State::Idle;
    bool m_createModeActive = false;

    // 用 QPointer 防止目标被信号槽外部或异常销毁引发野指针崩溃
    QPointer<Connector> m_activeConnector;
    EndpointType m_activeEndpointType = EndpointType::None;

    // 拖拽已有端点前备份旧锚点（用于用户按 ESC 键强行取消时光速回滚）
    ConnectorAnchor m_backupAnchor;

    // 保存创建初次点击的原始物理坐标（不收边框吸附影响），解决两点点击创建距离被吸附干扰的误判
    QPointF m_createStartScenePos;

    // 保存最近一次鼠标场景坐标，避免 Ctrl 键事件依赖全局光标位置
    QPointF m_lastScenePos;

    // 标记是否需要在松手事件中吸收当前点击释放（防止两点点击第二下 Press 直接完成后 Release 漏出传播到画布）
    bool m_consumeNextRelease = false;

    // 高亮吸附预览光点图元（QGraphicsObject化）与场景指针关联（防止场景或画布提前销毁产生悬空裸指针）
    QPointer<SnapIndicator> m_snapIndicator;
    QPointer<QGraphicsScene> m_indicatorScene;
};

#endif // CONNECTOR_CONTROLLER_H