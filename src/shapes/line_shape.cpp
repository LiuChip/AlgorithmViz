#include "line_shape.h"

#include <cmath>

LineShape::LineShape(QPointF start, QPointF end)
    : ConnectableShape(0.0, 0.0, 0.0, 0.0) {
  syncGeometry(start, end, false);
}

void LineShape::syncGeometry(QPointF sceneStart, QPointF sceneEnd,
                             bool notify) {
  if (lock_stat) {
    return;
  }

  // Convert the public scene coordinates into the current item coordinate
  // system first. This keeps endpoint setters correct even after the item has
  // been moved, rotated, scaled, or given a custom transform.
  const QPointF localStart = mapFromScene(sceneStart);
  const QPointF localEnd = mapFromScene(sceneEnd);
  const QRectF localRect(localStart, localEnd);
  const QRectF normalized = localRect.normalized();
  const QPointF localOrigin = normalized.topLeft();

  // Moving the local origin to (0, 0) changes the item-local coordinates. Move
  // the item by the corresponding parent-space delta so the actual scene
  // endpoints remain exactly at sceneStart and sceneEnd.
  const QPointF parentZero = mapToParent(QPointF(0, 0));
  const QPointF parentOrigin = mapToParent(localOrigin);

  if (notify) {
    prepareGeometryChange();
  }

  setPos(pos() + parentOrigin - parentZero);
  startPoint = localStart - localOrigin;
  endPoint = localEnd - localOrigin;
  width = normalized.width();
  height = normalized.height();
  update();
}

QPainterPath LineShape::localGeometryPath() const {
  QPainterPath path;
  path.moveTo(startPoint);
  path.lineTo(endPoint);
  return path;
}

QPointF LineShape::boundaryPointAtAngle(qreal angleRadians) const {
  // Lines are connectors rather than closed target shapes. This compatibility
  // implementation returns the endpoint lying farther along the requested
  // direction; closed node shapes provide the meaningful ray intersection.
  const QPointF center = (startPoint + endPoint) / 2.0;
  const QPointF direction(std::cos(angleRadians), std::sin(angleRadians));
  const qreal startProjection =
      QPointF::dotProduct(startPoint - center, direction);
  const qreal endProjection = QPointF::dotProduct(endPoint - center, direction);
  return startProjection >= endProjection ? startPoint : endPoint;
}

QPointF LineShape::getStartPoint() const { return mapToScene(startPoint); }

void LineShape::setStartPoint(QPointF point) {
  syncGeometry(point, getEndPoint());
}

void LineShape::setStartPoint(qreal x, qreal y) {
  setStartPoint(QPointF(x, y));
}

QPointF LineShape::getEndPoint() const { return mapToScene(endPoint); }

void LineShape::setEndPoint(QPointF point) {
  syncGeometry(getStartPoint(), point);
}

void LineShape::setEndPoint(qreal x, qreal y) { setEndPoint(QPointF(x, y)); }

void LineShape::copyLineGeometryTo(LineShape &shape) const {
  shape.startPoint = startPoint;
  shape.endPoint = endPoint;
}

QRectF LineShape::boundingRect() const {
  // ConnectableShape::shape() keeps a line selectable even without a visible
  // pen by using a one-unit hit stroke. Keep the required QGraphicsItem
  // boundingRect() margin in sync with that fallback stroke.
  const qreal halfPen = qMax<qreal>(border.borderWidth / 2.0, 0.5);
  return QRectF(-halfPen, -halfPen, width + 2.0 * halfPen,
                height + 2.0 * halfPen);
}

void LineShape::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                      QWidget *widget) {
  Q_UNUSED(option);
  Q_UNUSED(widget);

  applyBorderStyle(painter);
  painter->drawLine(startPoint, endPoint);
}

Shape *LineShape::clone() const {
  auto *cloned = new LineShape(QPointF(), QPointF());
  copyConnectablePropertiesTo(*cloned);
  copyLineGeometryTo(*cloned);
  return cloned;
}
