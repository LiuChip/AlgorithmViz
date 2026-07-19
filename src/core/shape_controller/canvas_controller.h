#ifndef CANVAS_CONTROLLER_H
#define CANVAS_CONTROLLER_H

#include <QObject>
#include <QPointF>

class Canvas;
class QMouseEvent;

class CanvasController : public QObject
{
    Q_OBJECT
public:
    explicit CanvasController(Canvas *canvas, QObject *parent = nullptr);
    ~CanvasController() override;

    bool handleMousePressEvent(QMouseEvent *event);
    bool handleMouseMoveEvent(QMouseEvent *event);
    bool handleMouseReleaseEvent(QMouseEvent *event);

    void cancelCurrentOperation();

public slots:
    void copy();
    void paste();
    void cut();
    void deleteSelected();
    void selectAll();

private:
    Canvas *m_canvas = nullptr;
};

#endif // CANVAS_CONTROLLER_H
