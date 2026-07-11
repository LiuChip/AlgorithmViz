#include "shape.h"

int Shape::IdVal = 0;
int Shape::Layer = 0;

void Shape::setID(){id = IdVal++;}

// === Position & Size ===
QPointF Shape::getPosition() const {return QPointF(x,y);}
void Shape::setPosition(QPointF point) { x = point.x(); y = point.y(); update(); }
void Shape::setPosition(qreal x, qreal y) { this->x = x; this->y = y; update(); }

QSizeF Shape::getSize() const{return QSizeF(width,height);}
void Shape::setSize(QSizeF size){width=size.width();height=size.height();update();}
void Shape::setSize(qreal width,qreal height){this->width=width;this->height=height;update();}

qreal Shape::getRotation()const{return rotation;}
void Shape::setRotation(qreal rotation){this->rotation=rotation;update();}

// === Border ===
Border Shape::getBorderInfo() const{return border;}
void Shape::setBorderInfo(Border border){this->border=border;update();}
void Shape::setBorderInfo(){border=Border();update();}

// === Fill ===
FillStyle Shape::getFillInfo() const{return fillStyle;}
void Shape::setFillInfo(){fillStyle=FillStyle();update();}
void Shape::setFillInfo(FillStyle fillStyle){this->fillStyle=fillStyle;update();}

// === Text ===
TextStyle Shape::getTextInfo() const{return textStyle;}
void Shape::setTextInfo(){textStyle=TextStyle();update();}
void Shape::setTextInfo(TextStyle textStyle){this->textStyle=textStyle;update();}

// === ID ===
int Shape::getID() const{return id;}

// === Name ===
QString Shape::getName() const{return name;}
void Shape::setName(QString name){this->name=name;}

// === Visible ===
bool Shape::getVisible() const{return visible;}
void Shape::changeVisibility(){visible=!visible;setVisible(visible);update();}

// === Layer ===
int Shape::getLayer() const{return layer;}
void Shape::setLayer(bool upper){
    if(upper) layer = ++Layer;
    else if(layer>0) layer--;
    setZValue(layer);
    update();
}
void Shape::setLayer(int layer){this->layer=layer;setZValue(layer);update();}

// === Lock ===
bool Shape::getLockStat() const{return lock_stat;}
void Shape::changeLockStat(){
    lock_stat=!lock_stat;
    setFlag(ItemIsMovable,!lock_stat);
    setFlag(ItemIsSelectable,!lock_stat);
    update();
}

// === Constructor ===
Shape::Shape(){setID();}
Shape::Shape(qreal x, qreal y, qreal width, qreal height)
    : x(x), y(y), width(width), height(height){setID();}
Shape::Shape(QPointF point,QSizeF size)
    : x(point.x()), y(point.y()), width(size.width()), height(size.height()){setID();}
