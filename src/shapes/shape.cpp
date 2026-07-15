#include "shape.h"

int Shape::IdVal = 0;
int Shape::Layer = 0;

void Shape::setID() { id = IdVal++; }

QPointF Shape::getPosition() const { return pos(); }

void Shape::setPosition(QPointF point) {
  if (lock_stat) {
    return;
  }
  setPos(point);
  update();
}

void Shape::setPosition(qreal x, qreal y) { setPosition(QPointF(x, y)); }

QSizeF Shape::getSize() const { return QSizeF(width, height); }

void Shape::setSize(QSizeF size) { setSize(size.width(), size.height()); }

void Shape::setSize(qreal width, qreal height) {
  if (lock_stat || (this->width == width && this->height == height)) {
    return;
  }

  prepareGeometryChange();
  this->width = width;
  this->height = height;
  update();
  emit geometryChanged();
}

qreal Shape::getRotation() const { return QGraphicsItem::rotation(); }

void Shape::setRotation(qreal rotation) {
  if (lock_stat) {
    return;
  }
  QGraphicsItem::setRotation(rotation);
  update();
}

void Shape::setScale(qreal scale) {
  if (lock_stat) {
    return;
  }
  QGraphicsItem::setScale(scale);
  update();
}

Border Shape::getBorderInfo() const { return border; }

void Shape::setBorderInfo(Border border) {
  bool changed = (this->border.borderWidth != border.borderWidth);
  if (changed) {
    prepareGeometryChange();
  }
  this->border = border;
  update();
  if (changed) {
    emit geometryChanged();
  }
}

void Shape::setBorderInfo() {
  prepareGeometryChange();
  border = Border();
  update();
  emit geometryChanged();
}

FillStyle Shape::getFillInfo() const { return fillStyle; }

void Shape::setFillInfo() {
  fillStyle = FillStyle();
  update();
}

void Shape::setFillInfo(FillStyle fillStyle) {
  this->fillStyle = fillStyle;
  update();
}

TextStyle Shape::getTextInfo() const { return textStyle; }

void Shape::setTextInfo() {
  textStyle = TextStyle();
  update();
}

void Shape::setTextInfo(TextStyle textStyle) {
  this->textStyle = textStyle;
  update();
}

int Shape::getID() const { return id; }

QString Shape::getName() const { return name; }

void Shape::setName(QString name) { this->name = name; }

bool Shape::getVisible() const { return visible; }

void Shape::changeVisibility() {
  visible = !visible;
  setVisible(visible);
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

void Shape::setLayer(int layer) {
  this->layer = layer;
  setZValue(layer);
  update();
}

bool Shape::getLockStat() const { return lock_stat; }

bool Shape::isLocked() const { return lock_stat; }

void Shape::setLocked(bool locked) {
  lock_stat = locked;
  setFlag(ItemIsMovable, !locked);
  // A locked item remains selectable so it can be unlocked from the UI.
  setFlag(ItemIsSelectable, true);
  update();
}

void Shape::toggleLocked() { setLocked(!lock_stat); }

void Shape::changeLockStat() { toggleLocked(); }

Shape::Shape(qreal x, qreal y, qreal width, qreal height)
    : width(width), height(height) {
  setID();
  setPos(x, y);
  setFlag(ItemIsMovable, true);
  setFlag(ItemIsSelectable, true);
  setFlag(ItemSendsGeometryChanges, true);
}

Shape::Shape(QPointF point, QSizeF size)
    : width(size.width()), height(size.height()) {
  setID();
  setPos(point);
  setFlag(ItemIsMovable, true);
  setFlag(ItemIsSelectable, true);
  setFlag(ItemSendsGeometryChanges, true);
}

QVariant Shape::itemChange(GraphicsItemChange change, const QVariant &value) {
  if (lock_stat) {
    switch (change) {
    case ItemPositionChange:
      return pos();
    case ItemRotationChange:
      return rotation();
    case ItemScaleChange:
      return scale();
    case ItemTransformChange:
      return transform();
    case ItemTransformOriginPointChange:
      return transformOriginPoint();
    default:
      break;
    }
  }

  // 如果图形的位置变化、旋转变化或变换完成，通知所关联的连接线或观察者。
  switch (change) {
  case ItemPositionHasChanged:
  case ItemRotationHasChanged:
  case ItemScaleHasChanged:
  case ItemTransformHasChanged:
  case ItemTransformOriginPointHasChanged:
    emit geometryChanged();
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
  shape.visible = this->visible;
  shape.layer = this->layer;
  shape.lock_stat = false;

  shape.setPos(this->pos());
  shape.setZValue(this->zValue());
  shape.setTransform(this->transform());
  shape.setVisible(this->visible);
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
    fill.setAlphaF(fillStyle.fillOpacity);
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
  const int textFlags = textStyle.alignment | Qt::TextWordWrap;
  painter->drawText(rect, textFlags, textStyle.text);
}
