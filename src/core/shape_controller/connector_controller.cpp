#include "connector_controller.h"
#include "core/canvas.h"
#include "core/undo_manager.h"
#include "core/commands/undo_commands.h"
#include "drawing_constraints.h"
#include <cmath>

ConnectorController::ConnectorController(Canvas* canvas, QObject* parent):QObject(parent),m_canvas(canvas)
{
    if(m_canvas)
    {
        m_canvas->setMouseTracking(true);
        connect(m_canvas, &QObject::destroyed, this, [this]() {
            cancelCurrentOperation();
            destroySnapIndicator();
        });
    }
}

ConnectorController::~ConnectorController()
{
    cancelCurrentOperation();
    destroySnapIndicator();
}

void ConnectorController::destroySnapIndicator()
{
    if(m_indicatorScene && !m_snapIndicator.isNull())
    {
        m_indicatorScene->removeItem(m_snapIndicator);
        delete m_snapIndicator;
    }
    m_snapIndicator = nullptr;
    m_indicatorScene = nullptr;
}

void ConnectorController::resetOperationState()
{
    m_activeConnector = nullptr;
    m_activeEndpointType = EndpointType::None;
    hideSnapIndicator();
    m_state = State::Idle;
}

void ConnectorController::abortCreating()
{
    // [WARNING]: abortCreating() 直接执行 delete，只能用于尚未进入画布、未提交到
    // Undo 栈历史的临时 Connector。绝对不可用于已在场景或历史快照中的连线，
    // 否则会造成 QUndoStack 引用悬空指针，引发应用崩溃。
    if(m_activeConnector)
    {
        delete m_activeConnector;
        m_activeConnector = nullptr;
    }
    hideSnapIndicator();
    m_activeEndpointType = EndpointType::None;
    m_state = State::Idle;
}

void ConnectorController::setCreateModeActive(bool active)
{
    if(m_createModeActive == active) return;
    m_createModeActive = active;
    if(!active && m_state != State::Idle)
    {
        cancelCurrentOperation();
    }
}

void ConnectorController::startDraggingEndpoint(Connector* connector, EndpointType endpoint, const QPointF& scenePos, Qt::KeyboardModifiers modifiers)
{
    if(!connector || connector->isLocked() || endpoint == EndpointType::None) return;
    m_activeConnector = connector;
    m_activeEndpointType = endpoint;
    if(endpoint == EndpointType::Start)
    {
        m_backupAnchor = connector->getStartAnchor();
    }
    else if(endpoint == EndpointType::End)
    {
        m_backupAnchor = connector->getEndAnchor();
    }
    m_state = State::DraggingEndpoint;
    updateDraggingEndpoint(scenePos, modifiers);
}

void ConnectorController::updateDraggingEndpoint(const QPointF& scenePos, Qt::KeyboardModifiers modifiers)
{
    if(!m_activeConnector || m_activeConnector->isLocked() || m_activeEndpointType == EndpointType::None)
    {
        resetOperationState();
        return;
    }

    const QPointF fixedPoint = (m_activeEndpointType == EndpointType::Start)
        ? m_activeConnector->getEndAnchor().resolveScenePoint()
        : m_activeConnector->getStartAnchor().resolveScenePoint();
    const QPointF effectiveScenePos = modifiers.testFlag(Qt::ShiftModifier)
        ? DrawingConstraints::constrainLineEndpoint(fixedPoint, scenePos)
        : scenePos;

    ConnectorAnchor newAnchor;
    if(modifiers.testFlag(Qt::ControlModifier))
    {
        newAnchor = ConnectorAnchor::createFree(effectiveScenePos);
    }
    else
    {
        AnchorResolver::ResolveOptions options;
        options.scenePoint = effectiveScenePos;
        options.scene = m_canvas ? m_canvas->scene() : nullptr;
        options.ctrlPressed = false;
        options.viewScale = getSafeViewScale();
        options.ignoreShape = nullptr;
        newAnchor = AnchorResolver::resolve(options);
    }
    if(m_activeEndpointType == EndpointType::Start)
    {
        if(!m_activeConnector->setStartAnchor(newAnchor)) {
            resetOperationState();
            return;
        }
    }
    else if(m_activeEndpointType == EndpointType::End)
    {
        if(!m_activeConnector->setEndAnchor(newAnchor)) {
            resetOperationState();
            return;
        }
    }
    updateSnapIndicator(newAnchor);
}

void ConnectorController::finishDraggingEndpoint(const QPointF& scenePos, Qt::KeyboardModifiers modifiers)
{
    if(!m_activeConnector || m_activeConnector->isLocked() || m_activeEndpointType == EndpointType::None)
    {
        resetOperationState();
        return;
    }
    const QPointF fixedPoint = (m_activeEndpointType == EndpointType::Start)
        ? m_activeConnector->getEndAnchor().resolveScenePoint()
        : m_activeConnector->getStartAnchor().resolveScenePoint();
    const QPointF effectiveScenePos = modifiers.testFlag(Qt::ShiftModifier)
        ? DrawingConstraints::constrainLineEndpoint(fixedPoint, scenePos)
        : scenePos;

    AnchorResolver::ResolveOptions options;
    options.scenePoint = effectiveScenePos;
    options.scene = m_canvas ? m_canvas->scene() : nullptr;
    options.ctrlPressed = modifiers.testFlag(Qt::ControlModifier);
    options.viewScale = getSafeViewScale();
    options.ignoreShape = nullptr;
    ConnectorAnchor finalAnchor = AnchorResolver::resolve(options);
    if (finalAnchor.mode() == ConnectorAnchor::Mode::Free) {
        cancelCurrentOperation();
        return;
    }
    if (m_activeEndpointType == EndpointType::Start) {
        if(!m_activeConnector->setStartAnchor(finalAnchor)) {
            resetOperationState();
            return;
        }
    } else if (m_activeEndpointType == EndpointType::End) {
        if(!m_activeConnector->setEndAnchor(finalAnchor)) {
            resetOperationState();
            return;
        }
    }

    QPointF startPt = m_activeConnector->getStartAnchor().resolveScenePoint();
    QPointF endPt = m_activeConnector->getEndAnchor().resolveScenePoint();
    qreal len = QLineF(startPt, endPt).length();

    if (len < 2.0 / getSafeViewScale()) {
        // Revert to backup anchor if the new length is too short.
        if (m_activeEndpointType == EndpointType::Start) {
            m_activeConnector->setStartAnchor(m_backupAnchor);
        } else {
            m_activeConnector->setEndAnchor(m_backupAnchor);
        }
    } else {
        if (m_canvas && m_canvas->undoManager()) {
            ModifyConnectorCommand::Endpoint ep = (m_activeEndpointType == EndpointType::Start) ? ModifyConnectorCommand::Endpoint::Start : ModifyConnectorCommand::Endpoint::End;
            m_canvas->undoManager()->push(new ModifyConnectorCommand(m_activeConnector, ep, m_backupAnchor, finalAnchor));
        }
        emit connectorModified(m_activeConnector);
    }

    resetOperationState();
}

void ConnectorController::cancelCurrentOperation()
{
    if(m_state == State::Idle) return;
    if(m_state == State::Creating)
    {
        if(m_activeConnector) {
            delete m_activeConnector;
        }
    }
    else if(m_state == State::DraggingEndpoint)
    {
        if(m_activeConnector && !m_activeConnector->isLocked())
        {
            if(m_activeEndpointType == EndpointType::Start)
            {
                m_activeConnector->setStartAnchor(m_backupAnchor);
            }
            else if(m_activeEndpointType == EndpointType::End)
            {
                m_activeConnector->setEndAnchor(m_backupAnchor);
            }
        }
    }
    resetOperationState();
}

qreal ConnectorController::getSafeViewScale() const
{
    if (!m_canvas) return 1.0;
    const qreal scale = m_canvas->transform().m11();
    if (!std::isfinite(scale) || scale <= 1e-6) return 1.0;
    return scale;
}

bool ConnectorController::isSelfConnection(const ConnectorAnchor& start, const ConnectorAnchor& end) const
{
    ConnectableShape* startTarget = start.targetShape();
    ConnectableShape* endTarget = end.targetShape();
    if (startTarget != nullptr && startTarget == endTarget) {
        return true;
    }
    return false;
}

bool ConnectorController::hitTestConnectorEndpoint(const QPointF& scenePos, Connector*& outConnector, EndpointType& outType) const
{
    if (!m_canvas || !m_canvas->scene()) return false;
    qreal tol = 10.0 / getSafeViewScale(); // 10px 的屏幕容差
    QRectF searchRect(scenePos.x() - tol, scenePos.y() - tol, tol * 2.0, tol * 2.0);
    QList<QGraphicsItem*> itemsInArea = m_canvas->scene()->items(searchRect);

    Connector* bestConn = nullptr;
    EndpointType bestType = EndpointType::None;
    qreal minDist = tol + 1.0;

    for(QGraphicsItem* item : itemsInArea)
    {
        if(auto conn = dynamic_cast<Connector*>(item))
        {
            if(conn->getVisible() && !conn->isLocked())
            {
                qreal dist1 = QLineF(scenePos, conn->getStartAnchor().resolveScenePoint()).length();
                if(dist1 <= tol && dist1 < minDist)
                {
                    bestConn = conn;
                    bestType = EndpointType::Start;
                    minDist = dist1;
                }
                qreal dist2 = QLineF(scenePos, conn->getEndAnchor().resolveScenePoint()).length();
                if(dist2 <= tol && dist2 < minDist)
                {
                    bestConn = conn;
                    bestType = EndpointType::End;
                    minDist = dist2;
                }
            }
        }
    }
    if (bestConn) {
        outConnector = bestConn;
        outType = bestType;
        return true;
    }
    return false;
}

void ConnectorController::updateSnapIndicator(const ConnectorAnchor& anchor)
{
    if(!m_canvas || !m_canvas->scene()) return;
    QGraphicsScene* currentScene = m_canvas->scene();
    if(m_indicatorScene != currentScene)
    {
        destroySnapIndicator();
    }
    QPointF scenePt = anchor.resolveScenePoint();
    if(m_snapIndicator.isNull())
    {
        m_snapIndicator = new SnapIndicator();
        currentScene->addItem(m_snapIndicator);
        m_indicatorScene = currentScene;
    }
    if(anchor.mode() == ConnectorAnchor::Mode::Free)
    {
        m_snapIndicator->hide();
        return;
    }
    else if(anchor.mode() == ConnectorAnchor::Mode::Boundary)
    {
        m_snapIndicator->setPen(QPen(Qt::blue, 2.0 / getSafeViewScale()));
        m_snapIndicator->setBrush(QBrush(Qt::cyan));

        qreal radius = 6.0 / getSafeViewScale();
        m_snapIndicator->setRect(scenePt.x() - radius, scenePt.y() - radius, radius * 2, radius * 2);
    }
    else if(anchor.mode() == ConnectorAnchor::Mode::Interior)
    {
        m_snapIndicator->setPen(QPen(Qt::darkGreen, 2.0 / getSafeViewScale()));
        m_snapIndicator->setBrush(QBrush(QColor(0,255,0,100)));

        qreal radius = 5.0 / getSafeViewScale();
        m_snapIndicator->setRect(scenePt.x() - radius, scenePt.y() - radius, radius * 2, radius * 2);
    }
    m_snapIndicator->show();
}

void ConnectorController::hideSnapIndicator()
{
    if(!m_snapIndicator.isNull())
    {
        m_snapIndicator->hide();
    }
}

void ConnectorController::startCreating(const QPointF& scenePos, Qt::KeyboardModifiers modifiers)
{
    m_createStartScenePos = scenePos;
    AnchorResolver::ResolveOptions options;
    options.scenePoint = scenePos;
    options.scene = m_canvas ? m_canvas->scene() : nullptr;
    options.ctrlPressed = modifiers.testFlag(Qt::ControlModifier);
    options.viewScale = getSafeViewScale();
    options.ignoreShape = nullptr;
    ConnectorAnchor startAnchor = AnchorResolver::resolve(options);
    m_activeConnector = new Connector(startAnchor.resolveScenePoint(), startAnchor.resolveScenePoint());
    if (m_canvas) {
        // 创建工具决定连线的初始端点样式。普通 Line 必须显式设为 None，
        // 不能依赖 Connector 的构造默认值，否则历史默认值变化会污染工具语义。
        Connector::EndStyle style = Connector::EndStyle::None;
        if (m_canvas->toolMode() == Canvas::ToolMode::CreateArrow) {
            style = Connector::EndStyle::Arrow;
        } else if (m_canvas->toolMode() == Canvas::ToolMode::CreateDualArrow) {
            style = Connector::EndStyle::DualArrow;
        }
        m_activeConnector->setEndStyle(style);
        if (m_canvas->scene()) {
            m_canvas->scene()->addItem(m_activeConnector);
        }
    }
    if (!m_activeConnector->setStartAnchor(startAnchor) || !m_activeConnector->setEndAnchor(startAnchor)) {
        abortCreating();
        return;
    }
    updateSnapIndicator(startAnchor);
    m_state = State::Creating;
}

void ConnectorController::updateCreating(const QPointF& scenePos, Qt::KeyboardModifiers modifiers)
{
    if (!m_activeConnector || m_activeConnector->isLocked()) {
        abortCreating();
        return;
    }

    const QPointF startPoint = m_activeConnector->getStartAnchor().resolveScenePoint();
    const QPointF effectiveScenePos = modifiers.testFlag(Qt::ShiftModifier)
        ? DrawingConstraints::constrainLineEndpoint(startPoint, scenePos)
        : scenePos;

    AnchorResolver::ResolveOptions options;
    options.scenePoint = effectiveScenePos;
    options.scene = m_canvas ? m_canvas->scene() : nullptr;
    options.ctrlPressed = modifiers.testFlag(Qt::ControlModifier);
    options.viewScale = getSafeViewScale();
    options.ignoreShape = nullptr;
    ConnectorAnchor endAnchor = AnchorResolver::resolve(options);
    if (!m_activeConnector->setEndAnchor(endAnchor)) {
        abortCreating();
        return;
    }
    updateSnapIndicator(endAnchor);
}

void ConnectorController::finishCreating(const QPointF& scenePos, Qt::KeyboardModifiers modifiers)
{
    if (!m_activeConnector || m_activeConnector->isLocked()) {
        abortCreating();
        return;
    }
    // 判断用户是点击松开（Click-Move-Click）还是按住拖动（Drag-to-Connect），必须基于原始场景坐标 m_createStartScenePos
    qreal dist = QLineF(scenePos, m_createStartScenePos).length();
    if(dist <= 8.0 / getSafeViewScale()) return;

    const QPointF startPoint = m_activeConnector->getStartAnchor().resolveScenePoint();
    const QPointF effectiveScenePos = modifiers.testFlag(Qt::ShiftModifier)
        ? DrawingConstraints::constrainLineEndpoint(startPoint, scenePos)
        : scenePos;

    AnchorResolver::ResolveOptions options;
    options.scenePoint = effectiveScenePos;
    options.scene = m_canvas ? m_canvas->scene() : nullptr;
    options.ctrlPressed = modifiers.testFlag(Qt::ControlModifier);
    options.viewScale = getSafeViewScale();
    options.ignoreShape = nullptr;
    ConnectorAnchor endAnchor = AnchorResolver::resolve(options);

    QPointF startPt = m_activeConnector->getStartAnchor().resolveScenePoint();
    bool isTrashLine = false;
    if(QLineF(endAnchor.resolveScenePoint(), startPt).length() < 5.0 / getSafeViewScale() ||
       isSelfConnection(m_activeConnector->getStartAnchor(), endAnchor))
    {
        isTrashLine = true;
    }

    if (isTrashLine) {
        abortCreating();
        return;
    }
    if (!m_activeConnector->setEndAnchor(endAnchor)) {
        abortCreating();
        return;
    }

    if (m_canvas && m_canvas->undoManager()) {
        m_canvas->undoManager()->push(new CreateConnectorCommand(m_activeConnector, m_canvas->scene()));
    }

    emit connectorCreated(m_activeConnector);
    resetOperationState();
}

bool ConnectorController::handleMousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (!event) return false;
    return handleMousePress(event->scenePos(), event->button(), event->modifiers());
}

bool ConnectorController::handleMouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    if (!event) return false;
    return handleMouseMove(event->scenePos(), event->modifiers());
}

bool ConnectorController::handleMouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    if (!event) return false;
    return handleMouseRelease(event->scenePos(), event->button(), event->modifiers());
}

bool ConnectorController::handleKeyPressEvent(QKeyEvent* event)
{
    if (!event) return false;
    return handleKeyPress(event->key(), event->modifiers());
}

bool ConnectorController::handleKeyReleaseEvent(QKeyEvent* event)
{
    if (!event) return false;
    return handleKeyRelease(event->key(), event->modifiers());
}

bool ConnectorController::handleMousePress(const QPointF& scenePos, Qt::MouseButton button, Qt::KeyboardModifiers modifiers)
{
    if (!m_canvas || !m_canvas->scene()) return false;
    m_lastScenePos = scenePos;
    if(button != Qt::LeftButton) return false;

    if(m_state == State::Creating)
    {
        finishCreating(scenePos, modifiers);
        // 如果成功完成了两点点击连线（状态回到了 Idle），标记吸收下一次同一操作流对应的鼠标释放
        if(m_state == State::Idle) {
            m_consumeNextRelease = true;
        }
        return true;
    }

    Connector* hitConnector = nullptr;
    EndpointType hitEndpoint = EndpointType::None;
    if(hitTestConnectorEndpoint(scenePos, hitConnector, hitEndpoint))
    {
        if(!hitConnector->isLocked())
        {
            startDraggingEndpoint(hitConnector, hitEndpoint, scenePos, modifiers);
            return true;
        }
    }
    else if(m_createModeActive)
    {
        startCreating(scenePos, modifiers);
        if (m_state == State::Creating) {
            return true;
        }
        return false;
    }
    return false;
}

bool ConnectorController::handleMouseMove(const QPointF& scenePos, Qt::KeyboardModifiers modifiers)
{
    m_lastScenePos = scenePos;
    if(m_state == State::Creating)
    {
        updateCreating(scenePos, modifiers);
        return true;
    }
    else if(m_state == State::DraggingEndpoint)
    {
        updateDraggingEndpoint(scenePos, modifiers);
        return true;
    }
    else if(m_createModeActive && m_state == State::Idle)
    {
        AnchorResolver::ResolveOptions options;
        options.scenePoint = scenePos;
        options.scene = m_canvas ? m_canvas->scene() : nullptr;
        options.ctrlPressed = modifiers.testFlag(Qt::ControlModifier);
        options.viewScale = getSafeViewScale();
        options.ignoreShape = nullptr;
        ConnectorAnchor hoverAnchor = AnchorResolver::resolve(options);
        if (hoverAnchor.mode() != ConnectorAnchor::Mode::Free) {
            updateSnapIndicator(hoverAnchor);
        } else {
            hideSnapIndicator();
        }
    }
    return false;
}

bool ConnectorController::handleMouseRelease(const QPointF& scenePos, Qt::MouseButton button, Qt::KeyboardModifiers modifiers)
{
    m_lastScenePos = scenePos;
    if(button != Qt::LeftButton) return false;

    // 如果刚在 handleMousePress 中两点点击确认了连线，直接吸收该 Release 事件以防穿透给 CanvasController
    if(m_consumeNextRelease)
    {
        m_consumeNextRelease = false;
        return true;
    }

    if(m_state == State::Creating)
    {
        finishCreating(scenePos, modifiers);
        return true;
    }
    else if(m_state == State::DraggingEndpoint)
    {
        finishDraggingEndpoint(scenePos, modifiers);
        return true;
    }
    return false;
}

bool ConnectorController::handleKeyPress(int key, Qt::KeyboardModifiers modifiers)
{
    // 按下 ESC 键直接放弃并中止，保留绘图工具本身的激活状态。
    if (key == Qt::Key_Escape) {
        if (m_state != State::Idle) {
            cancelCurrentOperation();
            return true;
        }
    }

    // 修饰键变化时立即重算预览；鼠标事件仍会持续提供完整 modifiers，
    // 这里主要覆盖“按下修饰键后尚未移动鼠标”的瞬间反馈。
    if (key == Qt::Key_Control || key == Qt::Key_Shift) {
        Qt::KeyboardModifiers updatedModifiers = modifiers;
        updatedModifiers |= (key == Qt::Key_Control) ? Qt::ControlModifier : Qt::ShiftModifier;
        if (m_state == State::Creating && m_canvas) {
            updateCreating(m_lastScenePos, updatedModifiers);
            return true;
        } else if (m_state == State::DraggingEndpoint && m_canvas) {
            updateDraggingEndpoint(m_lastScenePos, updatedModifiers);
            return true;
        }
    }
    return false;
}

bool ConnectorController::handleKeyRelease(int key, Qt::KeyboardModifiers modifiers)
{
    if (key != Qt::Key_Control && key != Qt::Key_Shift) return false;

    Qt::KeyboardModifiers updatedModifiers = modifiers;
    if (key == Qt::Key_Control) {
        updatedModifiers &= ~Qt::ControlModifier;
    } else {
        updatedModifiers &= ~Qt::ShiftModifier;
    }
    if (m_state == State::Creating && m_canvas) {
        updateCreating(m_lastScenePos, updatedModifiers);
        return true;
    } else if (m_state == State::DraggingEndpoint && m_canvas) {
        updateDraggingEndpoint(m_lastScenePos, updatedModifiers);
        return true;
    }
    return false;
}
