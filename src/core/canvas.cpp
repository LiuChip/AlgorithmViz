#include "canvas.h"
#include <QVarLengthArray>
#include "shapes/shape.h"
#include "shape_controller/connector_controller.h"
#include "shape_controller/canvas_controller.h"
#include "shape_controller/control_box.h"
#include "shape_controller/anchor_resolver.h"
#include "undo_manager.h"
#include <QScrollBar>
#include <QMenu>
#include <QAction>
#include <QPainter>
#include <QtMath>

namespace {
// 空场景不能依赖 QGraphicsScene 根据图元自动推导 sceneRect。
// 否则第一条图元加入场景时，QGraphicsView 的视图映射会发生跳变，
// 从而导致首次绘制的起点或预览看起来发生偏移。
constexpr qreal kDefaultSceneWidth = 10000.0;
constexpr qreal kDefaultSceneHeight = 10000.0;
}

Canvas::Canvas(QWidget *parent)
    : QGraphicsView(parent)
    , m_scene(new QGraphicsScene(this))
    , m_connectorController(new ConnectorController(this))
    , m_canvasController(new CanvasController(this, this))
    , m_undoManager(new UndoManager(this))
{
    // 预先固定一个稳定的逻辑画布范围，避免空场景在加入第一条图元时
    // 自动收缩/扩展 sceneRect，进而改变 viewport 到 scene 的坐标映射。
    // 文档加载时如果包含自己的 sceneRect，加载逻辑仍可覆盖这个默认值。
    m_scene->setSceneRect(QRectF(0.0, 0.0, kDefaultSceneWidth, kDefaultSceneHeight));
    setScene(m_scene);
    // QGraphicsScene 是选择状态的唯一事实来源。无论选择来自画布点击、
    // 图形列表还是程序化恢复，都统一转发给属性面板等上层 UI。
    connect(m_scene, &QGraphicsScene::selectionChanged,
            this, &Canvas::selectionChanged);
    if (m_canvasController && m_scene) {
        m_canvasController->attachToScene(m_scene);
    }

    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setRenderHint(QPainter::TextAntialiasing);

    setDragMode(QGraphicsView::RubberBandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
}

Canvas::~Canvas()
{
    // m_scene 是 QObject 子对象，会在 Canvas 的派生析构结束后才被 Qt 清理；
    // 清理选中图元可能再次触发 selectionChanged，因此必须提前断开信号转发。
    if (m_scene)
        disconnect(m_scene, &QGraphicsScene::selectionChanged,
                   this, &Canvas::selectionChanged);
}

void Canvas::clearScene()
{
    if (m_connectorController) {
        m_connectorController->cancelCurrentOperation();
        m_connectorController->destroySnapIndicator();
    }
    if (m_canvasController) {
        m_canvasController->cancelCurrentOperation();
    }
    if (m_undoManager) {
        m_undoManager->clear();
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

    if (m_toolMode == ToolMode::Select || m_toolMode == ToolMode::BoxSelect) {
        setDragMode(QGraphicsView::NoDrag);
        setCursor(Qt::ArrowCursor);
        if (viewport()) viewport()->setCursor(Qt::ArrowCursor);
    } else {
        setDragMode(QGraphicsView::NoDrag);
        setCursor(Qt::CrossCursor);
        if (viewport()) viewport()->setCursor(Qt::CrossCursor);
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

void Canvas::setGridVisible(bool visible)
{
    if (m_gridVisible == visible)
        return;
    m_gridVisible = visible;
    if (viewport())
        viewport()->update();
    emit gridVisibilityChanged(m_gridVisible);
}

void Canvas::drawBackground(QPainter *painter, const QRectF &rect)
{
    painter->fillRect(rect, QColor(QStringLiteral("#fafbfc")));
    if (!m_gridVisible || !qIsFinite(m_gridSpacing) || m_gridSpacing <= 0.0)
        return;

    const qreal firstX = qFloor(rect.left() / m_gridSpacing) * m_gridSpacing;
    const qreal firstY = qFloor(rect.top() / m_gridSpacing) * m_gridSpacing;
    QVarLengthArray<QPointF, 512> points;
    for (qreal x = firstX; x <= rect.right(); x += m_gridSpacing) {
        for (qreal y = firstY; y <= rect.bottom(); y += m_gridSpacing)
            points.append(QPointF(x, y));
    }

    painter->save();
    painter->setPen(QPen(QColor(QStringLiteral("#d8dee9")), 0.0));
    painter->drawPoints(points.constData(), static_cast<int>(points.size()));
    painter->restore();
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

void Canvas::showContextMenu(const QPoint &pos) {
    QPointF scenePos = mapToScene(pos);
    QGraphicsItem *item = scene()->itemAt(scenePos, transform());

    ::Shape* hitShape = nullptr;
    while (item) {
        if (auto s = dynamic_cast<::Shape*>(item)) {
            hitShape = s;
            break;
        }
        item = item->parentItem();
    }

    QMenu menu(this);
    if (hitShape) {
        if (m_canvasController && !hitShape->isSelected()) {
            scene()->clearSelection();
            hitShape->setSelected(true);
        }

        QAction* frontAct = menu.addAction("置于顶层 (Bring to Front)");
        QAction* backAct = menu.addAction("置于底层 (Send to Back)");
        menu.addSeparator();
        QAction* delAct = menu.addAction("删除 (Delete)");

        QAction* selected = menu.exec(mapToGlobal(pos));
        if (selected == frontAct) {
            bringSelectedToFront();
        } else if (selected == backAct) {
            sendSelectedToBack();
        } else if (selected == delAct) {
            deleteSelected();
        }
    } else {
        QAction* selAllAct = menu.addAction("全选 (Select All)");
        QAction* clearAct = menu.addAction("清空画布 (Clear Scene)");

        QAction* selected = menu.exec(mapToGlobal(pos));
        if (selected == selAllAct) {
            selectAll();
        } else if (selected == clearAct) {
            clearAllItems();
        }
    }
}

void Canvas::mousePressEvent(QMouseEvent *event)
{
    QPointF scenePos = mapToScene(event->pos());
    emit cursorScenePositionChanged(scenePos);

    if (event->button() == Qt::RightButton) {
        m_isPanning = true;
        m_lastPanPoint = event->pos();
        m_panStartPoint = event->pos();
        setCursor(Qt::ClosedHandCursor);
        if (viewport()) viewport()->setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    if (m_toolMode == ToolMode::Select || m_toolMode == ToolMode::BoxSelect) {
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
        if (m_toolMode == ToolMode::Select && !itemAt(event->pos())) {
            m_isPanning = true;
            m_lastPanPoint = event->pos();
            setCursor(Qt::ClosedHandCursor);
            if (viewport()) viewport()->setCursor(Qt::ClosedHandCursor);
            event->accept();
            return;
        }
        QGraphicsView::mousePressEvent(event);
        return;
    }

    if (isLineCreationMode()) {
        if (m_connectorController && m_connectorController->handleMousePress(scenePos, event->button(), event->modifiers())) {
            event->accept();
            return;
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

    if (m_toolMode == ToolMode::Select || m_toolMode == ToolMode::BoxSelect) {
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
        Qt::CursorShape targetCursor = (m_toolMode == ToolMode::Select || m_toolMode == ToolMode::BoxSelect) ? Qt::ArrowCursor : Qt::CrossCursor;
        setCursor(targetCursor);
        if (viewport()) viewport()->setCursor(targetCursor);

        if (event->button() == Qt::RightButton) {
            if ((event->pos() - m_panStartPoint).manhattanLength() < 5) {
                showContextMenu(event->pos());
            }
        }

        event->accept();
        return;
    }

    if (m_toolMode == ToolMode::Select || m_toolMode == ToolMode::BoxSelect) {
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
        // ESC 取消当前未完成的交互，并保留原有的返回 SELECT 逻辑。
        // 完成一次绘制后仍会保持绘图工具，只有用户完成绘制或按 ESC 时才返回选择模式。
        if (m_connectorController) m_connectorController->cancelCurrentOperation();
        if (m_canvasController) m_canvasController->cancelCurrentOperation();
        if (m_toolMode != ToolMode::Select) setToolMode(ToolMode::Select);
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
