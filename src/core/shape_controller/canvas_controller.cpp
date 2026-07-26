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
#include "shapes/connector/connector.h"
#include "drawing_constraints.h"

#include "core/commands/undo_commands.h"
#include "core/undo_manager.h"
#include "export/scene_export_utils.h"

#include <QMouseEvent>
#include <QKeyEvent>
#include <QGraphicsScene>
#include <QPen>
#include <QBrush>
#include <QApplication>
#include <cmath>
#include <algorithm>
#include <limits>

CanvasController::CanvasController(Canvas *canvas, QObject *parent)
    : QObject(parent), m_canvas(canvas)
{
    m_controlBox = new ControlBox();
    m_controlBox->setData(SceneExport::ExcludeFromExportRole, true);
    m_controlBox->setZValue(99999.0);
    m_controlBox->setVisible(false);

    m_rubberBandItem = new RubberBandItem();
    m_rubberBandItem->setData(SceneExport::ExcludeFromExportRole, true);
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
        m_controlBox->setData(SceneExport::ExcludeFromExportRole, true);
        m_controlBox->setZValue(99999.0);
        m_controlBox->setVisible(false);
    }
    if (!m_rubberBandItem) {
        m_rubberBandItem = new RubberBandItem();
        m_rubberBandItem->setData(SceneExport::ExcludeFromExportRole, true);
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
    default:
        break;
    }
    if (shape) {
        // 设置默认属性：有边框（默认宽度2.0，实线，深冷灰），无填充（透明背景）
        shape->setBorderInfo(Border(2.0, QColor("#303133"), Qt::SolidLine));
        shape->setFillInfo(FillStyle(Qt::transparent, 1.0));
    }
    return shape;
}

void CanvasController::updatePreviewShape(const QPointF &scenePos,
                                             Qt::KeyboardModifiers modifiers)
{
    if (!m_previewShape) return;

    const bool constrain = modifiers.testFlag(Qt::ShiftModifier);
    if (LineShape *lineShape = dynamic_cast<LineShape *>(m_previewShape.data())) {
        const QPointF endPoint = constrain
            ? DrawingConstraints::constrainLineEndpoint(m_createStartScenePos, scenePos)
            : scenePos;
        lineShape->setEndpoints(m_createStartScenePos, endPoint);
        return;
    }

    // Shift 仅约束真正的几何图形；TextLabel 的尺寸由文本内容/固定文本框语义决定，
    // 不把它误变成正方形文本框。
    const Canvas::ToolMode mode = m_canvas ? m_canvas->toolMode() : Canvas::ToolMode::Select;
    const bool constrainShape = constrain &&
        (mode == Canvas::ToolMode::CreateRect ||
         mode == Canvas::ToolMode::CreateEllipse ||
         mode == Canvas::ToolMode::CreateDiamond);

    QRectF rect = DrawingConstraints::makeDragRect(m_createStartScenePos, scenePos, constrainShape);
    if (rect.width() < 1.0) rect.setWidth(1.0);
    if (rect.height() < 1.0) rect.setHeight(1.0);

    // Shape::setSize() 保持场景中心不变并可能调整 pos()；先应用尺寸，再校准左上角，
    // 这样预览不会因为中心保持策略在鼠标移动时跳动。
    m_previewShape->setSize(rect.size());
    m_previewShape->setPosition(rect.topLeft());
}

bool CanvasController::handleMousePressEvent(QMouseEvent *event)
{
    if (!m_canvas || !event || event->button() != Qt::LeftButton) return false;
    const QPointF scenePos = m_canvas->mapToScene(event->pos());

    if (m_canvas->toolMode() != Canvas::ToolMode::Select && m_canvas->toolMode() != Canvas::ToolMode::BoxSelect && m_canvas->toolMode() != Canvas::ToolMode::Connect) {
        setState(InteractionState::CreatingShape);
        m_createStartScenePos = scenePos;
        m_previewShape = createShapeInstance(scenePos, scenePos);
        if (m_previewShape && m_canvas->scene()) {
            m_canvas->scene()->addItem(m_previewShape);
        }
        event->accept();
        return true;
    }

    if (m_canvas->toolMode() == Canvas::ToolMode::Select || m_canvas->toolMode() == Canvas::ToolMode::BoxSelect) {
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
            if (m_canvas->toolMode() == Canvas::ToolMode::BoxSelect) {
                setState(InteractionState::BoxSelecting);
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
        }
    }

    return false;
}

bool CanvasController::handleMouseMoveEvent(QMouseEvent *event)
{
    if (!m_canvas || !event) return false;
    const QPointF scenePos = m_canvas->mapToScene(event->pos());

    if (m_state == InteractionState::CreatingShape) {
        updatePreviewShape(scenePos, event->modifiers());
        event->accept();
        return true;
    }

    if (m_state == InteractionState::BoxSelecting && m_rubberBandItem) {
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

    if (m_state == InteractionState::CreatingShape) {
        const QPointF scenePos = m_canvas->mapToScene(event->pos());
        updatePreviewShape(scenePos, event->modifiers());
        if (m_previewShape) {
            if (LineShape* l = dynamic_cast<LineShape*>(m_previewShape.data())) {
                if ((l->getStartPoint() - l->getEndPoint()).manhattanLength() < 5.0) {
                    l->setEndPoint(l->getStartPoint() + QPointF(120.0, 0.0));
                }
            } else {
                QSizeF sz = m_previewShape->getSize();
                if (sz.width() < 5.0 && sz.height() < 5.0) {
                    const bool constrain = event->modifiers().testFlag(Qt::ShiftModifier);
                    const qreal defaultSide = constrain ? 100.0 : 0.0;
                    m_previewShape->setSize(constrain ? defaultSide : 100.0,
                                             constrain ? defaultSide : 60.0);
                }
            }
            Shape *createdShape = m_previewShape.data();
            selectItem(createdShape, true);
            m_previewShape = nullptr;
            if (createdShape && m_canvas && m_canvas->undoManager())
                m_canvas->undoManager()->push(
                    new CreateShapeCommand(createdShape, m_canvas->scene()));
        }
        // 创建完成后保持当前绘图工具激活，允许用户连续创建同类图形。
        // 只有用户主动选择 SELECT/其他工具时，才应离开当前绘图模式。
        setState(InteractionState::Idle);
        event->accept();
        return true;
    }

    if (m_state == InteractionState::BoxSelecting) {
        if (m_rubberBandItem) {
            m_rubberBandItem->setVisible(false);
            if (m_canvas && m_canvas->scene()) {
                QRectF selRect = m_rubberBandItem->rect();
                QList<QGraphicsItem*> foundItems = m_canvas->scene()->items(selRect, Qt::IntersectsItemShape);
                if (!(event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier))) {
                    clearSelection();
                }
                for (QGraphicsItem* item : foundItems) {
                    if (Shape* s = dynamic_cast<Shape*>(item)) {
                        selectItem(s, false);
                    }
                }
            }
        }
        setState(InteractionState::Idle);
        event->accept();
        return true;
    }

    if (m_state == InteractionState::MovingItems) {
        setState(InteractionState::Idle);
        if (m_canvas && m_canvas->undoManager()) {
            QList<MoveItemsCommand::MoveData> moves;
            for (auto it = m_dragStartPositions.constBegin(); it != m_dragStartPositions.constEnd(); ++it) {
                Shape* shape = it.key();
                if (shape) {
                    QPointF oldPos = it.value();
                    QPointF newPos = shape->pos();
                    if (!qFuzzyCompare(oldPos.x(), newPos.x()) || !qFuzzyCompare(oldPos.y(), newPos.y())) {
                        moves.append({shape, oldPos, newPos});
                    }
                }
            }
            if (!moves.isEmpty()) {
                m_canvas->undoManager()->push(new MoveItemsCommand(moves));
            }
        }
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
        if (m_canvas && m_canvas->undoManager()) {
            QList<MoveItemsCommand::MoveData> moves;
            for (auto it = m_keyMoveStartPositions.constBegin(); it != m_keyMoveStartPositions.constEnd(); ++it) {
                Shape* shape = it.key();
                if (shape) {
                    QPointF oldPos = it.value();
                    QPointF newPos = shape->pos();
                    if (!qFuzzyCompare(oldPos.x(), newPos.x()) || !qFuzzyCompare(oldPos.y(), newPos.y())) {
                        moves.append({shape, oldPos, newPos});
                    }
                }
            }
            if (!moves.isEmpty()) {
                m_canvas->undoManager()->push(new MoveItemsCommand(moves));
            }
        }
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
        if (m_previewShape && m_canvas && m_canvas->scene()) {
            m_canvas->scene()->removeItem(m_previewShape);
            delete m_previewShape;
            m_previewShape = nullptr;
        }
        if (m_rubberBandItem) m_rubberBandItem->setVisible(false);
        setState(InteractionState::Idle);
    } else if (m_state == InteractionState::BoxSelecting) {
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
    m_shapeClipboard.clear();
    m_pasteGeneration = 0;

    QList<Shape *> ordered = m_selectedItems.values();
    std::sort(ordered.begin(), ordered.end(), [](const Shape *left, const Shape *right) {
        return left && right && left->getLayer() < right->getLayer();
    });
    for (Shape *shape : ordered) {
        if (shape)
            m_shapeClipboard.emplace_back(shape->clone());
    }
}

void CanvasController::paste()
{
    if (m_shapeClipboard.empty() || !m_canvas || !m_canvas->scene() ||
        !m_canvas->undoManager())
        return;

    ++m_pasteGeneration;
    const QPointF offset(20.0 * m_pasteGeneration,
                         20.0 * m_pasteGeneration);
    auto *transaction = new QUndoCommand(QStringLiteral("Paste Items"));
    QList<Shape *> pastedShapes;

    for (const std::unique_ptr<Shape> &prototype : m_shapeClipboard) {
        if (!prototype)
            continue;
        Shape *copyShape = prototype->clone();
        if (auto *line = dynamic_cast<LineShape *>(copyShape)) {
            line->setEndpoints(line->getStartPoint() + offset,
                               line->getEndPoint() + offset,
                               ApplyMode::HistoryReplay);
        } else if (auto *connector = dynamic_cast<Connector *>(copyShape)) {
            connector->setStartAnchor(ConnectorAnchor::createFree(
                                          connector->getStartAnchor().resolveScenePoint() + offset),
                                      ApplyMode::HistoryReplay);
            connector->setEndAnchor(ConnectorAnchor::createFree(
                                        connector->getEndAnchor().resolveScenePoint() + offset),
                                    ApplyMode::HistoryReplay);
        } else {
            copyShape->setPosition(copyShape->getPosition() + offset,
                                   ApplyMode::HistoryReplay);
        }
        new CreateShapeCommand(copyShape, m_canvas->scene(), transaction);
        pastedShapes.append(copyShape);
    }

    if (pastedShapes.isEmpty()) {
        delete transaction;
        return;
    }

    m_canvas->undoManager()->push(transaction);
    clearSelection();
    for (Shape *shape : pastedShapes)
        selectItem(shape, false);
}

void CanvasController::cut()
{
    copy();
    deleteSelected();
}

void CanvasController::deleteSelected()
{
    if (m_selectedItems.isEmpty() || !m_canvas || !m_canvas->scene())
        return;
    const QList<Shape *> toDelete = m_selectedItems.values();
    clearSelection();
    if (m_canvas->undoManager())
        m_canvas->undoManager()->push(
            new DeleteItemsCommand(toDelete, m_canvas->scene()));
}

void CanvasController::bringSelectedToFront()
{
    if (m_selectedItems.isEmpty() || !m_canvas || !m_canvas->scene() ||
        !m_canvas->undoManager())
        return;

    QList<Shape *> shapes = m_selectedItems.values();
    shapes.erase(std::remove(shapes.begin(), shapes.end(), nullptr), shapes.end());
    if (shapes.isEmpty())
        return;

    int maximumLayer = shapes.first()->getLayer();
    for (QGraphicsItem *item : m_canvas->scene()->items()) {
        if (auto *shape = dynamic_cast<Shape *>(item); shape && !shape->parentItem())
            maximumLayer = std::max(maximumLayer, shape->getLayer());
    }

    std::sort(shapes.begin(), shapes.end(), [](const Shape *left, const Shape *right) {
        return left->getLayer() < right->getLayer();
    });
    const qint64 firstLayer = static_cast<qint64>(maximumLayer) + 1;
    const qint64 lastLayer = firstLayer + static_cast<qint64>(shapes.size()) - 1;
    if (lastLayer > std::numeric_limits<int>::max())
        return;

    auto *transaction = new QUndoCommand(QStringLiteral("Bring Shapes to Front"));
    int nextLayer = static_cast<int>(firstLayer);
    for (Shape *shape : shapes) {
        new ShapePropertyCommand(shape, ShapePropertyCommand::Property::Layer,
                                 shape->getLayer(), nextLayer++,
                                 QStringLiteral("Bring to Front"), transaction);
    }
    m_canvas->undoManager()->push(transaction);
}

void CanvasController::sendSelectedToBack()
{
    if (m_selectedItems.isEmpty() || !m_canvas || !m_canvas->scene() ||
        !m_canvas->undoManager())
        return;

    QList<Shape *> shapes = m_selectedItems.values();
    shapes.erase(std::remove(shapes.begin(), shapes.end(), nullptr), shapes.end());
    if (shapes.isEmpty())
        return;

    int minimumLayer = shapes.first()->getLayer();
    for (QGraphicsItem *item : m_canvas->scene()->items()) {
        if (auto *shape = dynamic_cast<Shape *>(item); shape && !shape->parentItem())
            minimumLayer = std::min(minimumLayer, shape->getLayer());
    }

    std::sort(shapes.begin(), shapes.end(), [](const Shape *left, const Shape *right) {
        return left->getLayer() < right->getLayer();
    });
    const qint64 firstLayer = static_cast<qint64>(minimumLayer)
        - static_cast<qint64>(shapes.size());
    if (firstLayer < std::numeric_limits<int>::min())
        return;

    auto *transaction = new QUndoCommand(QStringLiteral("Send Shapes to Back"));
    int nextLayer = static_cast<int>(firstLayer);
    for (Shape *shape : shapes) {
        new ShapePropertyCommand(shape, ShapePropertyCommand::Property::Layer,
                                 shape->getLayer(), nextLayer++,
                                 QStringLiteral("Send to Back"), transaction);
    }
    m_canvas->undoManager()->push(transaction);
}

void CanvasController::clearAllItems()
{
    if (!m_canvas || !m_canvas->scene() || !m_canvas->undoManager())
        return;

    QList<Shape *> shapes;
    for (QGraphicsItem *item : m_canvas->scene()->items()) {
        if (auto *shape = dynamic_cast<Shape *>(item); shape && !shape->parentItem())
            shapes.append(shape);
    }
    if (shapes.isEmpty())
        return;

    cancelCurrentOperation();
    clearSelection();
    m_canvas->undoManager()->push(new DeleteItemsCommand(shapes, m_canvas->scene()));
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

void CanvasController::onResizeFinished(Shape* target, QSizeF oldSize, QSizeF newSize, QPointF oldPos, QPointF newPos)
{
    if (!target || !m_canvas || !m_canvas->undoManager()) return;
    if (oldSize != newSize || oldPos != newPos) {
        m_canvas->undoManager()->push(new ResizeItemCommand(target, oldSize, newSize, oldPos, newPos));
    }
}

void CanvasController::onRotateFinished(Shape* target, qreal oldRotation, qreal newRotation)
{
    if (!target || !m_canvas || !m_canvas->undoManager()) return;
    if (!qFuzzyCompare(oldRotation, newRotation)) {
        m_canvas->undoManager()->push(new RotateItemCommand(target, oldRotation, newRotation));
    }
}

void CanvasController::onEndpointMoveFinished(Shape* target, HandleType type, QPointF oldScenePos, QPointF newScenePos)
{
    Q_UNUSED(newScenePos);
    if (!target || !m_canvas || !m_canvas->undoManager()) return;

    if (auto* line = dynamic_cast<LineShape*>(target)) {
        QPointF finalStart = line->getStartPoint();
        QPointF finalEnd = line->getEndPoint();

        QPointF oldStart = finalStart;
        QPointF oldEnd = finalEnd;

        if (type == HandleType::StartEndpoint) {
            oldStart = oldScenePos;
        } else if (type == HandleType::EndEndpoint) {
            oldEnd = oldScenePos;
        }

        if (!qFuzzyCompare(oldStart.x(), finalStart.x()) || !qFuzzyCompare(oldStart.y(), finalStart.y()) ||
            !qFuzzyCompare(oldEnd.x(), finalEnd.x()) || !qFuzzyCompare(oldEnd.y(), finalEnd.y())) {
            m_canvas->undoManager()->push(new MoveEndpointCommand(line, oldStart, oldEnd, finalStart, finalEnd));
        }
    }
}

void CanvasController::onConnectorEndpointMoveFinished(Connector *target, HandleType endpoint, const ConnectorAnchor &oldAnchor, const ConnectorAnchor &newAnchor)
{
    if (!target || !m_canvas || !m_canvas->undoManager()) return;

    if (oldAnchor != newAnchor) {
        ModifyConnectorCommand::Endpoint ep = (endpoint == HandleType::StartEndpoint) ?
            ModifyConnectorCommand::Endpoint::Start : ModifyConnectorCommand::Endpoint::End;
        m_canvas->undoManager()->push(new ModifyConnectorCommand(target, ep, oldAnchor, newAnchor));
    }
}
