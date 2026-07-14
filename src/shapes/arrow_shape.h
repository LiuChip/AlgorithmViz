#ifndef ARROW_SHAPE_H
#define ARROW_SHAPE_H

#include "line_shape.h"

// 单箭头图形，在普通线段终点绘制箭头头部。
class ArrowShape : public LineShape {
public:
  // 复用 LineShape 的起点和终点构造函数。
  using LineShape::LineShape;

  // 创建当前箭头的深拷贝。
  Shape *clone() const override;

protected:
  // 返回覆盖线段和箭头头部的包围盒。
  QRectF boundingRect() const override;

  // 绘制线段以及终点箭头头部。
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;

private:
  // 箭头头部的边长或绘制尺寸。
  qreal arrowSize = 10.0;
};

#endif // ARROW_SHAPE_H
