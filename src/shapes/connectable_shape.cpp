#include "connectable_shape.h"

#include <QLineF>
#include <QPainterPathStroker>
#include <QtMath>

ConnectableShape::AnchorSpec ConnectableShape::AnchorSpec::createAnchor(
    qreal angleRadians) {
  AnchorSpec anchor;
  anchor.type = Type::BoundaryAngle;
  anchor.value.boundaryAngle = angleRadians;
  return anchor;
}

ConnectableShape::AnchorSpec ConnectableShape::AnchorSpec::createAnchor(
    QPointF normalizedPosition) {
  AnchorSpec anchor;
  anchor.type = Type::InteriorNormalized;
  anchor.value.interiorPosition.x = normalizedPosition.x();
  anchor.value.interiorPosition.y = normalizedPosition.y();
  return anchor;
}

qreal ConnectableShape::AnchorSpec::boundaryAngle() const {
  return type == Type::BoundaryAngle ? value.boundaryAngle : 0.0;
}

QPointF ConnectableShape::AnchorSpec::interiorPosition() const {
  if (type != Type::InteriorNormalized) {
    return QPointF();
  }
  return QPointF(value.interiorPosition.x, value.interiorPosition.y);
}

void ConnectableShape::copyConnectablePropertiesTo(
    ConnectableShape &shape) const {
  copyPropertiesTo(shape);
  shape.anchorPoints = anchorPoints;
}

QVector<ConnectableShape::AnchorSpec>
ConnectableShape::getAnchorPoints() const {
  return anchorPoints;
}

void ConnectableShape::setAnchorPoints(
    const QVector<AnchorSpec> &anchors) {
  anchorPoints = anchors;
  update();
}

QPainterPath ConnectableShape::shape() const {
  const QPainterPath geometry = localGeometryPath();
  if (geometry.isEmpty()) {
    return geometry;
  }

  // QGraphicsItem::shape() should remain usable for selection even when a
  // line has no visible pen. Closed geometry already contains its interior;
  // only add the actual border stroke there. Open geometry needs a minimum
  // stroke so a line remains selectable, and its boundingRect() provides the
  // corresponding margin in LineShape.
  bool closed = false;
  if (geometry.elementCount() >= 2) {
    const QPainterPath::Element first = geometry.elementAt(0);
    const QPainterPath::Element last =
        geometry.elementAt(geometry.elementCount() - 1);
    closed = QLineF(QPointF(first.x, first.y), QPointF(last.x, last.y))
                 .length() <= 1e-9;
  }

  if (closed && border.borderWidth <= 0.0) {
    return geometry;
  }

  QPainterPathStroker stroker;
  stroker.setWidth(closed ? border.borderWidth : qMax<qreal>(border.borderWidth, 1.0));
  return geometry.united(stroker.createStroke(geometry));
}

