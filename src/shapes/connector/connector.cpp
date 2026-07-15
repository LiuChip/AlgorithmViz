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
    if (startAnchor.mode != ConnectorAnchor::Mode::Free) {
        startAnchor.freeScenePoint = startAnchor.fallbackScenePoint;
        startAnchor.mode = ConnectorAnchor::Mode::Free;
        startAnchor.targetShape = nullptr;
    }
}

void Connector::degradeEndAnchorToFree() {
    if (endAnchor.mode != ConnectorAnchor::Mode::Free) {
        endAnchor.freeScenePoint = endAnchor.fallbackScenePoint;
        endAnchor.mode = ConnectorAnchor::Mode::Free;
        endAnchor.targetShape = nullptr;
    }
}

void Connector::refreshGeometry() {
    // 1. 通知底层渲染引擎刷新外接方框，避免线条在拖动或缩放时出现残留重绘阴影
    prepareGeometryChange();

    // 2. 调用端点探头算出全局最新物理点
    startPoint = startAnchor.resolveScenePoint();
    endPoint = endAnchor.resolveScenePoint();

    // 3. 算出基本长宽并更新
    QRectF rect(startPoint, endPoint);
    this->width = rect.normalized().width();
    this->height = rect.normalized().height();

    // 4. 重画自身并对外发出几何变更信号
    update();
    emit geometryChanged();
}

void Connector::rebuildTargetConnections() {
    // 1. 把先前挂载在本对象保存的所有旧信号句柄一次性干干净净断掉
    for (const auto& conn : targetConnections) {
        disconnect(conn);
    }
    targetConnections.clear();

    // 2. 收集现在起点和终点绑定的有效目标对象并做精准去重
    QSet<ConnectableShape*> targets;
    if (startAnchor.targetShape && startAnchor.mode != ConnectorAnchor::Mode::Free) {
        targets.insert(startAnchor.targetShape);
    }
    if (endAnchor.targetShape && endAnchor.mode != ConnectorAnchor::Mode::Free) {
        targets.insert(endAnchor.targetShape);
    }

    // 3. 对每一个去重后的有效目标，挂载唯一一次几何变化与销毁处理，防止双绑定销毁时重复刷新并消除未使用捕获
    for (ConnectableShape* target : targets) {
        targetConnections.append(
            connect(target, &Shape::geometryChanged,
                    this, &Connector::onTargetGeometryChanged)
        );

        // 记录在挂载本次连接瞬间，起点和终点分别是否绑定在该 target 上
        bool boundStart = (startAnchor.mode != ConnectorAnchor::Mode::Free && startAnchor.targetShape.data() == target);
        bool boundEnd = (endAnchor.mode != ConnectorAnchor::Mode::Free && endAnchor.targetShape.data() == target);

        targetConnections.append(
            connect(target, &QObject::destroyed, this, [this, boundStart, boundEnd](QObject* dyingObj) {
                bool degraded = false;
                // 检查起点：如果挂载时起点绑定在本目标上，且目前尚未改绑为其它非空对象
                if (boundStart && (startAnchor.targetShape == dyingObj || startAnchor.targetShape.isNull())) {
                    degradeStartAnchorToFree();
                    degraded = true;
                }
                // 检查终点：如果挂载时终点绑定在本目标上，且目前尚未改绑为其它非空对象
                if (boundEnd && (endAnchor.targetShape == dyingObj || endAnchor.targetShape.isNull())) {
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

void Connector::setStartAnchor(const ConnectorAnchor& anchor) {
    startAnchor = anchor;
    rebuildTargetConnections();
    refreshGeometry();
}

void Connector::setEndAnchor(const ConnectorAnchor& anchor) {
    endAnchor = anchor;
    rebuildTargetConnections();
    refreshGeometry();
}

void Connector::setEndStyle(EndStyle style) {
    if (this->endStyle != style) {
        prepareGeometryChange();
        this->endStyle = style;
        update();
        emit geometryChanged();
    }
}

void Connector::setLocked(bool locked) {
    Shape::setLocked(locked);
    // 强制无论如何切换锁定/解锁，Connector 本身绝对不允许通过 QGraphicsItem 被拖拽移动
    setFlag(ItemIsMovable, false);
}

// 把主线条与尾部箭头的全部形状构造成统一路径，以此支撑真正的包围盒与点击不被吃掉
QPainterPath Connector::visualPath() const {
    QPainterPath path = localGeometryPath();

    if (endStyle == EndStyle::Arrow && startPoint != endPoint) {
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
    if (endStyle == EndStyle::Arrow && startPoint != endPoint) {
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