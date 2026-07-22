#include "main_window.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_canvas(new Canvas(this))
{
    setCentralWidget(m_canvas);
    resize(1200, 800);
    setWindowTitle("AlgorithmViz");
}
