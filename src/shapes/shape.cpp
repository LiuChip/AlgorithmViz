#include "shape.h"
#include "shapes/connector/connector.h"
#include "shapes/line_shape.h"
#include <cmath>
#include <algorithm>
#include <QPainterPath>

int Shape::IdVal = 0;
int Shape::Layer = 0;

void Shape::setID() { id = IdVal++; }

QPointF Shape::getPosition() const { return pos(); }

bool Shape::setPosition(QPointF point) {
  if (lock_stat || !std::isfinite(point.x()) || !std::isfinite(point.y())) {
    return false;
  }
  setPos(point);
  update();
  return true;
}

bool Shape::setPosition(qreal x, qreal y) { return setPosition(QPointF(x, y)); }

QSizeF Shape::getSize() const { return QSizeF(width, height); }

bool Shape::setSize(QSizeF size) { return setSize(size.width(), size.height()); }

bool Shape::setSize(qreal newWidth, qreal newHeight) {
  if (!std::isfinite(newWidth) || !std::isfinite(newHeight) || newWidth < 0.0 || newHeight < 0.0) {
    return false;
  }
  if (lock_stat || (this->width == newWidth && this->height == newHeight)) {
    return false;
  }

  // 保存修改前的几何中心在场景空间中的绝对位置
  QPointF oldCenterScene = mapToScene(QPointF(this->width * 0.5, this->height * 0.5));

  m_inGeometryChange = true;
  prepareGeometryChange();
  this->width = newWidth;
  this->height = newHeight;
  updateTransformOrigin(); // 保持旋转和缩放中心始终位于局部几何中心

  // 恢复场景空间中的几何中心点不变（保持自转和自缩放的几何不变量）
  QPointF newCenterScene = mapToScene(QPointF(this->width * 0.5, this->height * 0.5));
  if (newCenterScene != oldCenterScene) {
    setPos(pos() + (oldCenterScene - newCenterScene));
  }
  m_inGeometryChange = false;

  update();
  emit geometryChanged();
  return true;
}

qreal Shape::getRotation() const { return QGraphicsItem::rotation(); }

bool Shape::setRotation(qreal rotation) {
  if (lock_stat || !std::isfinite(rotation)) {
    return false;
  }
  QGraphicsItem::setRotation(rotation);
  update();
  return true;
}

bool Shape::setScale(qreal scale) {
  if (lock_stat || !std::isfinite(scale) || qFuzzyIsNull(scale) || scale <= 0.0) {
    return false;
  }
  QGraphicsItem::setScale(scale);
  update();
  return true;
}

Border Shape::getBorderInfo() const { return border; }

bool Shape::setBorderInfo(Border newBorder) {
  if (!std::isfinite(newBorder.borderWidth) || newBorder.borderWidth < 0.0) {
    return false;
  }
  bool changed = (this->border.borderWidth != newBorder.borderWidth) ||
                 (this->border.borderStyle != newBorder.borderStyle);
  if (changed) {
    prepareGeometryChange();
  }
  this->border = newBorder;
  update();
  if (changed) {
    emit geometryChanged();
  }
  return true;
}

bool Shape::setBorderInfo() {
  prepareGeometryChange();
  border = Border();
  update();
  emit geometryChanged();
  return true;
}

FillStyle Shape::getFillInfo() const { return fillStyle; }

bool Shape::setFillInfo() {
  fillStyle = FillStyle();
  update();
  return true;
}

bool Shape::setFillInfo(FillStyle newFillStyle) {
  if (!std::isfinite(newFillStyle.fillOpacity)) {
    return false;
  }
  newFillStyle.fillOpacity = std::max<qreal>(0.0, std::min<qreal>(1.0, newFillStyle.fillOpacity));
  this->fillStyle = newFillStyle;
  update();
  return true;
}

TextStyle Shape::getTextInfo() const { return textStyle; }

void Shape::setTextInfo() {
  textStyle = TextStyle();
  update();
}

void Shape::setTextInfo(TextStyle newTextStyle) {
  this->textStyle = newTextStyle;
  update();
}

int Shape::getID() const { return id; }

QString Shape::getName() const { return name; }

void Shape::setName(QString newName) { this->name = newName; }

bool Shape::getVisible() const { return isVisible(); }

void Shape::changeVisibility() {
  setVisible(!isVisible());
  update();
}

int Shape::getLayer() const { return layer; }

void Shape::setLayer(bool upper) {
  if (upper) {
    layer = ++Layer;
  } else if (layer > 0) {
    --layer;
  }
  setZValue(layer);
  update();
}

void Shape::setLayer(int newLayer) {
  this->layer = newLayer;
  Layer = std::max(Layer, newLayer);
  setZValue(newLayer);
  update();
}

bool Shape::getLockStat() const { return lock_stat; }

bool Shape::isLocked() const { return lock_stat; }

void Shape::setLocked(bool locked) {
  if (lock_stat == locked) return;
  lock_stat = locked;
  setFlag(ItemIsMovable, !locked);
  // A locked item remains selectable so it can be unlocked from the UI.
  setFlag(ItemIsSelectable, true);
  update();
  emit lockedChanged(locked);
}

void Shape::toggleLocked() { setLocked(!lock_stat); }

void Shape::changeLockStat() { toggleLocked(); }

Shape::Shape(qreal x, qreal y, qreal initialWidth, qreal initialHeight)
    : width(std::max<qreal>(0.0, std::isfinite(initialWidth) ? initialWidth : 0.0)),
      height(std::max<qreal>(0.0, std::isfinite(initialHeight) ? initialHeight : 0.0)) {
  setID();
  setPos(std::isfinite(x) ? x : 0.0, std::isfinite(y) ? y : 0.0);
  updateTransformOrigin();
  setFlag(ItemIsMovable, true);
  setFlag(ItemIsSelectable, true);
  setFlag(ItemSendsGeometryChanges, true);
}

Shape::Shape(QPointF point, QSizeF size)
    : width(std::max<qreal>(0.0, std::isfinite(size.width()) ? size.width() : 0.0)),
      height(std::max<qreal>(0.0, std::isfinite(size.height()) ? size.height() : 0.0)) {
  setID();
  setPos(std::isfinite(point.x()) && std::isfinite(point.y()) ? point : QPointF(0, 0));
  updateTransformOrigin();
  setFlag(ItemIsMovable, true);
  setFlag(ItemIsSelectable, true);
  setFlag(ItemSendsGeometryChanges, true);
}

void Shape::updateTransformOrigin() {
  setTransformOriginPoint(this->width * 0.5, this->height * 0.5);
}

QVariant Shape::itemChange(GraphicsItemChange change, const QVariant &value) {
  // 【第一道防线】无论是否锁定，都拦截非法的几何值（NaN / Inf / 非法缩放），
  // 防止通过 QGraphicsItem::setPos() 等原生非虚接口绕过 Shape 自有 setter 的校验。
  switch (change) {
  case ItemPositionChange: {
    QPointF pt = value.toPointF();
    if (!std::isfinite(pt.x()) || !std::isfinite(pt.y())) {
      return pos(); // 拒绝非法坐标，保持旧值
    }
    if (lock_stat) return pos();
    break;
  }
  case ItemRotationChange: {
    qreal r = value.toReal();
    if (!std::isfinite(r)) {
      return rotation();
    }
    if (lock_stat) return rotation();
    break;
  }
  case ItemScaleChange: {
    qreal s = value.toReal();
    if (!std::isfinite(s) || s <= 0.0) {
      return scale();
    }
    if (lock_stat) return scale();
    break;
  }
  case ItemTransformChange: {
    // QTransform 是 3x3 矩阵，检查其 6 个关键系数的有限性
    QTransform t = value.value<QTransform>();
    if (!std::isfinite(t.m11()) || !std::isfinite(t.m12()) ||
        !std::isfinite(t.m21()) || !std::isfinite(t.m22()) ||
        !std::isfinite(t.dx()) || !std::isfinite(t.dy())) {
      return transform();
    }
    if (lock_stat) return transform();
    break;
  }
  case ItemTransformOriginPointChange: {
    QPointF pt = value.toPointF();
    if (!std::isfinite(pt.x()) || !std::isfinite(pt.y())) {
      return transformOriginPoint();
    }
    if (lock_stat) return transformOriginPoint();
    break;
  }
  default:
    break;
  }

  // 如果图形的位置变化、旋转变化或变换完成，通知所关联的连接线或观察者。
  switch (change) {
  case ItemPositionHasChanged:
  case ItemRotationHasChanged:
  case ItemScaleHasChanged:
  case ItemTransformHasChanged:
  case ItemTransformOriginPointHasChanged:
    if (!m_inGeometryChange) {
      emit geometryChanged();
    }
    break;
  default:
    break;
  }

  return QGraphicsObject::itemChange(change, value);
}

void Shape::copyPropertiesTo(Shape &shape) const {
  const bool locked = this->lock_stat;

  // Copy the transform while the target is unlocked. The final lock state is
  // applied only after all geometry-related properties are restored.
  shape.setLocked(false);
  shape.width = this->width;
  shape.height = this->height;
  shape.border = this->border;
  shape.fillStyle = this->fillStyle;
  shape.textStyle = this->textStyle;
  shape.name = this->name;
  shape.layer = this->layer;
  shape.lock_stat = false;

  shape.setPos(this->pos());
  shape.setZValue(this->zValue());
  shape.setTransform(this->transform());
  shape.setVisible(this->isVisible());
  shape.setRotation(this->rotation());
  shape.setOpacity(this->opacity());
  shape.setEnabled(this->isEnabled());
  shape.setScale(this->scale());
  shape.setTransformOriginPoint(this->transformOriginPoint());
  shape.setLocked(locked);
}

void Shape::applyFillStyle(QPainter *painter) const {
  if (fillStyle.fillColor != Qt::transparent) {
    QColor fill = fillStyle.fillColor;
    fill.setAlphaF(static_cast<float>(fillStyle.fillOpacity));
    painter->setBrush(fill);
  } else {
    painter->setBrush(Qt::NoBrush);
  }
}

void Shape::applyBorderStyle(QPainter *painter) const {
  if (border.borderWidth > 0.0 && border.borderStyle != Qt::NoPen) {
    painter->setPen(
        QPen(border.borderColor, border.borderWidth, border.borderStyle));
  } else {
    painter->setPen(Qt::NoPen);
  }
}

void Shape::drawText(QPainter *painter, const QRectF &rect) const {
  if (textStyle.text.isEmpty()) {
    return;
  }

  painter->setPen(textStyle.textColor);
  painter->setFont(textStyle.font);
  const int textFlags = static_cast<int>(textStyle.alignment) | Qt::TextWordWrap;
  painter->drawText(rect, textFlags, textStyle.text);
}

void Shape::drawSelectionOutline(QPainter *painter) const {
  if (!isSelected()) {
    return;
  }
  painter->save();
  painter->setRenderHint(QPainter::Antialiasing, true);
  QPen dashPen(QColor(0, 120, 215), 1.5, Qt::DashLine);
  dashPen.setCosmetic(true);
  painter->setPen(dashPen);
  painter->setBrush(Qt::NoBrush);

  if (auto *conn = dynamic_cast<const Connector*>(this)) {
    painter->drawPath(conn->localGeometryPath());
  } else if (auto *line = dynamic_cast<const LineShape*>(this)) {
    painter->drawPath(line->localGeometryPath());
  } else {
    painter->drawRect(QRectF(0.0, 0.0, width, height));
  }
  painter->restore();
}
