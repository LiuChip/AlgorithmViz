#ifndef CONNECTOR_H
#define CONNECTOR_H

#include "../shape.h"
#include "connector_anchor.h"

// 连接线图形类。直接继承 Shape，不作为 ConnectableShape，从而从结构上彻底根除连线自身再次成为锚定目标造成的递归死循环。
class Connector : public Shape
{
    Q_OBJECT
public:
    enum class EndStyle{
        None,
        Arrow
    };
    explicit Connector(QPointF startScenePt, QPointF endScenePt);

    // 设置锚点核心接口
    bool setStartAnchor(const ConnectorAnchor& anchor);
    bool setEndAnchor(const ConnectorAnchor& anchor);

    ConnectorAnchor getStartAnchor() const { return startAnchor; }
    ConnectorAnchor getEndAnchor() const { return endAnchor; }

    // 显式将两端降级为 Free 模式（面向控制器的公开接口，遵守锁定语义）
    bool degradeStartAnchorToFreeIfUnlocked();
    bool degradeEndAnchorToFreeIfUnlocked();

    void setEndStyle(EndStyle style);
    EndStyle getEndStyle() const { return endStyle; }

    void setLocked(bool locked) override;

    // 必须实现的抽象接口
    Shape* clone() const override;

    // QGraphicsItem 必需的具体绘制与碰撞判定（在 QGraphicsItem 中为 public 接口）
    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    // 保护措施：禁用外部直接针对图形本身的位置、宽高、旋转与缩放设置并返回 false
    bool setPosition(QPointF point) override { Q_UNUSED(point); return false; }
    bool setPosition(qreal x, qreal y) override { Q_UNUSED(x); Q_UNUSED(y); return false; }
    bool setSize(QSizeF size) override { Q_UNUSED(size); return false; }
    bool setSize(qreal width, qreal height) override { Q_UNUSED(width); Q_UNUSED(height); return false; }
    bool setRotation(qreal rotation) override { Q_UNUSED(rotation); return false; }
    bool setScale(qreal scale) override { Q_UNUSED(scale); return false; }

    bool setBorderInfo(Border newBorder) override;
    bool setBorderInfo() override { return Shape::setBorderInfo(); }

private:
    // 1. 保存端点的锚点信息，供 UI/控制层调用 setStartAnchor/setEndAnchor 接口时使用
    ConnectorAnchor startAnchor;
    ConnectorAnchor endAnchor;

    // 2. 记住解析出来的场景坐标，供真正调用 QPainter 画直线和箭头时使用
    QPointF startPoint;
    QPointF endPoint;

    // 3. 终点样式（有没有箭头）
    EndStyle endStyle = EndStyle::Arrow;

    // 4. 专有的连接监听句柄，方便干净无死角地管理与去重
    QVector<QMetaObject::Connection> targetConnections;

    // 核心私有辅助：统一对各个目标图形重新建连（完美去重，且各自专属响应被销毁监听）
    void rebuildTargetConnections();
    // 内部自愈降级接口（面向目标销毁回调，无视锁定状态）
    void degradeStartAnchorToFree();
    void degradeEndAnchorToFree();
    // 核心私有同步：解析两端并重新算自己的外接矩形
    void refreshGeometry();
    // 核心骨架生成：把主线与箭头构造成统一路径（保证包围盒与点击不被切裁）
    QPainterPath visualPath() const;
    // 本地基础主线路径生成
    QPainterPath localGeometryPath() const;

protected:
    // 保护图形不被外力拖移原点及发生额外形变
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

private slots:
    // 当连接的目标图形移动、变形或重置尺寸时触发
    void onTargetGeometryChanged();
};

#endif // CONNECTOR_H
