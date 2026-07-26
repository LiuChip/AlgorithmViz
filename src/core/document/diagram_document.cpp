#include "diagram_document.h"
#include <QGraphicsScene>
#include <QMap>
#include "../../shapes/rect_shape.h"
#include "../../shapes/ellipse_shape.h"
#include "../../shapes/diamond_shape.h"
#include "../../shapes/line_shape.h"
#include "../../shapes/arrow_shape.h"
#include "../../shapes/dual_arrow_shape.h"
#include "../../shapes/text_label.h"
#include "../undo_manager.h"
#include <QSet>
#include <memory>

DiagramDocument DiagramDocument::fromScene(const QGraphicsScene* scene) {
    DiagramDocument doc;
    if (!scene) return doc;

    doc.canvasRect = scene->sceneRect();

    // 遍历所有图元
    for (QGraphicsItem* item : scene->items()) {
        if (item->parentItem()) continue;

        if (Connector* connector = dynamic_cast<Connector*>(item)) {
            ConnectorData cData;
            cData.common.id = connector->getID();
            cData.common.name = connector->getName();
            cData.common.layer = connector->getLayer();
            cData.common.locked = connector->isLocked();
            cData.common.visible = connector->isVisible();
            cData.common.border = connector->getBorderInfo();
            cData.common.fillStyle = connector->getFillInfo();
            cData.common.textStyle = connector->getTextInfo();

            cData.endStyle = connector->getEndStyle();

            ConnectorAnchor startAnchor = connector->getStartAnchor();
            cData.startMode = startAnchor.getMode();
            if (cData.startMode == ConnectorAnchor::Mode::Free) {
                cData.startFreePoint = startAnchor.getFreeScenePoint();
            } else if (cData.startMode == ConnectorAnchor::Mode::Boundary || cData.startMode == ConnectorAnchor::Mode::Interior) {
                Shape* target = startAnchor.getTargetShape();
                cData.startFreePoint = startAnchor.resolveScenePoint(); // fallback for serialization
                if (target) {
                    cData.startTargetId = target->getID();
                    if (cData.startMode == ConnectorAnchor::Mode::Boundary) cData.startBoundaryAngle = startAnchor.getBoundaryAngle();
                    else cData.startInteriorNormalized = startAnchor.getInteriorNormalized();
                } else {
                    // 目标丢失，降级为 Free
                    cData.startMode = ConnectorAnchor::Mode::Free;
                }
            }

            ConnectorAnchor endAnchor = connector->getEndAnchor();
            cData.endMode = endAnchor.getMode();
            if (cData.endMode == ConnectorAnchor::Mode::Free) {
                cData.endFreePoint = endAnchor.getFreeScenePoint();
            } else if (cData.endMode == ConnectorAnchor::Mode::Boundary || cData.endMode == ConnectorAnchor::Mode::Interior) {
                Shape* target = endAnchor.getTargetShape();
                cData.endFreePoint = endAnchor.resolveScenePoint(); // fallback for serialization
                if (target) {
                    cData.endTargetId = target->getID();
                    if (cData.endMode == ConnectorAnchor::Mode::Boundary) cData.endBoundaryAngle = endAnchor.getBoundaryAngle();
                    else cData.endInteriorNormalized = endAnchor.getInteriorNormalized();
                } else {
                    cData.endMode = ConnectorAnchor::Mode::Free;
                }
            }

            doc.connectors.append(cData);
        }
        else if (Shape* shape = dynamic_cast<Shape*>(item)) {
            ShapeData sData;
            sData.common.id = shape->getID();
            sData.common.name = shape->getName();
            sData.common.layer = shape->getLayer();
            sData.common.locked = shape->isLocked();
            sData.common.visible = shape->isVisible();
            sData.common.border = shape->getBorderInfo();
            sData.common.fillStyle = shape->getFillInfo();
            sData.common.textStyle = shape->getTextInfo();

            sData.position = shape->getPosition();
            sData.size = shape->getSize();
            sData.rotation = shape->getRotation();

            // 提取特殊属性
            if (dynamic_cast<RectShape*>(shape)) sData.type = "RectShape";
            else if (dynamic_cast<EllipseShape*>(shape)) sData.type = "EllipseShape";
            else if (dynamic_cast<DiamondShape*>(shape)) sData.type = "DiamondShape";
            else if (TextLabel* text = dynamic_cast<TextLabel*>(shape)) {
                sData.type = "TextLabel";
                sData.textLayoutMode = text->getTextLayoutMode();
            }
            else if (LineShape* line = dynamic_cast<LineShape*>(shape)) {
                if (dynamic_cast<DualArrowShape*>(shape)) sData.type = "DualArrowShape";
                else if (dynamic_cast<ArrowShape*>(shape)) sData.type = "ArrowShape";
                else sData.type = "LineShape";

                sData.startPoint = line->getStartPoint();
                sData.endPoint = line->getEndPoint();
            }

            if (!sData.type.isEmpty()) doc.shapes.append(sData);
        }
    }
    return doc;
}

LoadResult DiagramDocument::applyToScene(QGraphicsScene* scene, UndoManager* undoManager) const {
    LoadResult result;
    result.success = true;

    if (!scene) {
        result.success = false;
        result.diagnostics.append({Diagnostic::Error, "Scene is null."});
        return result;
    }

    // 阶段0：前置安全校验 (负数 ID、重复 ID)
    QSet<int> seenIds;
    for (const auto& sData : shapes) {
        if (sData.common.id < 0) {
            result.success = false;
            result.diagnostics.append({Diagnostic::Error, QString("Shape has negative ID: %1").arg(sData.common.id)});
            return result;
        }
        if (seenIds.contains(sData.common.id)) {
            result.success = false;
            result.diagnostics.append({Diagnostic::Error, QString("Duplicate ID detected: %1").arg(sData.common.id)});
            return result;
        }
        seenIds.insert(sData.common.id);
    }
    for (const auto& cData : connectors) {
        if (cData.common.id < 0) {
            result.success = false;
            result.diagnostics.append({Diagnostic::Error, QString("Connector has negative ID: %1").arg(cData.common.id)});
            return result;
        }
        if (seenIds.contains(cData.common.id)) {
            result.success = false;
            result.diagnostics.append({Diagnostic::Error, QString("Duplicate ID detected: %1").arg(cData.common.id)});
            return result;
        }
        seenIds.insert(cData.common.id);
    }

    std::vector<std::unique_ptr<Shape>> newShapes;
    std::vector<std::unique_ptr<Connector>> newConnectors;
    QMap<int, ConnectableShape*> idMap;

    // 阶段1：在内存中创建所有的常规图形，但不添加到场景
    for (const ShapeData& sData : shapes) {
        Shape* shape = nullptr;

        if (sData.type == "RectShape") shape = new RectShape(0, 0, 10, 10);
        else if (sData.type == "EllipseShape") shape = new EllipseShape(0, 0, 10, 10);
        else if (sData.type == "DiamondShape") shape = new DiamondShape(0, 0, 10, 10);
        else if (sData.type == "TextLabel") {
            TextLabel* textLabel = new TextLabel(0, 0, sData.common.textStyle.text);
            shape = textLabel;
        }
        else if (sData.type == "LineShape" || sData.type == "ArrowShape" || sData.type == "DualArrowShape") {
            if (sData.type == "ArrowShape") shape = new ArrowShape(sData.startPoint, sData.endPoint);
            else if (sData.type == "DualArrowShape") shape = new DualArrowShape(sData.startPoint, sData.endPoint);
            else shape = new LineShape(sData.startPoint, sData.endPoint);
        } else {
            result.success = false;
            result.diagnostics.append({Diagnostic::Error, QString("Unknown shape type: %1. Loading aborted to prevent data loss.").arg(sData.type)});
            return result;
        }

        shape->setID(sData.common.id);

        // 注意：Setter 在值相同（例如默认 10x10）或某些线形不支持尺寸/旋转时返回 false
        // 这里的 false 不表示发生灾难性错误，直接顺道向图元应用属性即可
        shape->setPosition(sData.position, ApplyMode::SerializationLoad);
        shape->setSize(sData.size, ApplyMode::SerializationLoad);
        shape->setRotation(sData.rotation, ApplyMode::SerializationLoad);
        shape->setBorderInfo(sData.common.border);

        shape->setName(sData.common.name);
        shape->setLayer(sData.common.layer);
        shape->setLocked(sData.common.locked);
        shape->setVisible(sData.common.visible);
        shape->setFillInfo(sData.common.fillStyle);
        shape->setTextInfo(sData.common.textStyle);

        if (sData.type == "TextLabel") {
            static_cast<TextLabel*>(shape)->setTextLayoutMode(sData.textLayoutMode);
        }

        newShapes.push_back(std::unique_ptr<Shape>(shape));
        if (auto connectable = dynamic_cast<ConnectableShape*>(shape)) {
            idMap[sData.common.id] = connectable;
        }
    }

    // 阶段2：创建所有连接线
    for (const ConnectorData& cData : connectors) {
        Connector* connector = new Connector(QPointF(0,0), QPointF(10,10));
        connector->setID(cData.common.id);
        connector->setName(cData.common.name);
        connector->setLayer(cData.common.layer);
        connector->setLocked(cData.common.locked);
        connector->setVisible(cData.common.visible);

        if (!connector->setBorderInfo(cData.common.border)) {
            result.success = false;
            result.diagnostics.append({Diagnostic::Error, QString("Failed to set border for connector %1").arg(cData.common.id)});
            delete connector;
            return result;
        }

        connector->setFillInfo(cData.common.fillStyle);
        connector->setTextInfo(cData.common.textStyle);

        connector->setEndStyle(cData.endStyle);

        // 恢复 Start Anchor
        if (cData.startMode == ConnectorAnchor::Mode::Free) {
            connector->setStartAnchor(ConnectorAnchor::createFree(cData.startFreePoint), ApplyMode::SerializationLoad);
        } else if (cData.startMode == ConnectorAnchor::Mode::Boundary && idMap.contains(cData.startTargetId)) {
            if (!connector->setStartAnchor(ConnectorAnchor::createBoundary(idMap[cData.startTargetId], cData.startBoundaryAngle), ApplyMode::SerializationLoad)) {
                result.diagnostics.append({Diagnostic::Warning, QString("Failed to bind start boundary anchor for connector %1. Downgrading to Free.").arg(cData.common.id)});
                connector->setStartAnchor(ConnectorAnchor::createFree(cData.startFreePoint), ApplyMode::SerializationLoad);
            }
        } else if (cData.startMode == ConnectorAnchor::Mode::Interior && idMap.contains(cData.startTargetId)) {
            if (!connector->setStartAnchor(ConnectorAnchor::createInterior(idMap[cData.startTargetId], cData.startInteriorNormalized), ApplyMode::SerializationLoad)) {
                result.diagnostics.append({Diagnostic::Warning, QString("Failed to bind start interior anchor for connector %1. Downgrading to Free.").arg(cData.common.id)});
                connector->setStartAnchor(ConnectorAnchor::createFree(cData.startFreePoint), ApplyMode::SerializationLoad);
            }
        } else {
            result.diagnostics.append({Diagnostic::Warning, QString("Start target %1 not found for connector %2. Downgrading to Free.").arg(cData.startTargetId).arg(cData.common.id)});
            connector->setStartAnchor(ConnectorAnchor::createFree(cData.startFreePoint), ApplyMode::SerializationLoad);
        }

        // 恢复 End Anchor
        if (cData.endMode == ConnectorAnchor::Mode::Free) {
            connector->setEndAnchor(ConnectorAnchor::createFree(cData.endFreePoint), ApplyMode::SerializationLoad);
        } else if (cData.endMode == ConnectorAnchor::Mode::Boundary && idMap.contains(cData.endTargetId)) {
            if (!connector->setEndAnchor(ConnectorAnchor::createBoundary(idMap[cData.endTargetId], cData.endBoundaryAngle), ApplyMode::SerializationLoad)) {
                result.diagnostics.append({Diagnostic::Warning, QString("Failed to bind end boundary anchor for connector %1. Downgrading to Free.").arg(cData.common.id)});
                connector->setEndAnchor(ConnectorAnchor::createFree(cData.endFreePoint), ApplyMode::SerializationLoad);
            }
        } else if (cData.endMode == ConnectorAnchor::Mode::Interior && idMap.contains(cData.endTargetId)) {
            if (!connector->setEndAnchor(ConnectorAnchor::createInterior(idMap[cData.endTargetId], cData.endInteriorNormalized), ApplyMode::SerializationLoad)) {
                result.diagnostics.append({Diagnostic::Warning, QString("Failed to bind end interior anchor for connector %1. Downgrading to Free.").arg(cData.common.id)});
                connector->setEndAnchor(ConnectorAnchor::createFree(cData.endFreePoint), ApplyMode::SerializationLoad);
            }
        } else {
            result.diagnostics.append({Diagnostic::Warning, QString("End target %1 not found for connector %2. Downgrading to Free.").arg(cData.endTargetId).arg(cData.common.id)});
            connector->setEndAnchor(ConnectorAnchor::createFree(cData.endFreePoint), ApplyMode::SerializationLoad);
        }

        newConnectors.push_back(std::unique_ptr<Connector>(connector));
    }

    // 阶段3：确保一切都成功构建后，再清空 UndoStack 避免悬空指针
    if (undoManager) {
        undoManager->clear();
    }

    // 阶段4：清空场景
    QList<QGraphicsItem*> items = scene->items();
    for (QGraphicsItem* item : items) {
        if (!item->parentItem() && dynamic_cast<Shape*>(item)) {
            scene->removeItem(item);
            delete item;
        }
    }

    // 阶段5：将构建好的图元加入场景
    scene->setSceneRect(canvasRect);
    for (auto& shape : newShapes) {
        scene->addItem(shape.release());
    }
    for (auto& connector : newConnectors) {
        scene->addItem(connector.release());
    }

    return result;
}
