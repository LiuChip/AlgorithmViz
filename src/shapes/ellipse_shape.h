#ifndef ELLIPSE_SHAPE_H
#define ELLIPSE_SHAPE_H

#include "connectable_shape.h"

// 椭圆或圆形图形，支持填充、边框、文本和连接锚点。
class EllipseShape : public ConnectableShape {
public:
  // 复用 ConnectableShape 的位置和尺寸构造函数。
  using ConnectableShape::ConnectableShape;

  // 创建当前椭圆的深拷贝。
  Shape *clone() const override;

  // 返回椭圆的本地几何路径。
  QPainterPath localGeometryPath() const override;

  // 计算从椭圆中心沿指定角度射线命中的边框点。
  QPointF boundaryPointAtAngle(qreal angleRadians) const override;

protected:
  // 返回包含边框宽度的椭圆包围盒。
  QRectF boundingRect() const override;

  // 绘制椭圆的填充、边框和文本。
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;
};

#endif // ELLIPSE_SHAPE_H
