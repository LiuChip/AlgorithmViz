#include "line_shape.h"

LineShape::LineShape(QPointF start, QPointF end)
    : Shape(QRectF(start, end).normalized().topLeft(),
            QRectF(start, end).normalized().size()),
      startPoint(start), endPoint(end) {}

void LineShape::syncGeometry(bool notify) {
  const QRectF rect(startPoint, endPoint);
  const QRectF normalized = rect.normalized();

  if (notify) {
    prepareGeometryChange();
  }

  setPos(normalized.topLeft());
  width = normalized.width();
  height = normalized.height();
  update();
}

QPointF LineShape::getStartPoint() const { return startPoint; }

void LineShape::setStartPoint(QPointF point) {
  startPoint = point;
  syncGeometry(true);
}

void LineShape::setStartPoint(qreal x, qreal y) {
  setStartPoint(QPointF(x, y));
}

QPointF LineShape::getEndPoint() const { return endPoint; }

void LineShape::setEndPoint(QPointF point) {
  endPoint = point;
  syncGeometry(true);
}

void LineShape::setEndPoint(qreal x, qreal y) { setEndPoint(QPointF(x, y)); }

QRectF LineShape::boundingRect() const {
  qreal halfPen = border.borderWidth / 2.0;
  return QRectF(-halfPen, -halfPen, width + border.borderWidth,
                height + border.borderWidth);
}

void LineShape::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                      QWidget *widget) {
  Q_UNUSED(option);
  Q_UNUSED(widget);

  if (border.borderWidth > 0.0 && border.borderStyle != Qt::NoPen) {
    QPen pen(border.borderColor, border.borderWidth, border.borderStyle);
    painter->setPen(pen);
  } else {
    painter->setPen(Qt::NoPen);
  }

  const QPointF localStart = startPoint - pos();
  const QPointF localEnd = endPoint - pos();
  painter->drawLine(localStart, localEnd);
}
