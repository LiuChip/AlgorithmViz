#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include "../core/canvas.h"

class QAction;
class QActionGroup;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    Canvas* canvas() const { return m_canvas; }

private slots:
    void onEditModeChanged(Canvas::EditMode mode);

private:
    void setupToolBar();

    Canvas *m_canvas = nullptr;
    QAction *m_selectAction = nullptr;
    QAction *m_connectAction = nullptr;
};

#endif // MAIN_WINDOW_H
