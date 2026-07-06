#ifndef CANVAS_H
#define CANVAS_H

#include <QGraphicsView>
#include <QGraphicsScene>

class Canvas : public QGraphicsView
{
    Q_OBJECT

public:
    explicit Canvas(QWidget *parent = nullptr);
    QGraphicsScene *scene() const { return m_scene; }

private:
    QGraphicsScene *m_scene;
};

#endif // CANVAS_H
