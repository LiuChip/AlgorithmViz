#ifndef LINE_SHAPE_H
#define LINE_SHAPE_H

#include "connectable_shape.h"

// 线段图形。端点以本地坐标保存，对外提供场景坐标接口。
class LineShape : public ConnectableShape {
protected:
  // 线段端点的本地坐标。这样移动图形项不会破坏线段自身几何。
  QPointF startPoint;
  QPointF endPoint;

  // 根据两个场景端点重新计算本地端点、位置、宽度和高度。
  void syncGeometry(QPointF sceneStart, QPointF sceneEnd,
                    bool notify = true);

  // 复制线段的本地端点数据。
  void copyLineGeometryTo(LineShape &shape) const;

public:
  // 使用两个场景端点创建线段。
  LineShape(QPointF start, QPointF end);

  // 创建当前线段的深拷贝。
  Shape *clone() const override;

  // 返回线段的本地路径。
  QPainterPath localGeometryPath() const override;

  // 线段不是闭合节点，此函数仅提供兼容性的方向端点选择。
  QPointF boundaryPointAtAngle(qreal angleRadians) const override;

  // 返回起点的场景坐标。
  QPointF getStartPoint() const;

  // 使用场景坐标设置起点。
  void setStartPoint(QPointF point);

  // 使用 x、y 场景坐标设置起点。
  void setStartPoint(qreal x, qreal y);

  // 返回终点的场景坐标。
  QPointF getEndPoint() const;

  // 使用场景坐标设置终点。
  void setEndPoint(QPointF point);

  // 使用 x、y 场景坐标设置终点。
  void setEndPoint(qreal x, qreal y);

  // 一次性同时设置起点和终点，仅触发一次几何重计算与通知。
  void setEndpoints(QPointF start, QPointF end);

  // 屏蔽虚函数 setSize，线段的宽高仅作为端点的派生结果，禁止直接修改。
  void setSize(QSizeF size) override;
  void setSize(qreal width, qreal height) override;

  // 屏蔽旋转与缩放，线段的方向与长短完全由两个端点主宰。
  void setRotation(qreal rotation) override;
  void setScale(qreal scale) override;

protected:
  // 屏蔽外来通过鼠标或手柄造成的非法旋转缩放
  QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

  // 返回包含线宽和最小命中区域的包围盒。
  QRectF boundingRect() const override;

  // 绘制线段本身。
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;
};

#endif // LINE_SHAPE_H
