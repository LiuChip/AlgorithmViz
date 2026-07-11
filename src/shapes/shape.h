#ifndef SHAPE
#define SHAPE
#include <Qt>
#include <QColor>
#include <QString>
#include <QVector>
#include <QPointF>
#include <QSizeF>
#include <QFont>
#include <QGraphicsItem>
#include <QPainter>

struct Border{
    qreal borderWidth = 0.0;
    QColor borderColor = Qt::transparent;
    Qt::PenStyle borderStyle = Qt::NoPen;
    Border() = default;
    Border(qreal borderWidth,QColor borderColor,Qt::PenStyle borderStyle)
    {
        this->borderWidth=borderWidth;
        this->borderColor=borderColor;
        this->borderStyle=borderStyle;
    }
};

struct FillStyle{
    QColor fillColor = Qt::transparent;
    qreal fillOpacity = 1.0; //transparency
    FillStyle() = default;
    FillStyle(QColor fillColor,qreal fillOpacity)
    {
        this->fillColor=fillColor;
        this->fillOpacity=fillOpacity;
    }
};

struct TextStyle{
    QString text = "";
    qreal fontSize = 16;
    QString fontFamily = "Arial";
    int fontWeight = QFont::Normal;
    QColor textColor = Qt::black;
    Qt::AlignmentFlag textAligh = Qt::AlignCenter;
    TextStyle() = default;
    TextStyle(QString text,qreal fontSize,QString fontFamily,int fontWeight,QColor textColor,Qt::AlignmentFlag textAligh)
    {
        this->text=text;
        this->fontSize=fontSize;
        this->fontFamily=fontFamily;
        this->fontWeight=fontWeight;
        this->textColor=textColor;
        this->textAligh=textAligh;
    }
};

class Shape:public QGraphicsItem
{
protected:
    //Static
    static int IdVal;
    static int Layer;

    //Positon and Size
    qreal x,y;
    qreal width,height;
    qreal rotation = 0.0;

    //Border Style

    Border border;

    //Filling Style

    FillStyle fillStyle;

    //Text Style

    TextStyle textStyle;

    //Indentify
    int id = -1;
    QString name = "";
    bool visible = true;
    int layer = 0;
    bool lock_stat = false;

    //Link Stat
    QVector<QPointF> anchorPoints;

private:
    void setID();
public:

    //Getter & Setter
    QPointF getPosition() const;
    void setPosition(QPointF point);
    void setPosition(qreal x,qreal y);
    QSizeF getSize() const;
    void setSize(QSizeF size);
    void setSize(qreal width,qreal height);
    qreal getRotation() const;
    void setRotation(qreal rotation);

    Border getBorderInfo() const;
    void setBorderInfo(Border borderStyle);
    void setBorderInfo();

    FillStyle getFillInfo() const;
    void setFillInfo();
    void setFillInfo(FillStyle fillStyle);

    TextStyle getTextInfo() const;
    void setTextInfo();
    void setTextInfo(TextStyle textStyle);

    int getID() const;

    QString getName() const;
    void setName(QString name = "");

    bool getVisible() const;
    void changeVisibility();

    int getLayer() const;
    void setLayer(bool upper = true); //Shift Layer
    void setLayer(int layer);

    bool getLockStat() const;
    void changeLockStat();

    //Function
    QRectF boundingRect() const override = 0;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override = 0;

    //Constructor
    Shape();
    Shape(qreal x, qreal y, qreal width, qreal height);
    Shape(QPointF point,QSizeF size);
    virtual ~Shape() = default;

};

#endif