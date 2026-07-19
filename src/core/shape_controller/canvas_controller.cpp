#include "canvas_controller.h"
#include "core/canvas.h"
#include <QMouseEvent>

CanvasController::CanvasController(Canvas *canvas, QObject *parent)
    : QObject(parent), m_canvas(canvas)
{
}

CanvasController::~CanvasController() = default;

bool CanvasController::handleMousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    return false;
}

bool CanvasController::handleMouseMoveEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    return false;
}

bool CanvasController::handleMouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    return false;
}

void CanvasController::cancelCurrentOperation()
{
}

void CanvasController::copy()
{
}

void CanvasController::paste()
{
}

void CanvasController::cut()
{
}

void CanvasController::deleteSelected()
{
}

void CanvasController::selectAll()
{
}
