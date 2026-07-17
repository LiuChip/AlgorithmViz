#ifndef DUAL_ARROW_SHAPE_H
#define DUAL_ARROW_SHAPE_H

#include "line_shape.h"

// 双箭头图形，在普通线段两端都绘制箭头头部。
class DualArrowShape : public LineShape {
public:
  // 复用 LineShape 的起点和终点构造函数。
  using LineShape::LineShape;

  // 创建当前双箭头的深拷贝。
  Shape *clone() const override;

  // 返回包含线段和两端箭头的几何可视化路径
  QPainterPath visualPath() const;

  // 返回实际可命中区域路径
  QPainterPath shape() const override;

protected:
  // 返回覆盖线段和两端箭头头部的包围盒。
  QRectF boundingRect() const override;

  // 绘制线段以及起点、终点两个箭头头部。
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;

private:
  // 箭头头部的边长或绘制尺寸。
  qreal arrowSize = 10.0;
};

#endif // DUAL_ARROW_SHAPE_H
