#include "shape.h"

int Shape::IdVal = 0;
int Shape::Layer = 0;

void Shape::setID()
{
    id = IdVal++;
}

QPointF Shape::getPosition() const
{
    return pos();
}

void Shape::setPosition(QPointF point)
{
    setPos(point);
    update();
}

void Shape::setPosition(qreal x, qreal y)
{
    setPos(x, y);
    update();
}

QSizeF Shape::getSize() const
{
    return QSizeF(width, height);
}

void Shape::setSize(QSizeF size)
{
    setSize(size.width(), size.height());
}

void Shape::setSize(qreal width, qreal height)
{
    if (this->width == width && this->height == height) {
        return;
    }

    prepareGeometryChange();
    this->width = width;
    this->height = height;
    update();
}

qreal Shape::getRotation() const
{
    return QGraphicsItem::rotation();
}

void Shape::setRotation(qreal rotation)
{
    QGraphicsItem::setRotation(rotation);
    update();
}

Border Shape::getBorderInfo() const
{
    return border;
}

void Shape::setBorderInfo(Border border)
{
    if (this->border.borderWidth != border.borderWidth) {
        prepareGeometryChange();
    }
    this->border = border;
    update();
}

void Shape::setBorderInfo()
{
    prepareGeometryChange();
    border = Border();
    update();
}

FillStyle Shape::getFillInfo() const
{
    return fillStyle;
}

void Shape::setFillInfo()
{
    fillStyle = FillStyle();
    update();
}

void Shape::setFillInfo(FillStyle fillStyle)
{
    this->fillStyle = fillStyle;
    update();
}

TextStyle Shape::getTextInfo() const
{
    return textStyle;
}

void Shape::setTextInfo()
{
    textStyle = TextStyle();
    update();
}

void Shape::setTextInfo(TextStyle textStyle)
{
    this->textStyle = textStyle;
    update();
}

int Shape::getID() const
{
    return id;
}

QString Shape::getName() const
{
    return name;
}

void Shape::setName(QString name)
{
    this->name = name;
}

bool Shape::getVisible() const
{
    return visible;
}

void Shape::changeVisibility()
{
    visible = !visible;
    setVisible(visible);
    update();
}

int Shape::getLayer() const
{
    return layer;
}

void Shape::setLayer(bool upper)
{
    if (upper) {
        layer = ++Layer;
    } else if (layer > 0) {
        --layer;
    }
    setZValue(layer);
    update();
}

void Shape::setLayer(int layer)
{
    this->layer = layer;
    setZValue(layer);
    update();
}

bool Shape::getLockStat() const
{
    return lock_stat;
}

void Shape::changeLockStat()
{
    lock_stat = !lock_stat;
    setFlag(ItemIsMovable, !lock_stat);
    setFlag(ItemIsSelectable, !lock_stat);
    update();
}

Shape::Shape(qreal x, qreal y, qreal width, qreal height)
    : width(width), height(height)
{
    setID();
    setPos(x, y);
}

Shape::Shape(QPointF point, QSizeF size)
    : width(size.width()), height(size.height())
{
    setID();
    setPos(point);
}
