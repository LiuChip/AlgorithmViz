#ifndef CONNECTABLE_SHAPE_H
#define CONNECTABLE_SHAPE_H

#include "shape.h"

#include <QPainterPath>
#include <QVector>

// 可作为连接目标的图形基类，提供锚点描述和统一边界判断接口。
class ConnectableShape : public Shape {
public:
  // 描述一个图形上的锚点。边界锚点保存中心角度，内部锚点保存归一化坐标。
  struct AnchorSpec {
    enum class Type {
      BoundaryAngle,
      InteriorNormalized,
    };

    // 内部锚点的归一化坐标，x 和 y 的有效范围通常为 [0, 1]。
    struct InteriorValue {
      qreal x;
      qreal y;
    };

    Type type = Type::BoundaryAngle;

    // 根据 type 解释实际数据：BoundaryAngle 使用 boundaryAngle，
    // InteriorNormalized 使用 interiorPosition。
    union Value {
      qreal boundaryAngle;
      InteriorValue interiorPosition;

      // 默认将 Union 初始化为边界角度 0。
      constexpr Value() : boundaryAngle(0.0) {}
    } value;

    // 创建一个按中心角度保存的边界锚点。
    static AnchorSpec createAnchor(qreal angleRadians);

    // 创建一个按归一化坐标保存的内部锚点。
    static AnchorSpec createAnchor(QPointF normalizedPosition);

    // 读取边界锚点角度；如果当前不是边界锚点则返回 0。
    qreal boundaryAngle() const;

    // 读取内部锚点归一化坐标；如果当前不是内部锚点则返回空点。
    QPointF interiorPosition() const;
  };

protected:
  // 过渡阶段保存的锚点列表；正式 Connector 实现后可迁移到端点绑定对象。
  QVector<AnchorSpec> anchorPoints;

  // 继承 Shape 的位置和尺寸构造函数。
  using Shape::Shape;

  // 复制 Shape 公共属性和 ConnectableShape 的锚点数据。
  void copyConnectablePropertiesTo(ConnectableShape &shape) const;

public:
  // 返回当前图形保存的锚点列表。
  QVector<AnchorSpec> getAnchorPoints() const;

  // 替换当前图形的锚点列表。
  void setAnchorPoints(const QVector<AnchorSpec> &anchors);



  // 返回不包含边框宽度的真实本地图形路径。
  // 新增闭合图形时需要实现此函数。
  virtual QPainterPath localGeometryPath() const = 0;

  // 返回从本地图形中心沿指定角度射线与边框的交点。
  // 角度 0 指向 +X，正角度指向 Qt 坐标系中的 +Y。
  virtual QPointF boundaryPointAtAngle(qreal angleRadians) const = 0;

  // 返回用于 QGraphicsItem 命中和选中的实际路径。
  QPainterPath shape() const override;

};

#endif // CONNECTABLE_SHAPE_H
