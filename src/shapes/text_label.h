#ifndef TEXT_LABEL_H
#define TEXT_LABEL_H

#include "shape.h"

// 文本框图形，不支持锚点，支持自动尺寸和固定尺寸两种布局模式。
class TextLabel : public Shape {
public:
  // 文本框的尺寸计算模式。
  enum class TextLayoutMode {
    // 根据文本内容和字体自动计算宽高。
    AutoSize,

    // 使用用户指定的宽高，文本仅在宽度范围内换行。
    FixedSize,
  };

  // 使用场景坐标和文本内容创建文本框，默认使用 AutoSize。
  TextLabel(qreal x, qreal y, const QString &text);

  // 使用点和文本内容创建文本框，默认使用 AutoSize。
  TextLabel(QPointF point, const QString &text);

  // 创建当前文本框的深拷贝。
  Shape *clone() const override;

  // 设置文本内容，并按当前布局模式刷新几何尺寸。
  void setText(const QString &text);

  // 返回当前文本内容。
  QString getText() const;

  // 返回当前文本布局模式。
  TextLayoutMode getTextLayoutMode() const;

  // 设置文本布局模式，并在需要时重新计算尺寸。
  void setTextLayoutMode(TextLayoutMode mode);

  // 设置文本框尺寸，同时切换到 FixedSize 模式。
  void setSize(QSizeF size) override;

  // 设置文本框宽高，同时切换到 FixedSize 模式。
  void setSize(qreal width, qreal height) override;

  // 设置文本样式，并在 AutoSize 模式下重新计算尺寸。
  void setTextInfo(TextStyle textStyle) override;

  // 重置文本样式，并在 AutoSize 模式下重新计算尺寸。
  void setTextInfo() override;

  // 设置锁定状态；解锁后 AutoSize 文本会重新计算尺寸。
  void setLocked(bool locked) override;

protected:
  // 返回包含边框宽度的文本框包围盒。
  QRectF boundingRect() const override;

  // 绘制文本框填充、边框和文本内容。
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;

private:
  TextLayoutMode textLayoutMode = TextLayoutMode::AutoSize;

  // 根据当前字体和文本内容计算 AutoSize 所需的尺寸。
  QSizeF calculateTextSize() const;

  // 根据当前布局模式刷新文本框的几何尺寸。
  void refreshTextGeometry();
};

#endif // TEXT_LABEL_H
