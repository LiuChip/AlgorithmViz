#ifndef CANVAS_CONTROLLER_H
#define CANVAS_CONTROLLER_H

#include <QObject>
#include <QPointF>
#include <QSet>
#include <QMap>
#include <QPointer>
#include <QTimer>
#include <QGraphicsObject>
#include <QPen>
#include <QBrush>
#include <QPainter>

class Canvas;
class Shape;
class ControlBox;
class QMouseEvent;
class QKeyEvent;
class QGraphicsScene;

// 自定义橡皮筋框，继承 QGraphicsObject 以支持 QPointer 安全追踪生命周期
class RubberBandItem : public QGraphicsObject
{
    Q_OBJECT
public:
    explicit RubberBandItem(QGraphicsItem *parent = nullptr) : QGraphicsObject(parent) {}
    ~RubberBandItem() override = default;

    QRectF boundingRect() const override { return m_rect; }
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override
    {
        Q_UNUSED(option); Q_UNUSED(widget);
        painter->save();
        painter->setPen(m_pen);
        painter->setBrush(m_brush);
        painter->drawRect(m_rect);
        painter->restore();
    }
    void setRect(const QRectF &rect) {
        if (m_rect == rect) return;
        prepareGeometryChange();
        m_rect = rect;
        update();
    }
    void setPen(const QPen &pen) { m_pen = pen; update(); }
    void setBrush(const QBrush &brush) { m_brush = brush; update(); }
private:
    QRectF m_rect;
    QPen m_pen;
    QBrush m_brush;
};

// 图形画布核心交互控制器：全权统管图元的创建、选择、拖拽位移以及微调加速机制
class CanvasController : public QObject
{
    Q_OBJECT
public:
    enum class InteractionState {
        Idle,            // 空闲状态（常规选中或无操作）
        CreatingShape,   // 正在拉拽外框创建图元
        MovingItems,     // 正在鼠标拖动选中的图元集合
        KeyMovingItems   // 正在通过键盘 WASD/方向键长按加速移动图元
    };

    explicit CanvasController(Canvas *canvas, QObject *parent = nullptr);
    ~CanvasController() override;

    void attachToScene(QGraphicsScene *scene);
    void ensureOverlayItems();

    // 事件转接核心接口
    bool handleMousePressEvent(QMouseEvent *event);
    bool handleMouseMoveEvent(QMouseEvent *event);
    bool handleMouseReleaseEvent(QMouseEvent *event);
    bool handleKeyPressEvent(QKeyEvent *event);
    bool handleKeyReleaseEvent(QKeyEvent *event);

    void cancelCurrentOperation();

    // 状态与选区查询公开接口
    InteractionState currentState() const { return m_state; }
    QSet<Shape*> selectedItems() const { return m_selectedItems; }
    Shape* primarySelection() const { return m_primarySelection.data(); }
    ControlBox* controlBox() const;

    // 选区管理操作
    void selectItem(Shape *shape, bool clearOthers = true);
    void deselectItem(Shape *shape);
    void clearSelection();

public slots:
    void copy();
    void paste();
    void cut();
    void deleteSelected();
    void selectAll();

private slots:
    // KeyMoveEngine：按住键盘不放时的按帧加速平移回调
    void onKeyMoveTick();
    // 当所选的或追踪的 Shape 对象销毁时安全回收指针
    void onShapeDestroyed(QObject *object);

private:
    // 内部私有辅助
    void setState(InteractionState state);
    void updateControlBoxTarget();
    Shape* findShapeAt(const QPointF &scenePos) const;
    Shape* createShapeInstance(const QPointF &startScenePos, const QPointF &endScenePos);

    // 依赖与子组件
    Canvas *m_canvas = nullptr;
    QPointer<ControlBox> m_controlBox;
    QPointer<RubberBandItem> m_rubberBandItem; // 创建图形或框选时的橡皮筋矩形（QPointer 自动防空）

    // 交互状态与选区
    InteractionState m_state = InteractionState::Idle;
    QSet<Shape*> m_selectedItems;
    QPointer<Shape> m_primarySelection;

    // --- 图形创建快照 ---
    QPointF m_createStartScenePos;

    // --- 鼠标拖移图元快照 ---
    QPointF m_dragStartScenePos;
    QMap<Shape*, QPointF> m_dragStartPositions;

    // --- KeyMoveEngine 键盘加速移动引擎状态 ---
    QTimer *m_keyMoveTimer = nullptr;
    QSet<int> m_pressedKeys;               // 当前被按压不放的位移按键集合 (WASD & 方向键)
    qreal m_keyMoveSpeed = 1.0;            // 连续滑动当前的加速倍率
    QMap<Shape*, QPointF> m_keyMoveStartPositions; // 键盘按下初始瞬间图元位置，供最后的 Undo 打包用
};

#endif // CANVAS_CONTROLLER_H