#include "svg_exporter.h"
#include "scene_export_utils.h"

#include <QGraphicsScene>
#include <QPainter>
#include <QSaveFile>
#include <QSvgGenerator>

#include <algorithm>
#include <cmath>

namespace {
void setError(QString *errorMessage, const QString &message)
{
    if (errorMessage)
        *errorMessage = message;
}
} // namespace

bool SvgExporter::exportScene(QGraphicsScene *scene,
                              const QString &filePath,
                              QString *errorMessage,
                              qreal margin,
                              int maxOutputDimension)
{
    if (!scene) {
        setError(errorMessage, QStringLiteral("没有可导出的画布场景。"));
        return false;
    }
    if (filePath.trimmed().isEmpty()) {
        setError(errorMessage, QStringLiteral("导出路径不能为空。"));
        return false;
    }
    if (!std::isfinite(margin) || margin < 0.0 || maxOutputDimension <= 0) {
        setError(errorMessage, QStringLiteral("SVG 导出参数无效。"));
        return false;
    }

    SceneExport::ScopedOverlayHider overlayHider(scene);
    QRectF sourceRect = SceneExport::visibleItemsBoundingRect(scene);
    if (!sourceRect.isValid() || sourceRect.isEmpty())
        sourceRect = QRectF(0.0, 0.0, 1280.0, 720.0);
    sourceRect.adjust(-margin, -margin, margin, margin);

    const QSize outputSize = SceneExport::boundedOutputSize(sourceRect.size(), 1.0,
                                                            maxOutputDimension);
    if (outputSize.isEmpty()) {
        setError(errorMessage, QStringLiteral("画布边界无效，无法导出 SVG。"));
        return false;
    }

    QSaveFile output(filePath);
    if (!output.open(QIODevice::WriteOnly)) {
        setError(errorMessage, output.errorString());
        return false;
    }

    QSvgGenerator generator;
    generator.setOutputDevice(&output);
    generator.setSize(outputSize);
    generator.setViewBox(QRect(QPoint(0, 0), outputSize));
    generator.setTitle(QStringLiteral("AlgorithmViz Diagram"));
    generator.setDescription(QStringLiteral("Vector diagram exported by AlgorithmViz"));
    generator.setResolution(96);

    QPainter painter;
    if (!painter.begin(&generator)) {
        output.cancelWriting();
        setError(errorMessage, QStringLiteral("无法初始化 SVG 绘制设备。"));
        return false;
    }
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    scene->render(&painter,
                  QRectF(QPointF(0.0, 0.0), QSizeF(outputSize)),
                  sourceRect,
                  Qt::KeepAspectRatio);
    painter.end();

    if (!output.commit()) {
        setError(errorMessage, output.errorString());
        return false;
    }

    if (errorMessage)
        errorMessage->clear();
    return true;
}
