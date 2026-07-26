#include <QApplication>
#include "ui/main_window.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("AlgorithmViz"));
    QApplication::setApplicationName(QStringLiteral("AlgorithmViz"));
    QApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    MainWindow window;
    window.show();
    return app.exec();
}
