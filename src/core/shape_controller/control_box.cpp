#include "control_box.h"
#include "shapes/shape.h"
#include "shapes/line_shape.h"
#include "shapes/connector/connector.h"
#include <QGraphicsScene>
#include <QGraphicsView>
#include "anchor_resolver.h"
#include <QGuiApplication>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QScreen>
#include <QCursor>
#include <QtMath>
#include <cmath>

namespace {

// 创建类似 Windows“重启”图标的顺时针单箭头圆环光标。
// 光标以图像中心为热点，因此旋转手柄的实际拖动起点不会因图标形状而偏移。
QCursor createRotateCursor()
{
    constexpr qreal kLogicalSize = 32.0;
    constexpr qreal kCenter = kLogicalSize * 0.5;
    constexpr qreal kRadius = 9.5;

    // 在高 DPI 屏幕上按物理像素绘制，再通过 devicePixelRatio 保持 32px 逻辑尺寸。
    const QScreen *screen = QGuiApplication::primaryScreen();
    const qreal devicePixelRatio = screen ? qBound(1.0, screen->devicePixelRatio(), 4.0) : 1.0;
    const int physicalSize = qCeil(kLogicalSize * devicePixelRatio);

    QPixmap pixmap(physicalSize, physicalSize);
    pixmap.setDevicePixelRatio(devicePixelRatio);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 使用屏幕坐标角度：0 度向右，正角度顺时针。
    // 圆弧末端位于右上角，其切线方向指向右下方的圆弧尾部。
    constexpr qreal startAngleDegrees = 35.0;
    constexpr qreal endAngleDegrees = 315.0;
    constexpr int segmentCount = 64;

    QPainterPath arcPath;
    QPointF arrowTip;
    for (int index = 0; index <= segmentCount; ++index) {
        const qreal progress = static_cast<qreal>(index) / segmentCount;
        const qreal angleDegrees = startAngleDegrees
                                   + (endAngleDegrees - startAngleDegrees) * progress;
        const qreal angleRadians = qDegreesToRadians(angleDegrees);
        const QPointF point(kCenter + kRadius * std::cos(angleRadians),
                            kCenter + kRadius * std::sin(angleRadians));
        if (index == 0) {
            arcPath.moveTo(point);
        } else {
            arcPath.lineTo(point);
        }
        arrowTip = point;
    }

    const qreal arrowAngleRadians = qDegreesToRadians(endAngleDegrees);
    const QPointF tangent(-std::sin(arrowAngleRadians), std::cos(arrowAngleRadians));
    const QPointF normal(-tangent.y(), tangent.x());
    const QPointF arrowBaseCenter = arrowTip - tangent * 6.0;
    QPolygonF arrowHead;
    arrowHead << arrowTip
              << arrowBaseCenter + normal * 3.6
              << arrowBaseCenter - normal * 3.6;

    // 先画浅色轮廓，再画深色主体，让光标在深色和浅色画布上都清晰可见。
    const QColor outlineColor(255, 255, 255, 235);
    const QColor foregroundColor(25, 25, 25);
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(outlineColor, 5.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(arcPath);
    painter.setPen(QPen(foregroundColor, 2.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawPath(arcPath);

    painter.setPen(QPen(outlineColor, 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(outlineColor);
    painter.drawPolygon(arrowHead);
    painter.setPen(QPen(foregroundColor, 0.9, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(foregroundColor);
    painter.drawPolygon(arrowHead);

    return QCursor(pixmap, qRound(kCenter), qRound(kCenter));
}

// 每个进程只创建一次光标，避免每次构造手柄时重复分配并绘制 QPixmap。
const QCursor &rotateCursor()
{
    static const QCursor cursor = createRotateCursor();
    return cursor;
}

} // namespace

HandleItem::HandleItem(HandleType type, ControlBox *box, QGraphicsItem *parent)
    : QGraphicsObject(parent), m_type(type), m_box(box)
{
    // 开启缩放忽略，使手柄本地坐标单位大小 = 屏幕实际渲染像素大小
    setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);
    setHoverCursor();
}

HandleItem::~HandleItem() = default;
HandleType HandleItem::handleType() const
{
    return m_type;
}

void HandleItem::setHoverCursor()
{
    switch (m_type) {
    case HandleType::TopLeft:
    case HandleType::BottomRight:
        setCursor(Qt::SizeFDiagCursor); // 副对角缩放光标
        break;
    case HandleType::TopRight:
    case HandleType::BottomLeft:
        setCursor(Qt::SizeBDiagCursor); // 主对角缩放光标
        break;
    case HandleType::Top:
    case HandleType::Bottom:
        setCursor(Qt::SizeVerCursor);   // 垂直方向光标
        break;
    case HandleType::Left:
    case HandleType::Right:
        setCursor(Qt::SizeHorCursor);   // 水平方向光标
        break;
    case HandleType::Rotate:
        setCursor(rotateCursor());
        break;
    case HandleType::StartEndpoint:
    case HandleType::EndEndpoint:
        setCursor(Qt::CrossCursor);     // 端点精细十字光标
        break;
    }

}

QRectF HandleItem::boundingRect() const
{
    // boundingRect 必须完整包含 shape()，否则靠近手柄边缘的悬停命中会被场景索引裁掉。
    const qreal half = (m_size + 4.0) * 0.5;
    return QRectF(-half, -half, half * 2.0, half * 2.0);
}
QPainterPath HandleItem::shape() const
{
    QPainterPath path;
    // 为用户点击舒适度，额外向外扩大约 4px 的点击感错容区间
    qreal half = (m_size + 4.0) * 0.5;
    if (m_type == HandleType::Rotate || m_type == HandleType::StartEndpoint || m_type == HandleType::EndEndpoint) {
        path.addEllipse(QPointF(0, 0), half, half);
    } else {
        path.addRect(-half, -half, half * 2.0, half * 2.0);
    }
    return path;
}
void HandleItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    QPen pen(QColor(0, 120, 215), 1.5);
    QBrush brush(Qt::white);
    // 针对旋转和端点特殊调色
    if (m_type == HandleType::Rotate) {
        pen.setColor(QColor(40, 160, 40));      // 绿色旋转
        brush.setColor(QColor(240, 255, 240));
    } else if (m_type == HandleType::StartEndpoint || m_type == HandleType::EndEndpoint) {
        pen.setColor(QColor(220, 60, 60));      // 红色连线锚点
        brush.setColor(QColor(255, 240, 240));
    }
    painter->setPen(pen);
    painter->setBrush(brush);
    qreal half = m_size * 0.5;
    if (m_type == HandleType::Rotate || m_type == HandleType::StartEndpoint || m_type == HandleType::EndEndpoint) {
        painter->drawEllipse(QPointF(0, 0), half, half);
    } else {
        painter->drawRect(QRectF(-half, -half, m_size, m_size));
    }
    painter->restore();
}

void HandleItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit handlePressed(this, event->scenePos());
        event->accept();
    } else {
        QGraphicsObject::mousePressEvent(event);
    }
}
void HandleItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    emit handleMoved(this, event->scenePos(), event->modifiers());
    event->accept();
}
void HandleItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        setHoverCursor(); // 松手恢复悬停光标
        emit handleReleased(this, event->scenePos());
        event->accept();
    } else {
        QGraphicsObject::mouseReleaseEvent(event);
    }
}

ControlBox::ControlBox(QGraphicsItem *parent)
    : QGraphicsObject(parent)
{
    setZValue(99999.0);
}
ControlBox::~ControlBox()
{
    clearHandles();
}
void ControlBox::setTarget(Shape *target)
{
    if (m_target == target) return;
    if (m_target) {
        disconnect(m_target, &Shape::geometryChanged, this, &ControlBox::onTargetGeometryChanged);
        disconnect(m_target, &QObject::destroyed, this, &ControlBox::onTargetDestroyed);
        disconnect(m_target, &Shape::lockedChanged, this, &ControlBox::onTargetLockedChanged);
    }
    m_target = target;
    m_isOperating = false;
    clearHandles();
    if (!m_target || !m_target->isVisible() || m_target->isLocked()) {
        setVisible(false);
        if (m_target) {
            connect(m_target, &Shape::lockedChanged, this, &ControlBox::onTargetLockedChanged);
        }
        return;
    }
    connect(m_target, &Shape::geometryChanged, this, &ControlBox::onTargetGeometryChanged);
    connect(m_target, &QObject::destroyed, this, &ControlBox::onTargetDestroyed);
    connect(m_target, &Shape::lockedChanged, this, &ControlBox::onTargetLockedChanged);
    // 区分图形类型创建专有手柄列表
    if (dynamic_cast<Connector*>(m_target.data()) || dynamic_cast<LineShape*>(m_target.data())) {
        createHandlesForLineShape();
    } else {
        createHandlesForNormalShape();
    }
    setVisible(true);
    updateHandlePositions();
}
Shape *ControlBox::target() const
{
    return m_target.data();
}
void ControlBox::clearHandles()
{
    for (auto *handle : m_handles) {
        if (handle) {
            handle->setEnabled(false);
            handle->setVisible(false);
            handle->setAcceptedMouseButtons(Qt::NoButton);
            if (handle->scene()) {
                handle->scene()->removeItem(handle);
            }
            handle->deleteLater();
        }
    }
    m_handles.clear();
}
void ControlBox::createHandlesForNormalShape()
{
    const HandleType types[] = {
        HandleType::TopLeft,     HandleType::Top,    HandleType::TopRight,
        HandleType::Right,       HandleType::BottomRight, HandleType::Bottom,
        HandleType::BottomLeft,  HandleType::Left,   HandleType::Rotate
    };
    for (HandleType type : types) {
        auto *handle = new HandleItem(type, this, this);
        connect(handle, &HandleItem::handlePressed, this, &ControlBox::onHandlePressed);
        connect(handle, &HandleItem::handleMoved, this, &ControlBox::onHandleMoved);
        connect(handle, &HandleItem::handleReleased, this, &ControlBox::onHandleReleased);
        m_handles.append(handle);
    }
}
void ControlBox::createHandlesForLineShape()
{
    const HandleType types[] = {
        HandleType::StartEndpoint, HandleType::EndEndpoint
    };
    for (HandleType type : types) {
        auto *handle = new HandleItem(type, this, this);
        connect(handle, &HandleItem::handlePressed, this, &ControlBox::onHandlePressed);
        connect(handle, &HandleItem::handleMoved, this, &ControlBox::onHandleMoved);
        connect(handle, &HandleItem::handleReleased, this, &ControlBox::onHandleReleased);
        m_handles.append(handle);
    }
}
HandleItem *ControlBox::findHandle(HandleType type) const
{
    for (auto *handle : m_handles) {
        if (handle && handle->handleType() == type) {
            return handle;
        }
    }
    return nullptr;
}

void ControlBox::updateHandlePositions()
{
    if (!m_target || !m_target->isVisible() || m_target->isLocked()) {
        setVisible(false);
        return;
    }
    prepareGeometryChange();
    setPos(m_target->pos());
    setTransformOriginPoint(m_target->transformOriginPoint());
    setRotation(m_target->rotation());
    setScale(m_target->scale());

    auto setHandlePos = [this](HandleType type, QPointF point){
        if (auto *h = findHandle(type)) {
            h->setPos(point);
        }
    };

    if (auto *connector = dynamic_cast<Connector*>(m_target.data())) {
        setHandlePos(HandleType::StartEndpoint, mapFromScene(connector->getStartAnchor().resolveScenePoint()));
        setHandlePos(HandleType::EndEndpoint, mapFromScene(connector->getEndAnchor().resolveScenePoint()));
    }
    else if(auto *line = dynamic_cast<LineShape*>(m_target.data()))
    {
        setHandlePos(HandleType::StartEndpoint, mapFromScene(line->getStartPoint()));
        setHandlePos(HandleType::EndEndpoint,mapFromScene(line->getEndPoint()));

    }
    else
    {
        const qreal left = 0.0;
        const qreal top = 0.0;
        const qreal right = m_target->getSize().width();
        const qreal bottom = m_target->getSize().height();
        const qreal midX = (left + right) * 0.5;
        const qreal midY = (top + bottom) * 0.5;
        setHandlePos(HandleType::TopLeft,     QPointF(left, top));
        setHandlePos(HandleType::Top,         QPointF(midX, top));
        setHandlePos(HandleType::TopRight,    QPointF(right, top));
        setHandlePos(HandleType::Right,       QPointF(right, midY));
        setHandlePos(HandleType::BottomRight, QPointF(right, bottom));
        setHandlePos(HandleType::Bottom,      QPointF(midX, bottom));
        setHandlePos(HandleType::BottomLeft,  QPointF(left, bottom));
        setHandlePos(HandleType::Left,        QPointF(left, midY));
        setHandlePos(HandleType::Rotate,      QPointF(midX, top - 24.0));
    }
}

QRectF ControlBox::boundingRect() const
{
    if (!m_target) return QRectF();
    if (dynamic_cast<Connector*>(m_target.data()) || dynamic_cast<LineShape*>(m_target.data())) {
        // 针对连线，外扩 16px 防止两端手柄悬停命中被裁剪
        return m_target->boundingRect().adjusted(-16.0, -16.0, 16.0, 16.0);
    }
    const QSizeF sz = m_target->getSize();
    // 针对普通图形，上方预留足够高度容纳旋转手柄与其连杆，四周容纳缩放手柄
    return QRectF(0.0, 0.0, sz.width(), sz.height()).adjusted(-16.0, -36.0, 16.0, 16.0);
}
void ControlBox::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    if (!m_target) return;
    // 连接线/线段不需要在外围画淡蓝色的矩形选框与连杆
    if (dynamic_cast<Connector*>(m_target.data()) || dynamic_cast<LineShape*>(m_target.data())) {
        return;
    }
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    QPen dashPen(QColor(0, 120, 215), 1.0, Qt::DashLine);
    dashPen.setCosmetic(true); // 保证屏幕像素下虚线宽度恒定不随整体缩放变形
    painter->setPen(dashPen);
    painter->setBrush(Qt::NoBrush);
    const QSizeF sz = m_target->getSize();
    // 画从上边界中点至旋转手柄的连线杆（边框选框已经由各 Shape::drawSelectionOutline 负责绘制）
    painter->drawLine(QPointF(sz.width() * 0.5, 0.0), QPointF(sz.width() * 0.5, -24.0));
    painter->restore();
}

void ControlBox::onTargetGeometryChanged()
{
    // 如果用户当前正在通过拖动控制盒手柄操作图元，不要强行覆盖打断当前拖动
    if (m_isOperating) {
        return;
    }
    updateHandlePositions();
}
void ControlBox::onTargetDestroyed()
{
    m_target = nullptr;
    m_isOperating = false;
    clearHandles();
    setVisible(false);
}
void ControlBox::onTargetLockedChanged(bool locked)
{
    Q_UNUSED(locked);
    Shape *t = m_target.data();
    m_target = nullptr;
    setTarget(t);
}
void ControlBox::onHandlePressed(HandleItem *handle, const QPointF &scenePos)
{
    if (!m_target || !handle) return;
    Q_UNUSED(scenePos);
    m_isOperating = true;
    m_startSize = m_target->getSize();
    m_startPos = m_target->pos();
    m_startRotation = m_target->rotation();
    m_startEndpointPos = scenePos;
    const HandleType type = handle->handleType();
    if (type == HandleType::Rotate) {
        emit rotateStarted(m_target.data());
    } else if (type == HandleType::StartEndpoint || type == HandleType::EndEndpoint) {
        emit endpointMoveStarted(m_target.data(), type);
    } else {
        emit resizeStarted(m_target.data());
    }

}
void ControlBox::onHandleReleased(HandleItem *handle, const QPointF &scenePos)
{
    if (!m_target || !handle) return;
    m_isOperating = false;
    const HandleType type = handle->handleType();
    if (type == HandleType::Rotate) {
        emit rotateFinished(m_target.data(), m_startRotation, m_target->rotation());
    } else if (type == HandleType::StartEndpoint || type == HandleType::EndEndpoint) {
        if (auto *connector = dynamic_cast<Connector*>(m_target.data())) {
            AnchorResolver::ResolveOptions options;
            options.scenePoint = scenePos;
            options.scene = connector->scene();
            options.ctrlPressed = (QGuiApplication::keyboardModifiers() & Qt::ControlModifier);
            if (options.scene && !options.scene->views().isEmpty()) {
                options.viewScale = options.scene->views().first()->transform().m11();
            } else {
                options.viewScale = 1.0;
            }
            if (!std::isfinite(options.viewScale) || options.viewScale <= 1e-6) {
                options.viewScale = 1.0;
            }
            ConnectorAnchor newAnchor = AnchorResolver::resolve(options);
            if (type == HandleType::StartEndpoint) {
                connector->setStartAnchor(newAnchor);
            } else {
                connector->setEndAnchor(newAnchor);
            }
        } else if (auto *line = dynamic_cast<LineShape*>(m_target.data())) {
            if (type == HandleType::StartEndpoint) {
                line->setStartPoint(scenePos);
            } else {
                line->setEndPoint(scenePos);
            }
        }
        emit endpointMoveFinished(m_target.data(), type, m_startEndpointPos, scenePos);
    } else {
        emit resizeFinished(m_target.data(), m_startSize, m_target->getSize(), m_startPos, m_target->pos());
    }
    // 操作结束后统一同位更新一次坐标，确保手柄无缝归位
    updateHandlePositions();
}
void ControlBox::onHandleMoved(HandleItem *handle, const QPointF &scenePos, Qt::KeyboardModifiers modifiers)
{
    if (!m_target || !handle) return;
    const HandleType type = handle->handleType();
    // 处理连线的端点拖动
    if (type == HandleType::StartEndpoint || type == HandleType::EndEndpoint) {
        if (auto *connector = dynamic_cast<Connector*>(m_target.data())) {
            AnchorResolver::ResolveOptions options;
            options.scenePoint = scenePos;
            options.scene = connector->scene();
            options.ctrlPressed = (modifiers & Qt::ControlModifier);
            if (options.scene && !options.scene->views().isEmpty()) {
                options.viewScale = options.scene->views().first()->transform().m11();
            } else {
                options.viewScale = 1.0;
            }
            if (!std::isfinite(options.viewScale) || options.viewScale <= 1e-6) {
                options.viewScale = 1.0;
            }
            ConnectorAnchor newAnchor = AnchorResolver::resolve(options);
            if (type == HandleType::StartEndpoint) {
                connector->setStartAnchor(newAnchor);
            } else {
                connector->setEndAnchor(newAnchor);
            }
        }
        else if(auto *line = dynamic_cast<LineShape*>(m_target.data()))
        {
            if (type == HandleType::StartEndpoint) {
                line->setStartPoint(scenePos);
            } else {
                line->setEndPoint(scenePos);
            }
        }
        updateHandlePositions();
        return;
    }

    // 处理旋转
    if (type == HandleType::Rotate) {
        const QSizeF size = m_target->getSize();
        const QPointF localCenter(size.width() * 0.5, size.height() * 0.5);
        const QPointF sceneCenter = m_target->mapToScene(localCenter);

        const qreal angleRadians = std::atan2((scenePos - sceneCenter).y(), (scenePos - sceneCenter).x());
        qreal angleDegrees = qRadiansToDegrees(angleRadians) + 90.0;

        while (angleDegrees < 0.0) angleDegrees += 360.0;
        while (angleDegrees >= 360.0) angleDegrees -= 360.0;

        if (modifiers & Qt::ShiftModifier) {
            angleDegrees = qRound(angleDegrees / 15.0) * 15.0;
            if (angleDegrees >= 360.0) angleDegrees -= 360.0;
        }

        m_target->setRotation(angleDegrees);
        updateHandlePositions();
        return;
    }
    const QPointF basisAnchorScenePos = getOppositeAnchorPos(type);
    applyResizeFromAnchor(type, basisAnchorScenePos, scenePos, modifiers);
    updateHandlePositions();
}

QPointF ControlBox::getOppositeAnchorPos(HandleType type) const
{
    if (!m_target) return QPointF();
    const QSizeF size = m_target->getSize();
    switch (type) {
    case HandleType::TopLeft:     return mapToScene(QPointF(size.width(), size.height()));
    case HandleType::Top:         return mapToScene(QPointF(size.width() * 0.5, size.height()));
    case HandleType::TopRight:    return mapToScene(QPointF(0.0, size.height()));
    case HandleType::Right:       return mapToScene(QPointF(0.0, size.height() * 0.5));
    case HandleType::BottomRight: return mapToScene(QPointF(0.0, 0.0));
    case HandleType::Bottom:      return mapToScene(QPointF(size.width() * 0.5, 0.0));
    case HandleType::BottomLeft:  return mapToScene(QPointF(size.width(), 0.0));
    case HandleType::Left:        return mapToScene(QPointF(size.width(), size.height() * 0.5));
    default:
        return QPointF();
    }
}

void ControlBox::applyResizeFromAnchor(HandleType type, const QPointF &basisAnchorScenePos, const QPointF &mouseScenePos, Qt::KeyboardModifiers modifiers)
{
    if (!m_target) return;
    const QPointF localMouse = m_target->mapFromScene(mouseScenePos);
    const QPointF localBasis = m_target->mapFromScene(basisAnchorScenePos);

    qreal newWidth = m_target->getSize().width();
    qreal newHeight = m_target->getSize().height();

    // 在图元局部坐标系中直接计算尺寸变化，完美兼容任意 rotation 和 scale
    switch (type) {
    case HandleType::TopLeft:     newWidth = localBasis.x() - localMouse.x(); newHeight = localBasis.y() - localMouse.y(); break;
    case HandleType::Top:                                                     newHeight = localBasis.y() - localMouse.y(); break;
    case HandleType::TopRight:    newWidth = localMouse.x() - localBasis.x(); newHeight = localBasis.y() - localMouse.y(); break;
    case HandleType::Right:       newWidth = localMouse.x() - localBasis.x();                                              break;
    case HandleType::BottomRight: newWidth = localMouse.x() - localBasis.x(); newHeight = localMouse.y() - localBasis.y(); break;
    case HandleType::Bottom:                                                  newHeight = localMouse.y() - localBasis.y(); break;
    case HandleType::BottomLeft:  newWidth = localBasis.x() - localMouse.x(); newHeight = localMouse.y() - localBasis.y(); break;
    case HandleType::Left:        newWidth = localBasis.x() - localMouse.x();                                              break;
    default: break;
    }

    newWidth = qMax<qreal>(10.0, newWidth);
    newHeight = qMax<qreal>(10.0, newHeight);

    if (modifiers & Qt::ShiftModifier) {
        if (type == HandleType::TopLeft || type == HandleType::TopRight ||
            type == HandleType::BottomLeft || type == HandleType::BottomRight) {
            const qreal dim = qMax(newWidth, newHeight);
            newWidth = dim;
            newHeight = dim;
        }
    }

    QPointF newLocalBasis;
    switch (type) {
    case HandleType::TopLeft:     newLocalBasis = QPointF(newWidth, newHeight); break;
    case HandleType::Top:         newLocalBasis = QPointF(newWidth * 0.5, newHeight); break;
    case HandleType::TopRight:    newLocalBasis = QPointF(0.0, newHeight); break;
    case HandleType::Right:       newLocalBasis = QPointF(0.0, newHeight * 0.5); break;
    case HandleType::BottomRight: newLocalBasis = QPointF(0.0, 0.0); break;
    case HandleType::Bottom:      newLocalBasis = QPointF(newWidth * 0.5, 0.0); break;
    case HandleType::BottomLeft:  newLocalBasis = QPointF(newWidth, 0.0); break;
    case HandleType::Left:        newLocalBasis = QPointF(newWidth, newHeight * 0.5); break;
    default: break;
    }

    m_target->setSize(QSizeF(newWidth, newHeight));
    QPointF currentBasisScene = m_target->mapToScene(newLocalBasis);
    QPointF deltaScene = basisAnchorScenePos - currentBasisScene;
    if (deltaScene != QPointF(0, 0)) {
        if (m_target->parentItem()) {
            QPointF parentDelta = m_target->parentItem()->mapFromScene(basisAnchorScenePos) - m_target->parentItem()->mapFromScene(currentBasisScene);
            m_target->setPosition(m_target->pos() + parentDelta);
        } else {
            m_target->setPosition(m_target->pos() + deltaScene);
        }
    }
}
