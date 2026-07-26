#ifndef UNDO_COMMANDS_H
#define UNDO_COMMANDS_H

#include <QUndoCommand>
#include <QPointer>
#include <QGraphicsScene>
#include <QList>
#include <variant>
#include "shapes/shape.h"
#include "shapes/line_shape.h"
#include "shapes/connector/connector.h"
#include "shapes/text_label.h"
#include "core/layout_engine/layout_engine.h"


// ---------------------------------------------------------
// ShapePropertyCommand
// ---------------------------------------------------------
// 将属性面板、图层列表和菜单中的单项属性修改统一纳入撤销栈。
class ShapePropertyCommand : public QUndoCommand {
public:
    enum class Property {
        Name, Locked, Visible, Layer, BorderStyle, Fill, Text, TextLayoutMode,
        EndStyle
    };
    using Value = std::variant<QString, bool, int, Border, FillStyle, TextStyle,
                               TextLabel::TextLayoutMode, Connector::EndStyle>;

    ShapePropertyCommand(Shape *shape, Property property, Value oldValue,
                         Value newValue, const QString &description,
                         QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

private:
    void apply(const Value &value);

    QPointer<Shape> m_shape;
    Property m_property;
    Value m_oldValue;
    Value m_newValue;
};

// ---------------------------------------------------------
// LayoutSnapshotCommand
// ---------------------------------------------------------
// 将 LayoutEngine 计算出的快照作为一个原子操作提交，失败时由引擎回滚。
class LayoutSnapshotCommand : public QUndoCommand {
public:
    explicit LayoutSnapshotCommand(LayoutSnapshot snapshot,
                                   const QString &description = QStringLiteral("Arrange Items"),
                                   QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

private:
    static LayoutSnapshot reversed(const LayoutSnapshot &snapshot);
    LayoutSnapshot m_snapshot;
};

// ---------------------------------------------------------
// CreateShapeCommand
// ---------------------------------------------------------
class CreateShapeCommand : public QUndoCommand {
public:
    CreateShapeCommand(Shape* shape, QGraphicsScene* scene, QUndoCommand* parent = nullptr);
    ~CreateShapeCommand() override;

    void undo() override;
    void redo() override;

private:
    Shape* m_shape;
    QGraphicsScene* m_scene;
    bool m_ownsShape;
};

// ---------------------------------------------------------
// DeleteItemsCommand
// ---------------------------------------------------------
class DeleteItemsCommand : public QUndoCommand {
public:
    DeleteItemsCommand(const QList<Shape*>& shapes, QGraphicsScene* scene, QUndoCommand* parent = nullptr);
    ~DeleteItemsCommand() override;

    void undo() override;
    void redo() override;

private:
    struct ConnectorSnapshot {
        Connector* connector;
        ConnectorAnchor startAnchor;
        ConnectorAnchor endAnchor;
    };

    QList<Shape*> m_shapes;
    QList<ConnectorSnapshot> m_cascadeConnectors;
    QGraphicsScene* m_scene;
    bool m_ownsShapes;
};

// ---------------------------------------------------------
// MoveItemsCommand
// ---------------------------------------------------------
class MoveItemsCommand : public QUndoCommand {
public:
    struct MoveData {
        QPointer<Shape> shape;
        QPointF oldPos;
        QPointF newPos;
    };

    explicit MoveItemsCommand(const QList<MoveData>& moves, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    QList<MoveData> m_moves;
};

// ---------------------------------------------------------
// ResizeItemCommand
// ---------------------------------------------------------
class ResizeItemCommand : public QUndoCommand {
public:
    ResizeItemCommand(Shape* shape, QSizeF oldSize, QSizeF newSize, QPointF oldPos, QPointF newPos, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    QPointer<Shape> m_shape;
    QSizeF m_oldSize, m_newSize;
    QPointF m_oldPos, m_newPos;
};

// ---------------------------------------------------------
// RotateItemCommand
// ---------------------------------------------------------
class RotateItemCommand : public QUndoCommand {
public:
    RotateItemCommand(Shape* shape, qreal oldAngle, qreal newAngle, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    QPointer<Shape> m_shape;
    qreal m_oldAngle, m_newAngle;
};

// ---------------------------------------------------------
// MoveEndpointCommand
// ---------------------------------------------------------
class MoveEndpointCommand : public QUndoCommand {
public:
    MoveEndpointCommand(LineShape* line, QPointF oldStart, QPointF oldEnd, QPointF newStart, QPointF newEnd, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    QPointer<LineShape> m_line;
    QPointF m_oldStart, m_oldEnd;
    QPointF m_newStart, m_newEnd;
};

// ---------------------------------------------------------
// CreateConnectorCommand
// ---------------------------------------------------------
class CreateConnectorCommand : public QUndoCommand {
public:
    CreateConnectorCommand(Connector* connector, QGraphicsScene* scene, QUndoCommand* parent = nullptr);
    ~CreateConnectorCommand() override;

    void undo() override;
    void redo() override;

private:
    Connector* m_connector;
    QGraphicsScene* m_scene;
    bool m_ownsConnector;
};

// ---------------------------------------------------------
// ModifyConnectorCommand
// ---------------------------------------------------------
class ModifyConnectorCommand : public QUndoCommand {
public:
    enum class Endpoint { Start, End };
    ModifyConnectorCommand(Connector* connector, Endpoint ep, ConnectorAnchor oldAnchor, ConnectorAnchor newAnchor, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

private:
    QPointer<Connector> m_connector;
    Endpoint m_endpoint;
    ConnectorAnchor m_oldAnchor, m_newAnchor;
};

#endif // UNDO_COMMANDS_H
