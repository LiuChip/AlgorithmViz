#include "undo_commands.h"
#include <QGraphicsItem>
#include <utility>

// ---------------------------------------------------------
// ShapePropertyCommand
// ---------------------------------------------------------
ShapePropertyCommand::ShapePropertyCommand(Shape *shape, Property property,
                                           Value oldValue, Value newValue,
                                           const QString &description,
                                           QUndoCommand *parent)
    : QUndoCommand(parent), m_shape(shape), m_property(property),
      m_oldValue(std::move(oldValue)), m_newValue(std::move(newValue))
{
    setText(description);
}

void ShapePropertyCommand::undo()
{
    apply(m_oldValue);
}

void ShapePropertyCommand::redo()
{
    apply(m_newValue);
}

void ShapePropertyCommand::apply(const Value &value)
{
    if (!m_shape)
        return;

    switch (m_property) {
    case Property::Name:
        m_shape->setName(std::get<QString>(value));
        break;
    case Property::Locked:
        m_shape->setLocked(std::get<bool>(value));
        break;
    case Property::Visible:
        m_shape->setVisible(std::get<bool>(value));
        break;
    case Property::Layer:
        m_shape->setLayer(std::get<int>(value));
        break;
    case Property::BorderStyle:
        m_shape->setBorderInfo(std::get<Border>(value));
        break;
    case Property::Fill:
        m_shape->setFillInfo(std::get<FillStyle>(value));
        break;
    case Property::Text:
        m_shape->setTextInfo(std::get<TextStyle>(value));
        break;
    case Property::TextLayoutMode:
        if (auto *label = dynamic_cast<TextLabel *>(m_shape.data()))
            label->setTextLayoutMode(std::get<TextLabel::TextLayoutMode>(value));
        break;
    case Property::EndStyle:
        if (auto *connector = dynamic_cast<Connector *>(m_shape.data()))
            connector->setEndStyle(std::get<Connector::EndStyle>(value));
        break;
    }
}

// ---------------------------------------------------------
// LayoutSnapshotCommand
// ---------------------------------------------------------
LayoutSnapshotCommand::LayoutSnapshotCommand(LayoutSnapshot snapshot,
                                             const QString &description,
                                             QUndoCommand *parent)
    : QUndoCommand(parent), m_snapshot(std::move(snapshot))
{
    setText(description);
}

void LayoutSnapshotCommand::undo()
{
    LayoutEngine::applyLayoutSnapshot(reversed(m_snapshot));
}

void LayoutSnapshotCommand::redo()
{
    LayoutEngine::applyLayoutSnapshot(m_snapshot);
}

LayoutSnapshot LayoutSnapshotCommand::reversed(const LayoutSnapshot &snapshot)
{
    LayoutSnapshot result;
    result.reserve(snapshot.size());
    for (const LayoutItemChange &change : snapshot)
        result.append({change.shape, change.newPos, change.oldPos,
                       change.newSize, change.oldSize});
    return result;
}

// ---------------------------------------------------------
// CreateShapeCommand
// ---------------------------------------------------------
CreateShapeCommand::CreateShapeCommand(Shape* shape, QGraphicsScene* scene, QUndoCommand* parent)
    : QUndoCommand(parent), m_shape(shape), m_scene(scene), m_ownsShape(false)
{
    setText("Create Shape");
}

CreateShapeCommand::~CreateShapeCommand()
{
    // If we own the shape and it's not in the scene, delete it
    if (m_ownsShape && m_shape) {
        delete m_shape;
    }
}

void CreateShapeCommand::undo()
{
    if (m_shape && m_scene) {
        m_scene->removeItem(m_shape);
        m_ownsShape = true; // We now own the shape, the scene does not
    }
}

void CreateShapeCommand::redo()
{
    if (m_shape && m_scene) {
        // Only add if not already in scene (might be already there on first push)
        if (m_shape->scene() != m_scene) {
            m_scene->addItem(m_shape);
        }
        m_ownsShape = false; // The scene owns the shape now
    }
}


// ---------------------------------------------------------
// DeleteItemsCommand
// ---------------------------------------------------------
DeleteItemsCommand::DeleteItemsCommand(const QList<Shape*>& shapes, QGraphicsScene* scene, QUndoCommand* parent)
    : QUndoCommand(parent), m_scene(scene), m_ownsShapes(false)
{
    setText("Delete Items");
    
    // Deduplicate incoming shapes to ensure unique ownership
    for (Shape* shape : shapes) {
        if (shape && !m_shapes.contains(shape)) {
            m_shapes.append(shape);
        }
    }

    if (m_scene) {
        // Cascade scan: find all connectors in the scene that are attached to any of the shapes being deleted.
        for (QGraphicsItem* item : m_scene->items()) {
            if (auto* connector = dynamic_cast<Connector*>(item)) {
                bool startBound = (connector->getStartAnchor().mode() != ConnectorAnchor::Mode::Free && m_shapes.contains(connector->getStartAnchor().targetShape()));
                bool endBound = (connector->getEndAnchor().mode() != ConnectorAnchor::Mode::Free && m_shapes.contains(connector->getEndAnchor().targetShape()));
                if ((startBound || endBound) && !m_shapes.contains(connector)) {
                    // This connector depends on a shape being deleted, and is not already in m_shapes.
                    m_cascadeConnectors.append({connector, connector->getStartAnchor(), connector->getEndAnchor()});
                }
            }
        }
    }
}

DeleteItemsCommand::~DeleteItemsCommand()
{
    if (m_ownsShapes) {
        for (Shape* shape : m_shapes) {
            if (shape) delete shape;
        }
        for (const auto& cascade : m_cascadeConnectors) {
            if (cascade.connector) delete cascade.connector;
        }
    }
}

void DeleteItemsCommand::undo()
{
    if (m_scene) {
        // Restore shapes
        for (Shape* shape : m_shapes) {
            if (shape && shape->scene() != m_scene) {
                m_scene->addItem(shape);
            }
        }
        // Restore connectors and their original anchors
        for (const auto& cascade : m_cascadeConnectors) {
            if (cascade.connector) {
                if (cascade.connector->scene() != m_scene) {
                    m_scene->addItem(cascade.connector);
                }
                cascade.connector->setStartAnchor(cascade.startAnchor, ApplyMode::HistoryReplay);
                cascade.connector->setEndAnchor(cascade.endAnchor, ApplyMode::HistoryReplay);
            }
        }
    }
    m_ownsShapes = false; // Scene owns them again
}

void DeleteItemsCommand::redo()
{
    if (m_scene) {
        // Remove cascaded connectors first
        for (const auto& cascade : m_cascadeConnectors) {
            if (cascade.connector && cascade.connector->scene() == m_scene) {
                m_scene->removeItem(cascade.connector);
            }
        }
        // Remove shapes
        for (Shape* shape : m_shapes) {
            if (shape && shape->scene() == m_scene) {
                m_scene->removeItem(shape);
            }
        }
    }
    m_ownsShapes = true; // We own them while they are removed from the scene
}


// ---------------------------------------------------------
// MoveItemsCommand
// ---------------------------------------------------------
MoveItemsCommand::MoveItemsCommand(const QList<MoveData>& moves, QUndoCommand* parent)
    : QUndoCommand(parent), m_moves(moves)
{
    setText("Move Items");
}

void MoveItemsCommand::undo()
{
    for (const auto& move : m_moves) {
        if (move.shape) {
            move.shape->setPosition(move.oldPos, ApplyMode::HistoryReplay);
        }
    }
}

void MoveItemsCommand::redo()
{
    for (const auto& move : m_moves) {
        if (move.shape) {
            move.shape->setPosition(move.newPos, ApplyMode::HistoryReplay);
        }
    }
}


// ---------------------------------------------------------
// ResizeItemCommand
// ---------------------------------------------------------
ResizeItemCommand::ResizeItemCommand(Shape* shape, QSizeF oldSize, QSizeF newSize, QPointF oldPos, QPointF newPos, QUndoCommand* parent)
    : QUndoCommand(parent), m_shape(shape), m_oldSize(oldSize), m_newSize(newSize), m_oldPos(oldPos), m_newPos(newPos)
{
    setText("Resize Item");
}

void ResizeItemCommand::undo()
{
    if (m_shape) {
        m_shape->setSize(m_oldSize, ApplyMode::HistoryReplay);
        m_shape->setPosition(m_oldPos, ApplyMode::HistoryReplay);
    }
}

void ResizeItemCommand::redo()
{
    if (m_shape) {
        m_shape->setSize(m_newSize, ApplyMode::HistoryReplay);
        m_shape->setPosition(m_newPos, ApplyMode::HistoryReplay);
    }
}


// ---------------------------------------------------------
// RotateItemCommand
// ---------------------------------------------------------
RotateItemCommand::RotateItemCommand(Shape* shape, qreal oldAngle, qreal newAngle, QUndoCommand* parent)
    : QUndoCommand(parent), m_shape(shape), m_oldAngle(oldAngle), m_newAngle(newAngle)
{
    setText("Rotate Item");
}

void RotateItemCommand::undo()
{
    if (m_shape) {
        m_shape->setRotation(m_oldAngle, ApplyMode::HistoryReplay);
    }
}

void RotateItemCommand::redo()
{
    if (m_shape) {
        m_shape->setRotation(m_newAngle, ApplyMode::HistoryReplay);
    }
}


// ---------------------------------------------------------
// MoveEndpointCommand
// ---------------------------------------------------------
MoveEndpointCommand::MoveEndpointCommand(LineShape* line, QPointF oldStart, QPointF oldEnd, QPointF newStart, QPointF newEnd, QUndoCommand* parent)
    : QUndoCommand(parent), m_line(line), m_oldStart(oldStart), m_oldEnd(oldEnd), m_newStart(newStart), m_newEnd(newEnd)
{
    setText("Move Line Endpoint");
}

void MoveEndpointCommand::undo()
{
    if (m_line) {
        m_line->setEndpoints(m_oldStart, m_oldEnd, ApplyMode::HistoryReplay);
    }
}

void MoveEndpointCommand::redo()
{
    if (m_line) {
        m_line->setEndpoints(m_newStart, m_newEnd, ApplyMode::HistoryReplay);
    }
}


// ---------------------------------------------------------
// CreateConnectorCommand
// ---------------------------------------------------------
CreateConnectorCommand::CreateConnectorCommand(Connector* connector, QGraphicsScene* scene, QUndoCommand* parent)
    : QUndoCommand(parent), m_connector(connector), m_scene(scene), m_ownsConnector(false)
{
    setText("Create Connector");
}

CreateConnectorCommand::~CreateConnectorCommand()
{
    if (m_ownsConnector && m_connector) {
        delete m_connector;
    }
}

void CreateConnectorCommand::undo()
{
    if (m_connector && m_scene) {
        m_scene->removeItem(m_connector);
        m_ownsConnector = true;
    }
}

void CreateConnectorCommand::redo()
{
    if (m_connector && m_scene) {
        if (m_connector->scene() != m_scene) {
            m_scene->addItem(m_connector);
        }
        m_ownsConnector = false;
    }
}


// ---------------------------------------------------------
// ModifyConnectorCommand
// ---------------------------------------------------------
ModifyConnectorCommand::ModifyConnectorCommand(Connector* connector, Endpoint ep, ConnectorAnchor oldAnchor, ConnectorAnchor newAnchor, QUndoCommand* parent)
    : QUndoCommand(parent), m_connector(connector), m_endpoint(ep), m_oldAnchor(oldAnchor), m_newAnchor(newAnchor)
{
    setText("Modify Connector");
}

void ModifyConnectorCommand::undo()
{
    if (m_connector) {
        if (m_endpoint == Endpoint::Start) {
            m_connector->setStartAnchor(m_oldAnchor, ApplyMode::HistoryReplay);
        } else {
            m_connector->setEndAnchor(m_oldAnchor, ApplyMode::HistoryReplay);
        }
    }
}

void ModifyConnectorCommand::redo()
{
    if (m_connector) {
        if (m_endpoint == Endpoint::Start) {
            m_connector->setStartAnchor(m_newAnchor, ApplyMode::HistoryReplay);
        } else {
            m_connector->setEndAnchor(m_newAnchor, ApplyMode::HistoryReplay);
        }
    }
}
