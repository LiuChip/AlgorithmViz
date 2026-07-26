#include <QtTest>
#include <QGraphicsScene>
#include <QUndoStack>
#include "core/commands/undo_commands.h"
#include "shapes/rect_shape.h"
#include "shapes/line_shape.h"
#include "shapes/connector/connector.h"
#include "core/canvas.h"

#include <limits>

class UndoCommandsTest : public QObject {
    Q_OBJECT

private slots:
    void testCreateShapeCommand() {
        QGraphicsScene scene;
        QUndoStack stack;
        
        RectShape* rect = new RectShape(0, 0, 100, 100);
        
        // Push should redo automatically
        stack.push(new CreateShapeCommand(rect, &scene));
        QCOMPARE(scene.items().size(), 1);
        QCOMPARE(rect->scene(), &scene);
        
        // Undo
        stack.undo();
        QCOMPARE(scene.items().size(), 0);
        QVERIFY(rect->scene() == nullptr);
        
        // Redo
        stack.redo();
        QCOMPARE(scene.items().size(), 1);
        QCOMPARE(rect->scene(), &scene);
    }
    
    void testDeleteItemsCommandWithCascade() {
        QGraphicsScene scene;
        QUndoStack stack;
        
        RectShape* rect1 = new RectShape(0, 0, 100, 100);
        RectShape* rect2 = new RectShape(200, 200, 100, 100);
        scene.addItem(rect1);
        scene.addItem(rect2);
        
        Connector* connector = new Connector(QPointF(50, 50), QPointF(250, 250));
        scene.addItem(connector);
        
        connector->setStartAnchor(ConnectorAnchor::createBoundary(rect1, 0));
        connector->setEndAnchor(ConnectorAnchor::createBoundary(rect2, 0));
        
        QCOMPARE(scene.items().size(), 3);
        
        // Delete rect1, connector should cascade
        QList<Shape*> toDelete = { rect1 };
        stack.push(new DeleteItemsCommand(toDelete, &scene));
        
        // After push (redo), rect1 and connector should be removed, rect2 remains
        QCOMPARE(scene.items().size(), 1);
        QVERIFY(scene.items().contains(rect2));
        
        // Undo
        stack.undo();
        QCOMPARE(scene.items().size(), 3);
        QVERIFY(connector->getStartAnchor().targetShape() == rect1);
        
        // Redo
        stack.redo();
        QCOMPARE(scene.items().size(), 1);
    }
    
    void testDeleteItemsCommandDuplicateFree() {
        QGraphicsScene scene;
        QUndoStack stack;
        
        RectShape* rect = new RectShape(0, 0, 100, 100);
        scene.addItem(rect);
        
        Connector* connector = new Connector(QPointF(50, 50), QPointF(250, 250));
        scene.addItem(connector);
        connector->setStartAnchor(ConnectorAnchor::createBoundary(rect, 0));
        
        // Pass BOTH rect and connector. This used to cause double free.
        QList<Shape*> toDelete = { rect, connector };
        DeleteItemsCommand* cmd = new DeleteItemsCommand(toDelete, &scene);
        stack.push(cmd);
        
        QCOMPARE(scene.items().size(), 0);
        stack.undo();
        QCOMPARE(scene.items().size(), 2);
        
        // Let stack destruct, deleting the command and shapes if it's the current state (redo state).
        // If double free occurs, this will crash.
        stack.redo();
    }
    
    void testMoveItemsCommand() {
        QGraphicsScene scene;
        QUndoStack stack;
        
        RectShape* rect = new RectShape(0, 0, 100, 100);
        scene.addItem(rect);
        rect->setPosition(QPointF(10, 10));
        
        QList<MoveItemsCommand::MoveData> moves = {
            {rect, QPointF(10, 10), QPointF(50, 50)}
        };
        
        stack.push(new MoveItemsCommand(moves));
        QCOMPARE(rect->pos(), QPointF(50, 50));
        
        stack.undo();
        QCOMPARE(rect->pos(), QPointF(10, 10));
        
        stack.redo();
        QCOMPARE(rect->pos(), QPointF(50, 50));
    }
    
    void testResizeItemCommand() {
        QGraphicsScene scene;
        QUndoStack stack;
        
        RectShape* rect = new RectShape(0, 0, 100, 100);
        scene.addItem(rect);
        rect->setSize(QSizeF(100, 100));
        rect->setPosition(QPointF(50, 50));
        
        stack.push(new ResizeItemCommand(rect, QSizeF(100, 100), QSizeF(200, 200), QPointF(50, 50), QPointF(100, 100)));
        QCOMPARE(rect->getSize(), QSizeF(200, 200));
        QCOMPARE(rect->pos(), QPointF(100, 100));
        
        stack.undo();
        QCOMPARE(rect->getSize(), QSizeF(100, 100));
        QCOMPARE(rect->pos(), QPointF(50, 50));
        
        stack.redo();
        QCOMPARE(rect->getSize(), QSizeF(200, 200));
        QCOMPARE(rect->pos(), QPointF(100, 100));
    }
    
    void testRotateItemCommand() {
        QGraphicsScene scene;
        QUndoStack stack;
        
        RectShape* rect = new RectShape(0, 0, 100, 100);
        scene.addItem(rect);
        rect->setRotation(0);
        
        stack.push(new RotateItemCommand(rect, 0, 45));
        QCOMPARE(rect->rotation(), 45.0);
        
        stack.undo();
        QCOMPARE(rect->rotation(), 0.0);
        
        stack.redo();
        QCOMPARE(rect->rotation(), 45.0);
    }
    
    void testMoveEndpointCommand() {
        QGraphicsScene scene;
        QUndoStack stack;
        
        LineShape* line = new LineShape(QPointF(0, 0), QPointF(100, 100));
        
        stack.push(new MoveEndpointCommand(line, QPointF(0, 0), QPointF(100, 100), QPointF(10, 10), QPointF(100, 100)));
        QCOMPARE(line->getStartPoint(), QPointF(10, 10));
        
        stack.undo();
        QCOMPARE(line->getStartPoint(), QPointF(0, 0));
        
        stack.redo();
        QCOMPARE(line->getStartPoint(), QPointF(10, 10));
    }
    
    void testCreateConnectorCommand() {
        QGraphicsScene scene;
        QUndoStack stack;
        
        Connector* connector = new Connector(QPointF(0, 0), QPointF(100, 100));
        // push handles adding it
        stack.push(new CreateConnectorCommand(connector, &scene));
        QCOMPARE(scene.items().size(), 1);
        
        stack.undo();
        QCOMPARE(scene.items().size(), 0);
        
        stack.redo();
        QCOMPARE(scene.items().size(), 1);
    }
    
    void testModifyConnectorCommand() {
        QGraphicsScene scene;
        QUndoStack stack;
        
        Connector* connector = new Connector(QPointF(0, 0), QPointF(100, 100));
        scene.addItem(connector);
        
        ConnectorAnchor oldAnchor = ConnectorAnchor::createFree(QPointF(0, 0));
        ConnectorAnchor newAnchor = ConnectorAnchor::createFree(QPointF(100, 100));
        connector->setStartAnchor(oldAnchor);
        
        stack.push(new ModifyConnectorCommand(connector, ModifyConnectorCommand::Endpoint::Start, oldAnchor, newAnchor));
        QCOMPARE(connector->getStartAnchor().resolveScenePoint(), QPointF(100, 100));
        
        stack.undo();
        QCOMPARE(connector->getStartAnchor().resolveScenePoint(), QPointF(0, 0));
        
        stack.redo();
        QCOMPARE(connector->getStartAnchor().resolveScenePoint(), QPointF(100, 100));
    }

    void testClearSceneIntegrity() {
        Canvas canvas;
        canvas.undoManager()->clear(); // Ensure it starts clean
        
        auto countShapes = [](QGraphicsScene* s) {
            int count = 0;
            for (auto* item : s->items()) {
                if (!item->parentItem() && dynamic_cast<Shape*>(item)) {
                    count++;
                }
            }
            return count;
        };

        RectShape* rect = new RectShape(0, 0, 100, 100);
        canvas.undoManager()->push(new CreateShapeCommand(rect, canvas.scene()));
        QCOMPARE(countShapes(canvas.scene()), 1);
        
        // This should clear the undo stack before clearing the scene
        canvas.clearScene();
        QCOMPARE(countShapes(canvas.scene()), 0);
        QVERIFY(!canvas.undoManager()->stack()->canUndo());
        QVERIFY(!canvas.undoManager()->stack()->canRedo());
    }

    void testCanvasControllerLayerCommandsAreUndoable() {
        Canvas canvas;
        auto *back = new RectShape(0, 0, 40, 40);
        auto *middle = new RectShape(50, 0, 40, 40);
        auto *front = new RectShape(100, 0, 40, 40);
        back->setLayer(1);
        middle->setLayer(2);
        front->setLayer(3);
        canvas.scene()->addItem(back);
        canvas.scene()->addItem(middle);
        canvas.scene()->addItem(front);

        canvas.canvasController()->selectItem(back);
        canvas.canvasController()->selectItem(middle, false);
        canvas.canvasController()->bringSelectedToFront();
        QCOMPARE(back->getLayer(), 4);
        QCOMPARE(middle->getLayer(), 5);
        QCOMPARE(front->getLayer(), 3);

        canvas.undoManager()->undo();
        QCOMPARE(back->getLayer(), 1);
        QCOMPARE(middle->getLayer(), 2);

        canvas.undoManager()->redo();
        QCOMPARE(back->getLayer(), 4);
        QCOMPARE(middle->getLayer(), 5);

        canvas.canvasController()->sendSelectedToBack();
        QCOMPARE(back->getLayer(), 1);
        QCOMPARE(middle->getLayer(), 2);
        QCOMPARE(front->getLayer(), 3);

        canvas.undoManager()->undo();
        QCOMPARE(back->getLayer(), 4);
        QCOMPARE(middle->getLayer(), 5);
    }

    void testCanvasControllerLayerCommandsRejectIntegerOverflow() {
        {
            Canvas canvas;
            auto *shape = new RectShape(0, 0, 40, 40);
            shape->setLayer(std::numeric_limits<int>::max());
            canvas.scene()->addItem(shape);
            canvas.canvasController()->selectItem(shape);

            const int undoCount = canvas.undoManager()->stack()->count();
            canvas.canvasController()->bringSelectedToFront();
            QCOMPARE(shape->getLayer(), std::numeric_limits<int>::max());
            QCOMPARE(canvas.undoManager()->stack()->count(), undoCount);
        }

        {
            Canvas canvas;
            auto *shape = new RectShape(0, 0, 40, 40);
            shape->setLayer(std::numeric_limits<int>::min());
            canvas.scene()->addItem(shape);
            canvas.canvasController()->selectItem(shape);

            const int undoCount = canvas.undoManager()->stack()->count();
            canvas.canvasController()->sendSelectedToBack();
            QCOMPARE(shape->getLayer(), std::numeric_limits<int>::min());
            QCOMPARE(canvas.undoManager()->stack()->count(), undoCount);
        }
    }

    void testCanvasControllerClearAllIsUndoable() {
        Canvas canvas;
        auto countShapes = [](QGraphicsScene *scene) {
            int count = 0;
            for (QGraphicsItem *item : scene->items()) {
                if (!item->parentItem() && dynamic_cast<Shape *>(item))
                    ++count;
            }
            return count;
        };

        auto *first = new RectShape(0, 0, 40, 40);
        auto *second = new RectShape(100, 0, 40, 40);
        auto *connector = new Connector(QPointF(40, 20), QPointF(100, 20));
        canvas.scene()->addItem(first);
        canvas.scene()->addItem(second);
        canvas.scene()->addItem(connector);
        connector->setStartAnchor(ConnectorAnchor::createBoundary(first, 0.0));
        connector->setEndAnchor(ConnectorAnchor::createBoundary(second, 3.14159265358979323846));
        QCOMPARE(countShapes(canvas.scene()), 3);

        canvas.canvasController()->clearAllItems();
        QCOMPARE(countShapes(canvas.scene()), 0);
        QVERIFY(canvas.undoManager()->stack()->canUndo());

        canvas.undoManager()->undo();
        QCOMPARE(countShapes(canvas.scene()), 3);
        QCOMPARE(connector->getStartAnchor().targetShape(), first);
        QCOMPARE(connector->getEndAnchor().targetShape(), second);

        canvas.undoManager()->redo();
        QCOMPARE(countShapes(canvas.scene()), 0);
    }

    void testLockedShapeUndoReplay() {
        QGraphicsScene scene;
        QUndoStack stack;
        
        RectShape* rect = new RectShape(0, 0, 100, 100);
        scene.addItem(rect);
        rect->setPosition(QPointF(10, 10));
        
        // Lock the shape
        rect->setLocked(true);
        QVERIFY(rect->isLocked());
        
        // Push an undo command that modifies the locked shape
        QList<MoveItemsCommand::MoveData> moves = {
            {rect, QPointF(10, 10), QPointF(50, 50)}
        };
        stack.push(new MoveItemsCommand(moves));
        
        // Because of ApplyMode::HistoryReplay, the command SHOULD bypass the lock
        QCOMPARE(rect->pos(), QPointF(50, 50));
        
        stack.undo();
        QCOMPARE(rect->pos(), QPointF(10, 10));
        
        stack.redo();
        QCOMPARE(rect->pos(), QPointF(50, 50));
        
        // Shape should still be locked at the end
        QVERIFY(rect->isLocked());
    }

    void testConnectorAnchorEquality() {
        RectShape* rect = new RectShape(0, 0, 100, 100);
        
        ConnectorAnchor a1 = ConnectorAnchor::createFree(QPointF(10, 10));
        ConnectorAnchor a2 = ConnectorAnchor::createFree(QPointF(10, 10));
        ConnectorAnchor a3 = ConnectorAnchor::createFree(QPointF(20, 20));
        QVERIFY(a1 == a2);
        QVERIFY(a1 != a3);
        
        ConnectorAnchor b1 = ConnectorAnchor::createBoundary(rect, 1.5);
        ConnectorAnchor b2 = ConnectorAnchor::createBoundary(rect, 1.5);
        ConnectorAnchor b3 = ConnectorAnchor::createBoundary(rect, 2.0);
        QVERIFY(b1 == b2);
        QVERIFY(b1 != b3);
        QVERIFY(a1 != b1);

        delete rect;
    }
};

QTEST_MAIN(UndoCommandsTest)
#include "undo_commands_test.moc"
