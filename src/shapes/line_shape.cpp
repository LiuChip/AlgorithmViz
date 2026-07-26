#include "line_shape.h"

#include <cmath>

LineShape::LineShape(QPointF start, QPointF end)
    : ConnectableShape(0.0, 0.0, 0.0, 0.0) {
  syncGeometry(start, end, false);
}

void LineShape::syncGeometry(QPointF sceneStart, QPointF sceneEnd,
                             bool notify, ApplyMode mode) {
  if ((mode == ApplyMode::UserEdit && lock_stat) || !std::isfinite(sceneStart.x()) || !std::isfinite(sceneStart.y()) ||
      !std::isfinite(sceneEnd.x()) || !std::isfinite(sceneEnd.y())) {
    return;
  }

  const QPointF localStart = mapFromScene(sceneStart);
  const QPointF localEnd = mapFromScene(sceneEnd);
  const QRectF localRect(localStart, localEnd);
  const QRectF normalized = localRect.normalized();
  const QPointF localOrigin = normalized.topLeft();

  const QPointF parentZero = mapToParent(QPointF(0, 0));
  const QPointF parentOrigin = mapToParent(localOrigin);

  if (notify) {
    prepareGeometryChange();
  }

  // 临时阻止内部 itemChange 提前多余地广播未完成的半途信号
  bool wasSending = flags() & ItemSendsGeometryChanges;
  if (wasSending) setFlag(ItemSendsGeometryChanges, false);

  setPos(pos() + parentOrigin - parentZero);
  startPoint = localStart - localOrigin;
  endPoint = localEnd - localOrigin;
  width = normalized.width();
  height = normalized.height();
  update();

  if (wasSending) setFlag(ItemSendsGeometryChanges, true);

  // 当一切端点和新外框尺寸彻底安顿完毕，只在这里干净利落发唯一一次正式通知！
  if (notify) {
    emit geometryChanged();
  }
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

bool LineShape::setStartPoint(QPointF point, ApplyMode mode) {
  if (!std::isfinite(point.x()) || !std::isfinite(point.y())) return false;
  if (mode == ApplyMode::UserEdit && lock_stat) return false;
  syncGeometry(point, getEndPoint(), true, mode);
  return true;
}

bool LineShape::setStartPoint(qreal x, qreal y, ApplyMode mode) {
  return setStartPoint(QPointF(x, y), mode);
}

QPointF LineShape::getEndPoint() const { return mapToScene(endPoint); }

bool LineShape::setEndPoint(QPointF point, ApplyMode mode) {
  if (!std::isfinite(point.x()) || !std::isfinite(point.y())) return false;
  if (mode == ApplyMode::UserEdit && lock_stat) return false;
  syncGeometry(getStartPoint(), point, true, mode);
  return true;
}

bool LineShape::setEndPoint(qreal x, qreal y, ApplyMode mode) {
  return setEndPoint(QPointF(x, y), mode);
}

bool LineShape::setEndpoints(QPointF start, QPointF end, ApplyMode mode) {
  if (!std::isfinite(start.x()) || !std::isfinite(start.y()) ||
      !std::isfinite(end.x()) || !std::isfinite(end.y())) {
    return false;
  }
  if (mode == ApplyMode::UserEdit && lock_stat) return false;
  syncGeometry(start, end, true, mode);
  return true;
}

bool LineShape::setSize(QSizeF size, ApplyMode mode) { Q_UNUSED(size); Q_UNUSED(mode); return false; }
bool LineShape::setSize(qreal width, qreal height, ApplyMode mode) {
  Q_UNUSED(width);
  Q_UNUSED(height);
  Q_UNUSED(mode);
  return false;
}

bool LineShape::setRotation(qreal rotation, ApplyMode mode) {
  Q_UNUSED(rotation);
  Q_UNUSED(mode);
  return false;
}

bool LineShape::setScale(qreal scale, ApplyMode mode) {
  Q_UNUSED(scale);
  Q_UNUSED(mode);
  return false;
}

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
  drawSelectionOutline(painter);
}

QVariant LineShape::itemChange(GraphicsItemChange change, const QVariant &value) {
  // 禁止对本线段进行任何额外的旋转与缩放或者复合变换，形状完全由两头点操控
  if (change == ItemRotationChange || change == ItemScaleChange ||
      change == ItemTransformChange) {
    if (change == ItemRotationChange) return 0.0;
    if (change == ItemScaleChange) return 1.0;
    if (change == ItemTransformChange) return QTransform();
  }
  return ConnectableShape::itemChange(change, value);
}

Shape *LineShape::clone() const {
  auto *cloned = new LineShape(QPointF(), QPointF());
  copyConnectablePropertiesTo(*cloned);
  copyLineGeometryTo(*cloned);
  return cloned;
}
