#include "text_label.h"

#include <algorithm>

#include <QFontMetricsF>
#include <QStringList>

TextLabel::TextLabel(qreal x, qreal y, const QString &text)
    : Shape(x, y, 0.0, 0.0) {
  setText(text);
}

TextLabel::TextLabel(QPointF point, const QString &text)
    : TextLabel(point.x(), point.y(), text) {}

void TextLabel::refreshTextGeometry() {
  if (getLockStat() || textLayoutMode != TextLayoutMode::AutoSize) {
    update();
    return;
  }

  const QSizeF size = calculateTextSize();
  if (width == size.width() && height == size.height()) {
    update();
    return;
  }

  prepareGeometryChange();
  width = size.width();
  height = size.height();
  update();
  emit geometryChanged();
}

void TextLabel::setText(const QString &text) {
  textStyle.text = text;
  refreshTextGeometry();
}

QString TextLabel::getText() const { return textStyle.text; }

TextLabel::TextLayoutMode TextLabel::getTextLayoutMode() const {
  return textLayoutMode;
}

void TextLabel::setTextLayoutMode(TextLayoutMode mode) {
  if (textLayoutMode == mode) {
    return;
  }

  textLayoutMode = mode;
  if (textLayoutMode == TextLayoutMode::AutoSize) {
    refreshTextGeometry();
  } else {
    update();
  }
}

void TextLabel::setSize(QSizeF size) {
  setSize(size.width(), size.height());
}

void TextLabel::setSize(qreal width, qreal height) {
  if (getLockStat()) {
    return;
  }

  textLayoutMode = TextLayoutMode::FixedSize;
  Shape::setSize(width, height);
}

void TextLabel::setTextInfo(TextStyle textStyle) {
  Shape::setTextInfo(std::move(textStyle));
  refreshTextGeometry();
}

void TextLabel::setTextInfo() {
  Shape::setTextInfo();
  refreshTextGeometry();
}

void TextLabel::setLocked(bool locked) {
  Shape::setLocked(locked);
  if (!locked && textLayoutMode == TextLayoutMode::AutoSize) {
    refreshTextGeometry();
  }
}

QSizeF TextLabel::calculateTextSize() const {
  constexpr qreal horizontalPadding = 10.0;
  constexpr qreal verticalPadding = 6.0;

  const QFontMetricsF metrics(textStyle.font);
  const QStringList lines = textStyle.text.split('\n');
  qreal longestLine = 0.0;

  for (const QString &line : lines) {
    longestLine = std::max(longestLine, metrics.horizontalAdvance(line));
  }

  const qsizetype lineCount = std::max<qsizetype>(1, lines.size());
  return QSizeF(longestLine + horizontalPadding,
                metrics.lineSpacing() * lineCount + verticalPadding);
}

QRectF TextLabel::boundingRect() const {
  const qreal halfPen = border.borderWidth / 2.0;
  return QRectF(-halfPen, -halfPen, width + border.borderWidth,
                height + border.borderWidth);
}

void TextLabel::paint(QPainter *painter,
                      const QStyleOptionGraphicsItem *option,
                      QWidget *widget) {
  Q_UNUSED(option);
  Q_UNUSED(widget);

  const QRectF rect(0, 0, width, height);
  applyFillStyle(painter);
  applyBorderStyle(painter);
  painter->drawRect(rect);
  drawText(painter, rect);
}

Shape *TextLabel::clone() const {
  auto *cloned = new TextLabel(x(), y(), textStyle.text);
  cloned->textLayoutMode = textLayoutMode;
  copyPropertiesTo(*cloned);
  return cloned;
}
