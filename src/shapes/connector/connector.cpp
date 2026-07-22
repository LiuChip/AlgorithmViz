#include "connector.h"
#include "../connectable_shape.h"
#include <cmath>
#include <QSet>
#include <QPainterPathStroker>
#include <QPolygonF>

// 兼容 Windows MSVC / C++17 不保证 M_PI 的跨平台方案
constexpr qreal kPi = 3.14159265358979323846;

Connector::Connector(QPointF startScenePt, QPointF endScenePt)
    : Shape(0.0, 0.0, 0.0, 0.0) // 坐标系默认全部归于画布原点，靠端点坐标直接展示
{
    // 【重要防护】：连线绝不允许被鼠标直接拖拽整根身体到处跑！必须锁定它的整体移动性
    setFlag(ItemIsMovable, false);

    // 初始化为自由坐标模式
    startAnchor = ConnectorAnchor::createFree(startScenePt);
    endAnchor = ConnectorAnchor::createFree(endScenePt);
    refreshGeometry();
}

void Connector::degradeStartAnchorToFree() {
    if (startAnchor.mode() != ConnectorAnchor::Mode::Free) {
        startAnchor.degradeToFree();
    }
}

void Connector::degradeEndAnchorToFree() {
    if (endAnchor.mode() != ConnectorAnchor::Mode::Free) {
        endAnchor.degradeToFree();
    }
}

bool Connector::degradeStartAnchorToFreeIfUnlocked() {
    if (isLocked()) return false;
    degradeStartAnchorToFree();
    refreshGeometry();
    return true;
}

bool Connector::degradeEndAnchorToFreeIfUnlocked() {
    if (isLocked()) return false;
    degradeEndAnchorToFree();
    refreshGeometry();
    return true;
}

void Connector::refreshGeometry() {
    // 1. 通知底层渲染引擎刷新外接方框，避免线条在拖动或缩放时出现残留重绘阴影
    prepareGeometryChange();

    // 2. 将两端解析为实际的场景物理坐标
    startPoint = startAnchor.resolveScenePoint();
    endPoint = endAnchor.resolveScenePoint();

    // 3. 及时触发界面的实际重制渲染动作，同时发射几何变动信号给外部系统
    update();
    emit geometryChanged();
}

void Connector::rebuildTargetConnections() {
    // 1. 首先彻底清空此前挂载到旧图形上的监听事件
    for (const auto& conn : targetConnections) {
        disconnect(conn);
    }
    targetConnections.clear();

    // 2. 针对起点和终点的实际非空目标进行去重收集
    QSet<ConnectableShape*> targets;
    if (startAnchor.targetShape() && startAnchor.mode() != ConnectorAnchor::Mode::Free) {
        targets.insert(startAnchor.targetShape());
    }
    if (endAnchor.targetShape() && endAnchor.mode() != ConnectorAnchor::Mode::Free) {
        targets.insert(endAnchor.targetShape());
    }

    // 3. 对每一个去重后的有效目标，挂载唯一一次几何变化与销毁处理，防止双绑定销毁时重复刷新并消除未使用捕获
    for (ConnectableShape* target : targets) {
        targetConnections.append(
            connect(target, &Shape::geometryChanged,
                    this, &Connector::onTargetGeometryChanged)
        );

        // 记录在挂载本次连接瞬间，起点和终点分别是否绑定在该 target 上
        bool boundStart = (startAnchor.mode() != ConnectorAnchor::Mode::Free && startAnchor.targetShape() == target);
        bool boundEnd = (endAnchor.mode() != ConnectorAnchor::Mode::Free && endAnchor.targetShape() == target);

        targetConnections.append(
            connect(target, &QObject::destroyed, this, [this, boundStart, boundEnd](QObject* dyingObj) {
                bool degraded = false;
                // 检查起点：如果挂载时起点绑定在本目标上，且目前尚未改绑为其它非空对象
                if (boundStart && (startAnchor.targetShape() == dyingObj || startAnchor.targetShape() == nullptr)) {
                    degradeStartAnchorToFree();
                    degraded = true;
                }
                // 检查终点：如果挂载时终点绑定在本目标上，且目前尚未改绑为其它非空对象
                if (boundEnd && (endAnchor.targetShape() == dyingObj || endAnchor.targetShape() == nullptr)) {
                    degradeEndAnchorToFree();
                    degraded = true;
                }
                // 如果起点或终点发生了实际降级，统一且仅执行一次几何刷新！
                if (degraded) {
                    refreshGeometry();
                }
            })
        );
    }
}

void Connector::onTargetGeometryChanged() {
    refreshGeometry();
}

bool Connector::setStartAnchor(const ConnectorAnchor& anchor) {
    if (isLocked()) return false; // 锁定状态下禁止用户主动修改端点
    startAnchor = anchor;
    rebuildTargetConnections();
    refreshGeometry();
    return true;
}

bool Connector::setEndAnchor(const ConnectorAnchor& anchor) {
    if (isLocked()) return false; // 锁定状态下禁止用户主动修改端点
    endAnchor = anchor;
    rebuildTargetConnections();
    refreshGeometry();
    return true;
}

void Connector::setEndStyle(EndStyle style) {
    if (this->endStyle != style) {
        prepareGeometryChange();
        this->endStyle = style;
        update();
        emit geometryChanged();
    }
}

bool Connector::setBorderInfo(Border newBorder) {
    if (!std::isfinite(newBorder.borderWidth) || newBorder.borderWidth < 0.0) {
        return false;
    }
    bool geomChanged = (this->border.borderWidth != newBorder.borderWidth) ||
                       (this->border.borderStyle != newBorder.borderStyle);
    if (geomChanged) {
        prepareGeometryChange();
    }
    this->border = newBorder;
    update();
    if (geomChanged) {
        emit geometryChanged();
    }
    return true;
}

void Connector::setLocked(bool locked) {
    Shape::setLocked(locked);
    // 强制无论如何切换锁定/解锁，Connector 本身绝对不允许通过 QGraphicsItem 被拖拽移动
    setFlag(ItemIsMovable, false);
}

// 把主线条与尾部箭头的全部形状构造成统一路径，以此支撑真正的包围盒与点击不被吃掉
QPainterPath Connector::visualPath() const {
    QPainterPath path = localGeometryPath();

    if (endStyle == EndStyle::Arrow && startPoint != endPoint &&
        border.borderWidth > 0.0 && border.borderStyle != Qt::NoPen) {
        qreal angle = std::atan2(endPoint.y() - startPoint.y(), endPoint.x() - startPoint.x());
        qreal arrowSize = qMax<qreal>(10.0, border.borderWidth * 3.0);
        qreal angleOffset = kPi / 6.0; // 30 度角

        QPointF arrowP1 = endPoint - QPointF(arrowSize * std::cos(angle - angleOffset),
                                             arrowSize * std::sin(angle - angleOffset));
        QPointF arrowP2 = endPoint - QPointF(arrowSize * std::cos(angle + angleOffset),
                                             arrowSize * std::sin(angle + angleOffset));

        QPolygonF arrowPolygon;
        arrowPolygon << endPoint << arrowP1 << arrowP2;
        path.addPolygon(arrowPolygon);
    }
    return path;
}

QPainterPath Connector::localGeometryPath() const {
    QPainterPath path;
    path.moveTo(startPoint);
    path.lineTo(endPoint);
    return path;
}

QRectF Connector::boundingRect() const {
    // 拿带有箭头多边形的实际全路径范围，额外再扩充描边的安全余量
    QRectF rect = visualPath().boundingRect();
    qreal margin = qMax<qreal>(border.borderWidth * 1.5, 12.0);
    return rect.adjusted(-margin, -margin, margin, margin);
}

QPainterPath Connector::shape() const {
    QPainterPath visual = visualPath();
    QPainterPathStroker stroker;
    stroker.setWidth(qMax<qreal>(border.borderWidth, 10.0)); // 给用户留下至少 10 像素的手指宽容度
    // visual.united 既包含了箭头实心内部全部区域，也包含了线条和轮廓周围至少 10 像素的容错边
    return visual.united(stroker.createStroke(visual));
}

void Connector::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    applyBorderStyle(painter);

    // 1. 画主线段
    painter->drawLine(startPoint, endPoint);

    // 2. 如果是箭头样式，填充底纹画出箭头三角形
    if (endStyle == EndStyle::Arrow && startPoint != endPoint &&
        border.borderWidth > 0.0 && border.borderStyle != Qt::NoPen) {
        qreal angle = std::atan2(endPoint.y() - startPoint.y(), endPoint.x() - startPoint.x());
        qreal arrowSize = qMax<qreal>(10.0, border.borderWidth * 3.0);
        qreal angleOffset = kPi / 6.0;

        QPointF arrowP1 = endPoint - QPointF(arrowSize * std::cos(angle - angleOffset),
                                             arrowSize * std::sin(angle - angleOffset));
        QPointF arrowP2 = endPoint - QPointF(arrowSize * std::cos(angle + angleOffset),
                                             arrowSize * std::sin(angle + angleOffset));

        QPolygonF arrowPolygon;
        arrowPolygon << endPoint << arrowP1 << arrowP2;
        painter->setBrush(border.borderColor);
        painter->drawPolygon(arrowPolygon);
    }

    // 3. 绘制文字标签
    drawText(painter, QRectF(startPoint, endPoint).normalized());
    drawSelectionOutline(painter);
}

QVariant Connector::itemChange(GraphicsItemChange change, const QVariant &value) {
    // 截获各种尝试让 Connector 本身发生偏移或旋转和非正常缩放的非法指令，强行将它的核心原点锁在点上
    if (change == ItemPositionChange || change == ItemRotationChange ||
        change == ItemScaleChange || change == ItemTransformChange) {
        if (change == ItemPositionChange) return QPointF(0, 0);
        if (change == ItemRotationChange) return 0.0;
        if (change == ItemScaleChange) return 1.0;
        if (change == ItemTransformChange) return QTransform();
    }
    return Shape::itemChange(change, value);
}

Shape* Connector::clone() const {
    // 独立克隆连接线时，生成的是一条处于纯自由空间（Mode::Free）的新连线，并完整复制外观属性（不重复拷贝）
    auto* copy = new Connector(startPoint, endPoint);
    copyPropertiesTo(*copy);
    copy->setEndStyle(this->endStyle);
    return copy;
}