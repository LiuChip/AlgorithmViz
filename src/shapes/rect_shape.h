#ifndef RECT_SHAPE_H
#define RECT_SHAPE_H

#include "connectable_shape.h"

// 矩形图形，支持填充、边框、文本和连接锚点。
class RectShape : public ConnectableShape {
public:
  // 复用 ConnectableShape 的位置和尺寸构造函数。
  using ConnectableShape::ConnectableShape;

  // 创建当前矩形的深拷贝。
  Shape *clone() const override;

  // 返回矩形的本地几何路径。
  QPainterPath localGeometryPath() const override;

  // 计算从矩形中心沿指定角度射线命中的边框点。
  QPointF boundaryPointAtAngle(qreal angleRadians) const override;

protected:
  // 返回包含边框宽度的矩形包围盒。
  QRectF boundingRect() const override;

  // 绘制矩形的填充、边框和文本。
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;
};

#endif // RECT_SHAPE_H
