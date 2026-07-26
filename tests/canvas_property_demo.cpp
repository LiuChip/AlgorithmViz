#include <QApplication>
#include <QMainWindow>
#include <QHBoxLayout>
#include <QDebug>
#include "../src/ui/toolbar_widget.h"
#include "../src/ui/canvas_widget.h"
#include "../src/ui/property_panel_widget.h"
#include "../src/shapes/rect_shape.h"
#include "../src/shapes/ellipse_shape.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QMainWindow window;
    window.setWindowTitle("AlgorithmViz 独立工作区测试与调试 (Canvas & Property Panel)");
    window.setStyleSheet("QMainWindow { background-color: #f0f2f5; }");
    window.resize(1100, 700);

    QWidget *centralWidget = new QWidget(&window);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(4, 16, 16, 16);
    mainLayout->setSpacing(4);

    // 1. Left Toolbar
    ToolBarWidget *toolbar = new ToolBarWidget(centralWidget);
    mainLayout->addWidget(toolbar);

    // 2. Middle Canvas Container
    CanvasWidget *canvasWidget = new CanvasWidget(centralWidget);
    mainLayout->addWidget(canvasWidget, 1); // Expand to fill middle area

    // 3. Right Property Panel
    PropertyPanelWidget *propPanel = new PropertyPanelWidget(centralWidget);
    propPanel->connectToCanvas(canvasWidget->canvas());
    mainLayout->addWidget(propPanel);

    // Bind toolbar clicks directly to canvas drawing modes!
    QObject::connect(toolbar, &ToolBarWidget::toolChanged, canvasWidget, &CanvasWidget::setToolFromToolbar);
    
    // When canvas resets tool mode back to select (e.g. after shape creation), sync toolbar
    QObject::connect(canvasWidget->canvas(), &Canvas::toolModeChanged, toolbar, [toolbar](Canvas::ToolMode m) {
        if (m == Canvas::ToolMode::Select) {
            toolbar->setCurrentTool(ToolType::Select);
        }
    });

    // Create some initial sample shapes on canvas for quick testing
    if (canvasWidget->canvas() && canvasWidget->canvas()->scene()) {
        RectShape* sampleRect = new RectShape(150, 120, 160, 100);
        sampleRect->setName("初始基础矩形");
        sampleRect->setFillInfo(FillStyle(QColor("#ecf5ff"), 0.8));
        sampleRect->setBorderInfo(Border(2.0, QColor("#409eff"), Qt::SolidLine));
        sampleRect->setTextInfo(TextStyle("点击选中看属性", 14, "Arial", QFont::Medium, QColor("#1f2d3d"), Qt::AlignCenter));
        canvasWidget->canvas()->scene()->addItem(sampleRect);

        EllipseShape* sampleCircle = new EllipseShape(420, 200, 120, 120);
        sampleCircle->setName("样例圆球");
        sampleCircle->setFillInfo(FillStyle(QColor("#f0f9eb"), 0.9));
        sampleCircle->setBorderInfo(Border(2.5, QColor("#67c23a"), Qt::SolidLine));
        sampleCircle->setTextInfo(TextStyle("测试圆型", 14, "Arial", QFont::Bold, QColor("#2b4b1b"), Qt::AlignCenter));
        canvasWidget->canvas()->scene()->addItem(sampleCircle);
    }

    window.setCentralWidget(centralWidget);
    window.show();

    qDebug() << "=== Canvas & PropertyPanel 独立工作区测试应用启动 ===";
    qDebug() << "功能体验支持: 左侧选取工具直接往中间拖曳作画；鼠标单选任一图形右侧自动展示实时改写属性表单！";

    return app.exec();
}
