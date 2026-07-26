#include <QtTest>

#include "export/scene_export_utils.h"
#include "export/svg_exporter.h"
#include "shapes/rect_shape.h"

#include <QBrush>
#include <QFile>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QTemporaryDir>

class SvgExporterTest : public QObject
{
    Q_OBJECT

private slots:
    void testExportsSceneAsVectorSvg()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString filePath = directory.filePath(QStringLiteral("diagram.svg"));

        QGraphicsScene scene;
        auto *rectangle = new RectShape(10.0, 20.0, 120.0, 70.0);
        rectangle->setName(QStringLiteral("Exported Rectangle"));
        scene.addItem(rectangle);

        QString errorMessage;
        QVERIFY2(SvgExporter::exportScene(&scene, filePath, &errorMessage),
                 qPrintable(errorMessage));

        QFile file(filePath);
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        const QByteArray svg = file.readAll();
        QVERIFY(svg.contains("<svg"));
        QVERIFY(svg.contains("<path") || svg.contains("<rect"));
        QVERIFY(!svg.contains("data:image/png"));
    }

    void testRejectsNullScene()
    {
        QString errorMessage;
        QVERIFY(!SvgExporter::exportScene(nullptr, QStringLiteral("unused.svg"), &errorMessage));
        QVERIFY(!errorMessage.isEmpty());
    }

    void testExcludesTransientSceneItems()
    {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString filePath = directory.filePath(QStringLiteral("without-overlays.svg"));

        QGraphicsScene scene;
        auto *content = scene.addRect(QRectF(0.0, 0.0, 80.0, 60.0),
                                      QPen(Qt::black), QBrush(Qt::green));
        QVERIFY(content);

        auto *overlay = scene.addRect(QRectF(2000.0, 2000.0, 500.0, 500.0),
                                      QPen(Qt::magenta), QBrush(Qt::magenta));
        overlay->setData(SceneExport::ExcludeFromExportRole, true);

        QString errorMessage;
        QVERIFY2(SvgExporter::exportScene(&scene, filePath, &errorMessage),
                 qPrintable(errorMessage));
        QVERIFY(overlay->isVisible());

        QFile file(filePath);
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        const QByteArray svg = file.readAll().toLower();
        QVERIFY(!svg.contains("#ff00ff"));
        QVERIFY(!svg.contains("width=\"4096\""));
        QVERIFY(!svg.contains("height=\"4096\""));
    }

    void testOutputSizeCapsVeryLargeScenes()
    {
        QCOMPARE(SceneExport::boundedOutputSize(QSizeF(20000.0, 10000.0), 2.0, 4096),
                 QSize(4096, 2048));
        QCOMPARE(SceneExport::boundedOutputSize(QSizeF(100.0, 50.0), 2.0, 4096),
                 QSize(200, 100));
        QVERIFY(SceneExport::boundedOutputSize(QSizeF(), 2.0, 4096).isEmpty());
    }
};

QTEST_MAIN(SvgExporterTest)
#include "svg_exporter_test.moc"
