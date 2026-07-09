#include <Qt>
#include <QColor>
#include <QString>
#include <QVector>
#include <QPointF>

class Shape
{
protected:
    //Positon and Size
    qreal x,y;
    qreal width,length;
    qreal rotation = 0.0;

    //Border Style
    qreal borderWidth = 0.0;
    QColor borderColor = 0.0;
    Qt::PenStyle borderStyle;

    //Filling Style
    QColor fillColor = Qt::transparent;
    qreal fillOpacity = 1.0; //transparency

    //Text Style
    QString text = "";
    qreal fontSize = 16;
    QString fontFamily = "Arial";
    int fontWeight;
    QColor textColor = Qt::black;
    Qt::AlignmentFlag textAligh = Qt::AlignCenter;

    //Indentify
    int id = -1;
    QString name = "";
    bool visible = true;
    int layer;
    bool lock_stat = false;

    //Link Stat
    QVector<QPointF> anchorPoints;

public:
    QPointF getPosition();

};
