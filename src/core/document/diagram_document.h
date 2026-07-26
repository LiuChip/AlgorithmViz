#ifndef DIAGRAM_DOCUMENT_H
#define DIAGRAM_DOCUMENT_H

#include <QList>
#include <QString>
#include <QPointF>
#include <QSizeF>
#include <QRectF>
#include "../../shapes/shape.h"
#include "../../shapes/connector/connector.h"
#include "../../shapes/text_label.h"

// 提取公共属性以防止状态漂移
struct CommonShapeData {
    int id = -1;
    QString name;
    int layer = 0;
    bool locked = false;
    bool visible = true;
    Border border;
    FillStyle fillStyle;
    TextStyle textStyle;
};

// 对应普通图形的数据结构
struct ShapeData {
    CommonShapeData common;
    QString type;           // 类名，如 "RectShape", "EllipseShape", "LineShape", "TextLabel"
    QPointF position;
    QSizeF size;
    qreal rotation = 0.0;

    // 仅供 LineShape 及其子类使用
    QPointF startPoint;
    QPointF endPoint;

    // 仅供 TextLabel 使用
    TextLabel::TextLayoutMode textLayoutMode = TextLabel::TextLayoutMode::AutoSize;
};

// 对应连接线的数据结构
struct ConnectorData {
    CommonShapeData common;

    // 目标图元的 ID，-1 表示游离端点（Free）
    int startTargetId = -1;
    int endTargetId = -1;

    ConnectorAnchor::Mode startMode = ConnectorAnchor::Mode::Free;
    ConnectorAnchor::Mode endMode = ConnectorAnchor::Mode::Free;

    QPointF startFreePoint;
    QPointF endFreePoint;

    qreal startBoundaryAngle = 0.0;
    qreal endBoundaryAngle = 0.0;

    QPointF startInteriorNormalized;
    QPointF endInteriorNormalized;

    // 未声明端点样式的旧文档按普通连线处理，避免无意中出现箭头。
    Connector::EndStyle endStyle = Connector::EndStyle::None;
};

class UndoManager;

struct Diagnostic {
    enum Level { Info, Warning, Error };
    Level level;
    QString message;
    int lineNumber = -1;
};

struct LoadResult {
    bool success;
    QList<Diagnostic> diagnostics;
};

// 整个画板的数据快照
class DiagramDocument {
public:
    QList<ShapeData> shapes;
    QList<ConnectorData> connectors;
    QRectF canvasRect;  // 画布逻辑大小

    static DiagramDocument fromScene(const QGraphicsScene* scene);
    LoadResult applyToScene(QGraphicsScene* scene, UndoManager* undoManager) const;
};

#endif // DIAGRAM_DOCUMENT_H
