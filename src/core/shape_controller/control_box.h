#ifndef CONTROL_BOX_H
#define CONTROL_BOX_H

#include <QGraphicsObject>
#include <QPointer>
#include <QPointF>
#include <QSizeF>
#include <QList>
#include <QPen>
#include <QBrush>
#include "shapes/shape.h"

class Shape;
class QGraphicsSceneMouseEvent;

enum class HandleType {
    TopLeft, Top, TopRight,
    Right,
    BottomRight, Bottom, BottomLeft,
    Left,
    Rotate,         // 旋转手柄（圆形，位于上方中点）
    StartEndpoint,  // 线段/连线起点手柄
    EndEndpoint     // 线段/连线终点手柄
};

class ControlBox;

class HandleItem : public QGraphicsObject
{
    Q_OBJECT
public:
    explicit HandleItem(HandleType type, ControlBox *box, QGraphicsItem *parent = nullptr);
    ~HandleItem() override;
    HandleType handleType() const;

    void setHoverCursor();// 根据手柄类型设置悬停光标

    // 重载 QGraphicsItem 的虚函数
    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

signals:
    void handlePressed(HandleItem *handle, const QPointF &scenePos);
    void handleMoved(HandleItem *handle, const QPointF &scenePos, Qt::KeyboardModifiers modifiers);
    void handleReleased(HandleItem *handle, const QPointF &scenePos);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    HandleType m_type;
    ControlBox *m_box = nullptr;
    qreal m_size = 8.0; // 屏幕空间像素大小 (因 ItemIgnoresTransformations 生效)



};

class ControlBox : public QGraphicsObject
{
    Q_OBJECT
    friend class ConnectorTest;
public:
    explicit ControlBox(QGraphicsItem *parent = nullptr);
    ~ControlBox() override;

    void setTarget(Shape* target);
    Shape* target() const;

    void updateHandlePositions();
    // QGraphicsItem 重载
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
signals:
    void resizeStarted(Shape *target);
    void resizeFinished(Shape *target, const QSizeF &oldSize, const QSizeF &newSize, const QPointF &oldPos, const QPointF &newPos);
    void rotateStarted(Shape *target);
    void rotateFinished(Shape *target, qreal oldAngle, qreal newAngle);
    void endpointMoveStarted(Shape *target, HandleType endpoint);
    void endpointMoveFinished(Shape *target, HandleType endpoint, const QPointF &oldScenePos, const QPointF &newScenePos);

private slots:
    void onTargetGeometryChanged();
    void onTargetDestroyed();
    void onTargetLockedChanged(bool locked);
    void onHandlePressed(HandleItem *handle, const QPointF &scenePos);
    void onHandleMoved(HandleItem *handle, const QPointF &scenePos, Qt::KeyboardModifiers modifiers);
    void onHandleReleased(HandleItem *handle, const QPointF &scenePos);

private:
    void createHandlesForNormalShape();
    void createHandlesForLineShape();
    void clearHandles();
    HandleItem* findHandle(HandleType type) const;// 辅助查找对应位置的函数

    // 对角线/中心手柄缩放辅助算子
    QPointF getOppositeAnchorPos(HandleType type) const;
    void applyResizeFromAnchor(HandleType type, const QPointF &anchorScenePos, const QPointF &mouseScenePos, Qt::KeyboardModifiers modifiers);
private:
    QPointer<Shape> m_target;
    QList<HandleItem*> m_handles;
    // 操作期间的初始快照记录
    QSizeF m_startSize;
    QPointF m_startPos;
    qreal m_startRotation = 0.0;
    QPointF m_startEndpointPos;
    bool m_isOperating = false;

};

#endif // CONTROL_BOX_H
