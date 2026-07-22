#include "canvas_controller.h"
#include "control_box.h"
#include "core/canvas.h"
#include "shapes/shape.h"
#include "shapes/rect_shape.h"
#include "shapes/ellipse_shape.h"
#include "shapes/diamond_shape.h"
#include "shapes/line_shape.h"
#include "shapes/arrow_shape.h"
#include "shapes/dual_arrow_shape.h"
#include "shapes/text_label.h"

#include <QMouseEvent>
#include <QKeyEvent>
#include <QGraphicsScene>
#include <QPen>
#include <QBrush>
#include <QApplication>
#include <cmath>

CanvasController::CanvasController(Canvas *canvas, QObject *parent)
    : QObject(parent), m_canvas(canvas)
{
    m_controlBox = new ControlBox();
    m_controlBox->setZValue(99999.0);
    m_controlBox->setVisible(false);

    m_rubberBandItem = new RubberBandItem();
    QPen dashedPen(QColor(0, 120, 215), 1.0, Qt::DashLine);
    dashedPen.setCosmetic(true);
    m_rubberBandItem->setPen(dashedPen);
    m_rubberBandItem->setBrush(QBrush(QColor(0, 120, 215, 40)));
    m_rubberBandItem->setZValue(99990.0);
    m_rubberBandItem->setVisible(false);

    if (m_canvas && m_canvas->scene()) {
        attachToScene(m_canvas->scene());
    }

    m_keyMoveTimer = new QTimer(this);
    m_keyMoveTimer->setInterval(16); // ~60FPS
    connect(m_keyMoveTimer, &QTimer::timeout, this, &CanvasController::onKeyMoveTick);
}

CanvasController::~CanvasController()
{
    if (m_controlBox) {
        delete m_controlBox;
    }
    if (m_rubberBandItem) {
        delete m_rubberBandItem;
    }
}

void CanvasController::ensureOverlayItems()
{
    if (!m_controlBox) {
        m_controlBox = new ControlBox();
        m_controlBox->setZValue(99999.0);
        m_controlBox->setVisible(false);
    }
    if (!m_rubberBandItem) {
        m_rubberBandItem = new RubberBandItem();
        QPen dashedPen(QColor(0, 120, 215), 1.0, Qt::DashLine);
        dashedPen.setCosmetic(true);
        m_rubberBandItem->setPen(dashedPen);
        m_rubberBandItem->setBrush(QBrush(QColor(0, 120, 215, 40)));
        m_rubberBandItem->setZValue(99990.0);
        m_rubberBandItem->setVisible(false);
    }
    if (m_canvas && m_canvas->scene()) {
        if (!m_controlBox->scene()) {
            m_canvas->scene()->addItem(m_controlBox);
        }
        if (!m_rubberBandItem->scene()) {
            m_canvas->scene()->addItem(m_rubberBandItem);
        }
    }
}

void CanvasController::attachToScene(QGraphicsScene *scene)
{
    if (!scene) return;
    ensureOverlayItems();
}

void CanvasController::setState(InteractionState state)
{
    if (m_state == state) return;
    m_state = state;
}

ControlBox* CanvasController::controlBox() const
{
    return m_controlBox.data();
}

void CanvasController::updateControlBoxTarget()
{
    ensureOverlayItems();
    if (!m_controlBox) return;
    if (m_selectedItems.size() == 1 && m_primarySelection) {
        m_controlBox->setTarget(m_primarySelection.data());
    } else {
        m_controlBox->setTarget(nullptr);
    }
}

Shape* CanvasController::findShapeAt(const QPointF &scenePos) const
{
    if (!m_canvas || !m_canvas->scene()) return nullptr;
    QList<QGraphicsItem*> items = m_canvas->scene()->items(scenePos);
    for (QGraphicsItem *item : items) {
        if (item == m_controlBox || item == m_rubberBandItem) continue;
        if (auto *shape = dynamic_cast<Shape*>(item)) {
            if (shape->isVisible()) {
                return shape;
            }
        }
    }
    return nullptr;
}

void CanvasController::selectItem(Shape *shape, bool clearOthers)
{
    if (!shape) return;
    if (clearOthers) {
        clearSelection();
    }
    if (!m_selectedItems.contains(shape)) {
        m_selectedItems.insert(shape);
        connect(shape, &QObject::destroyed, this, &CanvasController::onShapeDestroyed, Qt::UniqueConnection);
    }
    m_primarySelection = shape;
    shape->setSelected(true);
    updateControlBoxTarget();
}

void CanvasController::deselectItem(Shape *shape)
{
    if (!shape) return;
    if (m_selectedItems.remove(shape)) {
        disconnect(shape, &QObject::destroyed, this, &CanvasController::onShapeDestroyed);
        shape->setSelected(false);
        if (m_primarySelection == shape) {
            if (!m_selectedItems.isEmpty()) {
                m_primarySelection = *m_selectedItems.begin();
            } else {
                m_primarySelection = nullptr;
            }
        }
        updateControlBoxTarget();
    }
}

void CanvasController::clearSelection()
{
    for (Shape *shape : m_selectedItems) {
        if (shape) {
            disconnect(shape, &QObject::destroyed, this, &CanvasController::onShapeDestroyed);
            shape->setSelected(false);
        }
    }
    m_selectedItems.clear();
    m_primarySelection = nullptr;
    updateControlBoxTarget();
}

void CanvasController::onShapeDestroyed(QObject *object)
{
    Shape *s = static_cast<Shape*>(object);
    m_selectedItems.remove(s);
    m_dragStartPositions.remove(s);
    m_keyMoveStartPositions.remove(s);
    if (m_primarySelection.data() == s) {
        if (!m_selectedItems.isEmpty()) {
            m_primarySelection = *m_selectedItems.begin();
        } else {
            m_primarySelection = nullptr;
        }
    }
    updateControlBoxTarget();
}

Shape* CanvasController::createShapeInstance(const QPointF &startScenePos, const QPointF &endScenePos)
{
    if (!m_canvas) return nullptr;
    const Canvas::ToolMode mode = m_canvas->toolMode();

    const QPointF diff = endScenePos - startScenePos;
    const qreal dist = std::hypot(diff.x(), diff.y());
    bool isClickCreate = (dist < 5.0);

    QRectF rect;
    if (isClickCreate) {
        rect = QRectF(startScenePos.x() - 60.0, startScenePos.y() - 40.0, 120.0, 80.0);
    } else {
        qreal left = qMin(startScenePos.x(), endScenePos.x());
        qreal top = qMin(startScenePos.y(), endScenePos.y());
        qreal w = qMax(10.0, qAbs(endScenePos.x() - startScenePos.x()));
        qreal h = qMax(10.0, qAbs(endScenePos.y() - startScenePos.y()));
        rect = QRectF(left, top, w, h);
    }

    Shape *shape = nullptr;
    switch (mode) {
    case Canvas::ToolMode::CreateRect:
        shape = new RectShape(rect.x(), rect.y(), rect.width(), rect.height());
        break;
    case Canvas::ToolMode::CreateEllipse:
        shape = new EllipseShape(rect.x(), rect.y(), rect.width(), rect.height());
        break;
    case Canvas::ToolMode::CreateDiamond:
        shape = new DiamondShape(rect.x(), rect.y(), rect.width(), rect.height());
        break;
    case Canvas::ToolMode::CreateLine:
        if (isClickCreate) {
            shape = new LineShape(startScenePos, startScenePos + QPointF(120.0, 0.0));
        } else {
            shape = new LineShape(startScenePos, endScenePos);
        }
        break;
    case Canvas::ToolMode::CreateArrow:
        if (isClickCreate) {
            shape = new ArrowShape(startScenePos, startScenePos + QPointF(120.0, 0.0));
        } else {
            shape = new ArrowShape(startScenePos, endScenePos);
        }
        break;
    case Canvas::ToolMode::CreateDualArrow:
        if (isClickCreate) {
            shape = new DualArrowShape(startScenePos, startScenePos + QPointF(120.0, 0.0));
        } else {
            shape = new DualArrowShape(startScenePos, endScenePos);
        }
        break;
    case Canvas::ToolMode::CreateText:
        shape = new TextLabel(rect.x(), rect.y(), "Text");
        if (!isClickCreate) {
            shape->setSize(rect.width(), rect.height());
        }
        break;
    default:
        break;
    }
    return shape;
}

bool CanvasController::handleMousePressEvent(QMouseEvent *event)
{
    if (!m_canvas || !event || event->button() != Qt::LeftButton) return false;
    const QPointF scenePos = m_canvas->mapToScene(event->pos());

    if (m_canvas->toolMode() != Canvas::ToolMode::Select && m_canvas->toolMode() != Canvas::ToolMode::Connect) {
        setState(InteractionState::CreatingShape);
        m_createStartScenePos = scenePos;
        if (m_rubberBandItem) {
            if (m_canvas->scene() && !m_rubberBandItem->scene()) {
                m_canvas->scene()->addItem(m_rubberBandItem);
            }
            m_rubberBandItem->setRect(QRectF(scenePos, QSizeF(0, 0)));
            m_rubberBandItem->setVisible(true);
        }
        event->accept();
        return true;
    }

    if (m_canvas->toolMode() == Canvas::ToolMode::Select) {
        Shape *clickedShape = findShapeAt(scenePos);
        if (clickedShape) {
            bool isCtrlPressed = (event->modifiers() & Qt::ControlModifier);
            if (isCtrlPressed) {
                if (m_selectedItems.contains(clickedShape)) {
                    deselectItem(clickedShape);
                } else {
                    selectItem(clickedShape, false);
                }
            } else {
                if (!m_selectedItems.contains(clickedShape)) {
                    selectItem(clickedShape, true);
                }
            }

            bool allLocked = true;
            for (Shape *s : m_selectedItems) {
                if (s && !s->isLocked()) {
                    allLocked = false;
                    break;
                }
            }

            if (!isCtrlPressed && !m_selectedItems.isEmpty() && !allLocked) {
                setState(InteractionState::MovingItems);
                m_dragStartScenePos = scenePos;
                m_dragStartPositions.clear();
                for (Shape *s : m_selectedItems) {
                    if (s && !s->isLocked()) m_dragStartPositions.insert(s, s->pos());
                }
            } else {
                setState(InteractionState::Idle);
            }
            event->accept();
            return true;
        } else {
            if (!(event->modifiers() & Qt::ControlModifier)) {
                clearSelection();
            }
        }
    }

    return false;
}

bool CanvasController::handleMouseMoveEvent(QMouseEvent *event)
{
    if (!m_canvas || !event) return false;
    const QPointF scenePos = m_canvas->mapToScene(event->pos());

    if (m_state == InteractionState::CreatingShape && m_rubberBandItem) {
        QRectF rect(m_createStartScenePos, scenePos);
        m_rubberBandItem->setRect(rect.normalized());
        event->accept();
        return true;
    }

    if (m_state == InteractionState::MovingItems && !m_selectedItems.isEmpty()) {
        const QPointF delta = scenePos - m_dragStartScenePos;
        for (Shape *s : m_selectedItems) {
            if (!s || s->isLocked() || !m_dragStartPositions.contains(s)) continue;
            s->setPosition(m_dragStartPositions.value(s) + delta);
        }
        if (m_controlBox && m_controlBox->isVisible()) {
            m_controlBox->updateHandlePositions();
        }
        event->accept();
        return true;
    }

    return false;
}

bool CanvasController::handleMouseReleaseEvent(QMouseEvent *event)
{
    if (!m_canvas || !event || event->button() != Qt::LeftButton) return false;
    const QPointF scenePos = m_canvas->mapToScene(event->pos());

    if (m_state == InteractionState::CreatingShape) {
        if (m_rubberBandItem) {
            m_rubberBandItem->setVisible(false);
        }
        Shape *newShape = createShapeInstance(m_createStartScenePos, scenePos);
        if (newShape && m_canvas->scene()) {
            m_canvas->scene()->addItem(newShape);
            selectItem(newShape, true);
            // TODO(Task 10b): 将创建图元包装为 AddShapeCommand
        }
        setState(InteractionState::Idle);
        event->accept();
        return true;
    }

    if (m_state == InteractionState::MovingItems) {
        setState(InteractionState::Idle);
        // TODO(Task 10b): 包装为 MoveItemsCommand
        m_dragStartPositions.clear();
        event->accept();
        return true;
    }

    return false;
}

bool CanvasController::handleKeyPressEvent(QKeyEvent *event)
{
    if (!event || m_selectedItems.isEmpty()) return false;
    // 拦截带有 Ctrl、Meta 或 Alt 的系统修饰键组合，避免与快捷键冲突
    if (event->modifiers() & (Qt::ControlModifier | Qt::MetaModifier | Qt::AltModifier)) {
        return false;
    }
    if (m_state != InteractionState::Idle && m_state != InteractionState::KeyMovingItems) return false;

    const int key = event->key();
    bool isMoveKey = (key == Qt::Key_W || key == Qt::Key_A || key == Qt::Key_S || key == Qt::Key_D ||
                      key == Qt::Key_Up || key == Qt::Key_Down || key == Qt::Key_Left || key == Qt::Key_Right);
    if (!isMoveKey) return false;

    if (m_state == InteractionState::Idle) {
        setState(InteractionState::KeyMovingItems);
        m_keyMoveStartPositions.clear();
        for (Shape *s : m_selectedItems) {
            if (s) m_keyMoveStartPositions.insert(s, s->pos());
        }
        m_keyMoveSpeed = 1.0;
        if (m_keyMoveTimer && !m_keyMoveTimer->isActive()) {
            m_keyMoveTimer->start();
        }
    }

    m_pressedKeys.insert(key);
    event->accept();
    return true;
}

bool CanvasController::handleKeyReleaseEvent(QKeyEvent *event)
{
    if (!event || m_state != InteractionState::KeyMovingItems) return false;

    const int key = event->key();
    m_pressedKeys.remove(key);

    if (m_pressedKeys.isEmpty()) {
        if (m_keyMoveTimer && m_keyMoveTimer->isActive()) {
            m_keyMoveTimer->stop();
        }
        setState(InteractionState::Idle);
        // TODO(Task 10b): 包装为 MoveItemsCommand
        m_keyMoveStartPositions.clear();
        m_keyMoveSpeed = 1.0;
    }
    event->accept();
    return true;
}

void CanvasController::onKeyMoveTick()
{
    if (m_state != InteractionState::KeyMovingItems || m_selectedItems.isEmpty() || m_pressedKeys.isEmpty()) {
        return;
    }

    QPointF direction(0.0, 0.0);
    if (m_pressedKeys.contains(Qt::Key_A) || m_pressedKeys.contains(Qt::Key_Left))  direction.rx() -= 1.0;
    if (m_pressedKeys.contains(Qt::Key_D) || m_pressedKeys.contains(Qt::Key_Right)) direction.rx() += 1.0;
    if (m_pressedKeys.contains(Qt::Key_W) || m_pressedKeys.contains(Qt::Key_Up))    direction.ry() -= 1.0;
    if (m_pressedKeys.contains(Qt::Key_S) || m_pressedKeys.contains(Qt::Key_Down))  direction.ry() += 1.0;

    if (direction.isNull()) return;

    qreal step = 1.0;
    if (m_canvas && m_canvas->zoomScale() > 0.001) {
        step = 1.0 / m_canvas->zoomScale();
    }
    if (QApplication::keyboardModifiers() & Qt::ShiftModifier) {
        step *= 10.0;
    }

    m_keyMoveSpeed = qMin(m_keyMoveSpeed + 0.15, 6.0);
    const QPointF delta = direction * (step * m_keyMoveSpeed);

    for (Shape *s : m_selectedItems) {
        if (s && !s->isLocked()) {
            s->setPosition(s->pos() + delta);
        }
    }
    if (m_controlBox && m_controlBox->isVisible()) {
        m_controlBox->updateHandlePositions();
    }
}

void CanvasController::cancelCurrentOperation()
{
    if (m_state == InteractionState::CreatingShape) {
        if (m_rubberBandItem) m_rubberBandItem->setVisible(false);
        setState(InteractionState::Idle);
    } else if (m_state == InteractionState::MovingItems) {
        for (Shape *s : m_selectedItems) {
            if (s && m_dragStartPositions.contains(s)) {
                s->setPosition(m_dragStartPositions.value(s));
            }
        }
        m_dragStartPositions.clear();
        if (m_controlBox && m_controlBox->isVisible()) m_controlBox->updateHandlePositions();
        setState(InteractionState::Idle);
    } else if (m_state == InteractionState::KeyMovingItems) {
        if (m_keyMoveTimer) m_keyMoveTimer->stop();
        for (Shape *s : m_selectedItems) {
            if (s && m_keyMoveStartPositions.contains(s)) {
                s->setPosition(m_keyMoveStartPositions.value(s));
            }
        }
        if (m_controlBox && m_controlBox->isVisible()) m_controlBox->updateHandlePositions();
        m_pressedKeys.clear();
        setState(InteractionState::Idle);
    } else {
        clearSelection();
    }
}

void CanvasController::copy()
{
}

void CanvasController::paste()
{
}

void CanvasController::cut()
{
    copy();
    deleteSelected();
}

void CanvasController::deleteSelected()
{
    if (m_selectedItems.isEmpty()) return;
    QList<Shape*> toDelete = m_selectedItems.values();
    clearSelection();
    for (Shape *s : toDelete) {
        if (s) {
            s->deleteLater();
        }
    }
}

void CanvasController::selectAll()
{
    if (!m_canvas || !m_canvas->scene()) return;
    clearSelection();
    for (QGraphicsItem *item : m_canvas->scene()->items()) {
        if (item == m_controlBox || item == m_rubberBandItem) continue;
        if (auto *shape = dynamic_cast<Shape*>(item)) {
            if (shape->isVisible()) {
                m_selectedItems.insert(shape);
                connect(shape, &QObject::destroyed, this, &CanvasController::onShapeDestroyed, Qt::UniqueConnection);
                shape->setSelected(true);
            }
        }
    }
    if (!m_selectedItems.isEmpty()) {
        m_primarySelection = *m_selectedItems.begin();
    }
    updateControlBoxTarget();
}
