#include "canvas.h"
#include "shapes/shape.h"
#include "shape_controller/connector_controller.h"
#include "shape_controller/canvas_controller.h"
#include "shape_controller/control_box.h"
#include "shape_controller/anchor_resolver.h"
#include "undo_manager.h"
#include <QScrollBar>

Canvas::Canvas(QWidget *parent)
    : QGraphicsView(parent)
    , m_scene(new QGraphicsScene(this))
    , m_connectorController(new ConnectorController(this))
    , m_canvasController(new CanvasController(this, this))
    , m_undoManager(new UndoManager(this))
{
    setScene(m_scene);
    if (m_canvasController && m_scene) {
        m_canvasController->attachToScene(m_scene);
    }

    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setRenderHint(QPainter::TextAntialiasing);

    setDragMode(QGraphicsView::RubberBandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
}

Canvas::~Canvas() = default;

void Canvas::clearScene()
{
    if (m_connectorController) {
        m_connectorController->cancelCurrentOperation();
        m_connectorController->destroySnapIndicator();
    }
    if (m_canvasController) {
        m_canvasController->cancelCurrentOperation();
    }
    if (m_scene) {
        // 仅删除用户图元（直接继承自 Shape 且没有父 QGraphicsItem 的顶层图元），
        // 绝不调用 m_scene->clear()，以保护 ControlBox、RubberBandItem 以及手柄等基础设施。
        QList<QGraphicsItem*> items = m_scene->items();
        for (QGraphicsItem *item : items) {
            if (!item->parentItem() && dynamic_cast<::Shape*>(item)) {
                delete item;
            }
        }
    }
}

bool Canvas::isLineCreationMode() const {
    return m_toolMode == ToolMode::Connect ||
           m_toolMode == ToolMode::CreateLine ||
           m_toolMode == ToolMode::CreateArrow ||
           m_toolMode == ToolMode::CreateDualArrow;
}

void Canvas::setToolMode(ToolMode mode)
{
    if (m_toolMode == mode) return;
    m_toolMode = mode;
    if (m_connectorController) {
        m_connectorController->setCreateModeActive(isLineCreationMode());
    }

    if (m_toolMode == ToolMode::Select) {
        setDragMode(QGraphicsView::RubberBandDrag);
        setCursor(Qt::ArrowCursor);
    } else {
        setDragMode(QGraphicsView::NoDrag);
        setCursor(Qt::CrossCursor);
    }
    emit toolModeChanged(m_toolMode);
    emit editModeChanged(m_toolMode);
}

void Canvas::zoomIn()
{
    applyZoom(1.15, viewport()->rect().center());
}

void Canvas::zoomOut()
{
    applyZoom(1.0 / 1.15, viewport()->rect().center());
}

void Canvas::resetZoom()
{
    if (qFuzzyCompare(m_zoomScale, 1.0)) return;
    resetTransform();
    m_zoomScale = 1.0;
    emit zoomScaleChanged(m_zoomScale);
}

void Canvas::fitToScene()
{
    if (!m_scene) return;
    QRectF rect = m_scene->itemsBoundingRect();
    if (rect.isEmpty()) return;
    fitInView(rect, Qt::KeepAspectRatio);
    m_zoomScale = transform().m11();
    emit zoomScaleChanged(m_zoomScale);
}

void Canvas::fitToSelection()
{
    if (!m_scene) return;
    QList<QGraphicsItem*> selected = m_scene->selectedItems();
    if (selected.isEmpty()) return;
    QRectF rect;
    for (QGraphicsItem *item : selected) {
        rect = rect.united(item->sceneBoundingRect());
    }
    if (rect.isEmpty()) return;
    fitInView(rect, Qt::KeepAspectRatio);
    m_zoomScale = transform().m11();
    emit zoomScaleChanged(m_zoomScale);
}

void Canvas::applyZoom(qreal scaleFactor, const QPoint &viewportAnchor)
{
    qreal targetScale = m_zoomScale * scaleFactor;
    if (targetScale < 0.1) {
        scaleFactor = 0.1 / m_zoomScale;
        targetScale = 0.1;
    } else if (targetScale > 10.0) {
        scaleFactor = 10.0 / m_zoomScale;
        targetScale = 10.0;
    }

    if (qFuzzyCompare(scaleFactor, 1.0)) return;

    auto oldAnchor = transformationAnchor();
    setTransformationAnchor(QGraphicsView::NoAnchor);

    QPointF oldScenePos = mapToScene(viewportAnchor);
    scale(scaleFactor, scaleFactor);
    m_zoomScale = targetScale;

    QPointF newScenePos = mapToScene(viewportAnchor);
    QPointF delta = newScenePos - oldScenePos;
    translate(delta.x(), delta.y());

    setTransformationAnchor(oldAnchor);
    emit zoomScaleChanged(m_zoomScale);
}

void Canvas::mousePressEvent(QMouseEvent *event)
{
    QPointF scenePos = mapToScene(event->pos());
    emit cursorScenePositionChanged(scenePos);

    if (event->button() == Qt::RightButton) {
        m_isPanning = true;
        m_lastPanPoint = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    if (m_toolMode == ToolMode::Select) {
        // 第一级分流：若点击处在控制点（HandleItem）或控制盒（ControlBox）范围，直接交由 QGraphicsView 原生分发，
        // 防止 CanvasController 的 findShapeAt 穿透控制点误把底层的 Shape 捕获并进入 MovingItems 状态。
        QGraphicsItem *curr = itemAt(event->pos());
        while (curr) {
            if (dynamic_cast<HandleItem*>(curr) || dynamic_cast<ControlBox*>(curr)) {
                QGraphicsView::mousePressEvent(event);
                return;
            }
            curr = curr->parentItem();
        }

        if (m_canvasController && m_canvasController->handleMousePressEvent(event)) {
            event->accept();
            return;
        }
        if (!itemAt(event->pos())) {
            m_isPanning = true;
            m_lastPanPoint = event->pos();
            setCursor(Qt::ClosedHandCursor);
            event->accept();
            return;
        }
        QGraphicsView::mousePressEvent(event);
        return;
    }

    if (isLineCreationMode()) {
        AnchorResolver::ResolveOptions options{scenePos, m_scene, (event->modifiers() & Qt::ControlModifier) != 0, 12.0, m_zoomScale, nullptr};
        auto resolveResult = AnchorResolver::resolve(options);
        if (resolveResult.mode() != ConnectorAnchor::Mode::Free) {
            if (m_connectorController && m_connectorController->handleMousePress(scenePos, event->button(), event->modifiers())) {
                event->accept();
                return;
            }
        } else {
            if (m_canvasController && m_canvasController->handleMousePressEvent(event)) {
                event->accept();
                return;
            }
        }
        event->accept();
        return;
    }

    if (m_canvasController && m_canvasController->handleMousePressEvent(event)) {
        event->accept();
        return;
    }

    QGraphicsView::mousePressEvent(event);
}

void Canvas::mouseMoveEvent(QMouseEvent *event)
{
    QPointF scenePos = mapToScene(event->pos());
    emit cursorScenePositionChanged(scenePos);

    if (m_isPanning) {
        QPoint delta = event->pos() - m_lastPanPoint;
        m_lastPanPoint = event->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        event->accept();
        return;
    }

    if (m_toolMode == ToolMode::Select) {
        if (m_canvasController && m_canvasController->handleMouseMoveEvent(event)) {
            event->accept();
            return;
        }
        QGraphicsView::mouseMoveEvent(event);
        return;
    }

    if (isLineCreationMode()) {
        if (m_connectorController && m_connectorController->state() != ConnectorController::State::Idle) {
            m_connectorController->handleMouseMove(scenePos, event->modifiers());
            event->accept();
            return;
        }
        if (m_canvasController && m_canvasController->handleMouseMoveEvent(event)) {
            event->accept();
            return;
        }
        event->accept();
        return;
    }

    if (m_canvasController && m_canvasController->handleMouseMoveEvent(event)) {
        event->accept();
        return;
    }

    QGraphicsView::mouseMoveEvent(event);
}

void Canvas::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_isPanning && (event->button() == Qt::RightButton || event->button() == Qt::LeftButton)) {
        m_isPanning = false;
        setCursor(m_toolMode == ToolMode::Select ? Qt::ArrowCursor : Qt::CrossCursor);
        event->accept();
        return;
    }

    if (m_toolMode == ToolMode::Select) {
        if (m_canvasController && m_canvasController->handleMouseReleaseEvent(event)) {
            event->accept();
            return;
        }
        QGraphicsView::mouseReleaseEvent(event);
        return;
    }

    if (isLineCreationMode()) {
        QPointF scenePos = mapToScene(event->pos());
        if (m_connectorController && m_connectorController->state() != ConnectorController::State::Idle) {
            m_connectorController->handleMouseRelease(scenePos, event->button(), event->modifiers());
            event->accept();
            return;
        }
        if (m_canvasController && m_canvasController->handleMouseReleaseEvent(event)) {
            event->accept();
            return;
        }
        event->accept();
        return;
    }

    if (m_canvasController && m_canvasController->handleMouseReleaseEvent(event)) {
        event->accept();
        return;
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void Canvas::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        qreal delta = event->angleDelta().y();
        if (delta != 0) {
            qreal scaleFactor = (delta > 0) ? 1.15 : (1.0 / 1.15);
            applyZoom(scaleFactor, event->position().toPoint());
        }
        event->accept();
        return;
    }
    QGraphicsView::wheelEvent(event);
}

void Canvas::keyPressEvent(QKeyEvent *event)
{
    if ((event->modifiers() & Qt::ControlModifier) && (event->key() == Qt::Key_Plus || event->key() == Qt::Key_Equal)) {
        zoomIn(); event->accept(); return;
    }
    if ((event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_Minus) {
        zoomOut(); event->accept(); return;
    }
    if ((event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_0) {
        resetZoom(); event->accept(); return;
    }
    if (event->key() == Qt::Key_Escape) {
        if (m_toolMode != ToolMode::Select) setToolMode(ToolMode::Select);
        if (m_connectorController) m_connectorController->cancelCurrentOperation();
        if (m_canvasController) m_canvasController->cancelCurrentOperation();
        event->accept(); return;
    }

    if ((event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_Z) {
        if (event->modifiers() & Qt::ShiftModifier) {
            if (m_undoManager) m_undoManager->redo();
        } else {
            if (m_undoManager) m_undoManager->undo();
        }
        event->accept(); return;
    }
    if ((event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_Y) {
        if (m_undoManager) m_undoManager->redo();
        event->accept(); return;
    }

    if (m_canvasController) {
        if ((event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_C) {
            m_canvasController->copy(); event->accept(); return;
        }
        if ((event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_V) {
            m_canvasController->paste(); event->accept(); return;
        }
        if ((event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_X) {
            m_canvasController->cut(); event->accept(); return;
        }
        if ((event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_A) {
            m_canvasController->selectAll(); event->accept(); return;
        }
        if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
            m_canvasController->deleteSelected(); event->accept(); return;
        }
        if (m_canvasController->handleKeyPressEvent(event)) {
            return;
        }
    }

    QGraphicsView::keyPressEvent(event);
}

void Canvas::keyReleaseEvent(QKeyEvent *event)
{
    if (m_canvasController && m_canvasController->handleKeyReleaseEvent(event)) {
        return;
    }
    QGraphicsView::keyReleaseEvent(event);
}