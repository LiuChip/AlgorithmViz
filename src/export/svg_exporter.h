#ifndef SVG_EXPORTER_H
#define SVG_EXPORTER_H

#include <QString>

class QGraphicsScene;

// 使用 Qt 的矢量绘制管线将场景导出为独立 SVG 文件。
// 该类不依赖 MainWindow，便于后续在批量导出、命令行或插件中复用。
class SvgExporter
{
public:
    // 导出场景中的顶层可见图元。成功返回 true；失败时通过 errorMessage 返回原因。
    // margin 是场景坐标中的留白，maxOutputDimension 仅限制 SVG 的建议显示尺寸，
    // 不会把内容栅格化，也不会损失矢量几何。
    static bool exportScene(QGraphicsScene *scene,
                            const QString &filePath,
                            QString *errorMessage = nullptr,
                            qreal margin = 24.0,
                            int maxOutputDimension = 4096);
};

#endif // SVG_EXPORTER_H
