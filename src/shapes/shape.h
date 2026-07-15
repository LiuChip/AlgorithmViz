#ifndef SHAPE_H
#define SHAPE_H

#include <QColor>
#include <QFont>
#include <QGraphicsObject>
#if __has_include(<QGraphicsItem>)
#include <QGraphicsItem>
#endif
#include <QPainter>
#include <QPointF>
#include <QSizeF>
#include <QTransform>
#include <QVariant>
#include <QString>
#include <Qt>
#include <utility>

// 描述图形边框的宽度、颜色和线型。
struct Border {
  qreal borderWidth = 0.0;
  QColor borderColor = Qt::transparent;
  Qt::PenStyle borderStyle = Qt::NoPen;

  // 使用默认的透明、无边框样式创建 Border。
  Border() = default;

  // 使用指定的边框属性创建 Border。
  Border(qreal borderWidth, QColor borderColor, Qt::PenStyle borderStyle)
      : borderWidth(borderWidth), borderColor(borderColor),
        borderStyle(borderStyle) {}
};

// 描述图形填充颜色以及填充透明度。
struct FillStyle {
  QColor fillColor = Qt::transparent;
  qreal fillOpacity = 1.0;

  // 使用默认的透明填充创建 FillStyle。
  FillStyle() = default;

  // 使用指定颜色和透明度创建 FillStyle。
  FillStyle(QColor fillColor, qreal fillOpacity)
      : fillColor(fillColor), fillOpacity(fillOpacity) {}
};

// 描述图形中的文本内容、字体、颜色和对齐方式。
struct TextStyle {
  QString text;
  QFont font = QFont(QStringLiteral("Arial"), 16);
  QColor textColor = Qt::black;
  Qt::Alignment alignment = Qt::AlignCenter;

  // 使用默认字体、黑色文字和居中对齐创建 TextStyle。
  TextStyle() = default;

  // 使用指定的文本和字体属性创建 TextStyle。
  TextStyle(QString text, qreal fontSize, QString fontFamily, int fontWeight,
            QColor textColor, Qt::Alignment alignment)
      : text(std::move(text)), textColor(std::move(textColor)),
        alignment(alignment) {
    font.setFamily(std::move(fontFamily));
    font.setPointSizeF(fontSize);
    font.setWeight(static_cast<QFont::Weight>(fontWeight));
  }
};

// 所有图形的抽象基类，负责公共属性、绘制样式、变换、锁定和克隆接口。
class Shape : public QGraphicsObject {
  Q_OBJECT

protected:
  static int IdVal;
  static int Layer;

  qreal width = 0.0;
  qreal height = 0.0;

  Border border;
  FillStyle fillStyle;
  TextStyle textStyle;

  int id = -1;
  QString name = "";
  bool visible = true;
  int layer = 0;
  bool lock_stat = false;

  // 将当前 Shape 的公共属性复制到目标 Shape，但不复制 ID。
  void copyPropertiesTo(Shape &shape) const;

  // 拦截 QGraphicsItem 的几何变化通知，在锁定时阻止几何属性变化。
  virtual QVariant itemChange(GraphicsItemChange change,
                              const QVariant &value) override;

  // 根据当前 FillStyle 设置 QPainter 的画刷。
  void applyFillStyle(QPainter *painter) const;

  // 根据当前 Border 设置 QPainter 的画笔。
  void applyBorderStyle(QPainter *painter) const;

  // 按当前 TextStyle 在指定矩形内绘制文本，并支持自动换行。
  void drawText(QPainter *painter, const QRectF &rect) const;

private:
  // 为当前对象分配一个全局唯一的自增 ID。
  void setID();

public:
  // Shape 不允许无参构造，所有图形都必须明确给出位置和尺寸。
  Shape() = delete;

  // 创建位于 (x, y)、尺寸为 width x height 的图形。
  Shape(qreal x, qreal y, qreal width, qreal height);

  // 创建位于 point、尺寸为 size 的图形。
  Shape(QPointF point, QSizeF size);

  // 通过基类指针安全析构具体图形对象。
  virtual ~Shape() = default;

  // 返回图形在父项/场景坐标系中的位置。
  QPointF getPosition() const;

  // 设置图形在父项/场景坐标系中的位置；锁定时忽略请求。
  virtual void setPosition(QPointF point);

  // 使用 x、y 设置图形位置；锁定时忽略请求。
  virtual void setPosition(qreal x, qreal y);

  // 返回图形的逻辑尺寸，不包含旋转和缩放影响。
  QSizeF getSize() const;

  // 设置图形尺寸；具体子类可以重载以实现特殊尺寸逻辑。
  virtual void setSize(QSizeF size);

  // 使用 width、height 设置图形尺寸。
  virtual void setSize(qreal width, qreal height);

  // 返回图形的旋转角度。
  qreal getRotation() const;

  // 设置图形旋转角度；锁定时忽略请求。
  virtual void setRotation(qreal rotation);

  // 设置图形缩放比例；锁定时忽略请求。
  virtual void setScale(qreal scale);

  // 返回当前边框样式。
  Border getBorderInfo() const;

  // 设置边框样式，并在边框宽度变化时通知 Qt 更新几何区域。
  virtual void setBorderInfo(Border borderStyle);

  // 将边框恢复为默认的无边框样式。
  virtual void setBorderInfo();

  // 返回当前填充样式。
  FillStyle getFillInfo() const;

  // 将填充恢复为默认的透明填充。
  virtual void setFillInfo();

  // 设置填充样式。
  virtual void setFillInfo(FillStyle fillStyle);

  // 返回当前文本样式。
  TextStyle getTextInfo() const;

  // 将文本样式恢复为默认值。
  virtual void setTextInfo();

  // 设置文本样式。
  virtual void setTextInfo(TextStyle textStyle);

  // 返回对象的唯一 ID。
  int getID() const;

  // 返回对象名称。
  QString getName() const;

  // 设置对象名称。
  void setName(QString name = "");

  // 返回图形当前是否可见。
  bool getVisible() const;

  // 切换图形可见状态。
  void changeVisibility();

  // 返回图形当前图层值。
  int getLayer() const;

  // 将图形置于更高层或降低一层。
  void setLayer(bool upper = true);

  // 直接设置图形图层值。
  void setLayer(int layer);

  // 返回旧接口名称对应的锁定状态。
  bool getLockStat() const;

  // 返回图形是否处于锁定状态。
  bool isLocked() const;

  // 设置锁定状态；锁定只限制位置、尺寸和变换，不限制样式编辑。
  virtual void setLocked(bool locked);

  // 在锁定和未锁定之间切换。
  void toggleLocked();

  // 兼容旧接口，等价于 toggleLocked()。
  void changeLockStat();

  // 返回图形的局部包围盒，由具体图形实现。
  QRectF boundingRect() const override = 0;

  // 使用 QPainter 绘制图形，由具体图形实现。
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override = 0;

  // 创建当前图形的深拷贝，并为副本分配新的 ID。
  virtual Shape *clone() const = 0;

signals:
  // 当图形的位置、尺寸、旋转或几何形状发生变化时发出此信号。
  void geometryChanged();
};

#endif // SHAPE_H
