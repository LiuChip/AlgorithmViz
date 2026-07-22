#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include "../core/canvas.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    Canvas* canvas() const { return m_canvas; }

private:
    Canvas *m_canvas = nullptr;
};

#endif // MAIN_WINDOW_H
