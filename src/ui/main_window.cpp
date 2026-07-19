#include "main_window.h"
#include "../core/shape_controller/connector_controller.h"
#include <QToolBar>
#include <QActionGroup>
#include <QAction>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_canvas(new Canvas(this))
{
    setCentralWidget(m_canvas);
    resize(1200, 800);
    setWindowTitle("AlgorithmViz");

    setupToolBar();

    connect(m_canvas, &Canvas::editModeChanged, this, &MainWindow::onEditModeChanged);
}

void MainWindow::setupToolBar()
{
    auto *toolBar = addToolBar("Mode ToolBar");
    toolBar->setMovable(false);

    auto *modeGroup = new QActionGroup(this);
    modeGroup->setExclusive(true);

    m_selectAction = toolBar->addAction("Select Mode");
    m_selectAction->setCheckable(true);
    m_selectAction->setChecked(true);
    modeGroup->addAction(m_selectAction);

    m_connectAction = toolBar->addAction("Connect Mode");
    m_connectAction->setCheckable(true);
    modeGroup->addAction(m_connectAction);

    connect(m_selectAction, &QAction::triggered, this, [this]() {
        m_canvas->setEditMode(Canvas::EditMode::Select);
    });
    connect(m_connectAction, &QAction::triggered, this, [this]() {
        m_canvas->setEditMode(Canvas::EditMode::Connect);
    });
}

void MainWindow::onEditModeChanged(Canvas::EditMode mode)
{
    if (mode == Canvas::EditMode::Select && m_selectAction) {
        m_selectAction->setChecked(true);
    } else if (mode == Canvas::EditMode::Connect && m_connectAction) {
        m_connectAction->setChecked(true);
    }
}
