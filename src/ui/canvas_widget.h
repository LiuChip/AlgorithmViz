#ifndef CANVAS_WIDGET_H
#define CANVAS_WIDGET_H

#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include "../core/canvas.h"
#include "toolbar_widget.h"

class CanvasWidget : public QFrame
{
    Q_OBJECT
public:
    explicit CanvasWidget(QWidget *parent = nullptr);
    ~CanvasWidget() override;

    Canvas* canvas() const { return m_canvas; }

public slots:
    void setToolFromToolbar(ToolType tool);
    void updateZoomLabel();

private:
    Canvas* m_canvas = nullptr;
    QLabel* m_zoomLabel = nullptr;
    QLabel* m_coordsLabel = nullptr;
};

#endif // CANVAS_WIDGET_H
