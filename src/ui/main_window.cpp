#include "main_window.h"
#include "../core/canvas.h"
#include <QSplitter>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    auto *canvas = new Canvas(this);
    setCentralWidget(canvas);
    resize(1200, 800);
    setWindowTitle("AlgorithmViz");
}
