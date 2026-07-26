#include <QApplication>
#include <QMainWindow>
#include <QDebug>
#include <QLabel>
#include <QVBoxLayout>
#include "../src/ui/toolbar_widget.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QMainWindow window;
    window.setWindowTitle("Toolbar Widget & Flyout Demo");
    window.setStyleSheet("QMainWindow { background-color: #f0f2f5; }");

    // Central layout with toolbar and a status label
    QWidget *centralWidget = new QWidget(&window);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(4, 20, 20, 20);
    mainLayout->setSpacing(8);

    ToolBarWidget *toolbar = new ToolBarWidget(centralWidget);
    mainLayout->addWidget(toolbar);

    // Right-side instructional and status display area
    QFrame* infoBox = new QFrame(centralWidget);
    infoBox->setStyleSheet("QFrame { background-color: #ffffff; border: 1px solid #e2e4e8; border-radius: 12px; }");
    QVBoxLayout *infoLayout = new QVBoxLayout(infoBox);
    infoLayout->setContentsMargins(20, 20, 20, 20);

    QLabel *statusTitle = new QLabel("🎨 极简扁平化左侧工具栏调试控制台", infoBox);
    statusTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #1a1a1a; border: none;");
    
    QLabel *instructions = new QLabel(
        "交互操作指引：\n"
        "• 短按 (Click)：直接选中目标图元工具，激活专属亮天蓝色高亮外线框 (#009dff)。\n"
        "• 连线工具唤起菜单 (Flyout)：向左侧第二个线型图元上<b>按住 250ms 或按下拖曳</b>，\n"
        "  即刻渲染带优雅灰底阴影的弹出线形候选胶囊（普通直线 / 单箭头 / 双箭头）。\n"
        "• 磁吸感应选择：按住不放并在滑出的三样备选中滑动，高亮方框即时感应吸附，\n"
        "  <b>「鼠标移到哪个高亮，松开代表自动确认与回填」</b>！\n\n"
        "当前选定的图元工具为：",
        infoBox
    );
    instructions->setStyleSheet("font-size: 13px; color: #4b5563; border: none; line-height: 1.4;");
    
    QLabel *currentSelectedLabel = new QLabel("✦ Select (Cursor) [蓝色高亮狂奔状态]", infoBox);
    currentSelectedLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #009dff; border: none; padding-top: 10px;");

    infoLayout->addWidget(statusTitle);
    infoLayout->addSpacing(10);
    infoLayout->addWidget(instructions);
    infoLayout->addWidget(currentSelectedLabel);
    infoLayout->addStretch();

    mainLayout->addWidget(infoBox, 1);
    window.setCentralWidget(centralWidget);
    window.resize(600, 480);

    // Initial log message
    qDebug() << "==========================================================";
    qDebug() << "[Toolbar Demo 启动] 缺省默认工具 ID: Select (0) -> 蓝色框已初始点亮。";
    qDebug() << "操作提示: 点击按钮查看瞬时响应；对线形按钮深按或按住拖动可展现悬浮衍生弹窗。";
    qDebug() << "==========================================================";

    // Connect signals to log vivid feedback to both qDebug and interface
    QObject::connect(toolbar, &ToolBarWidget::toolClicked, [](ToolType type) {
        qDebug() << "👉 [Tool CLICKED (被单击捕获)]  用户刚直接敲定了图元工具:" 
                 << ToolBarWidget::toolName(type)
                 << "(Enum ID:" << static_cast<int>(type) << ")";
    });

    QObject::connect(toolbar, &ToolBarWidget::toolChanged, [currentSelectedLabel](ToolType type) {
        QString name = ToolBarWidget::toolName(type);
        qDebug() << "✨ [Tool CHANGED (被选中与提亮)] 目前全新主推选取的画笔工具为:" 
                 << name 
                 << "->【专属科技天蓝色 #009dff 高亮精工锁墙已经包裹此图标！】";
        currentSelectedLabel->setText(QString("✦ %1 [蓝色高亮锁墙当前运行]").arg(name));
    });

    window.show();
    return app.exec();
}
