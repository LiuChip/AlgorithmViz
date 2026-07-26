#include <QtTest>

#include "../src/core/canvas.h"
#include "../src/shapes/ellipse_shape.h"
#include "../src/shapes/rect_shape.h"
#include "../src/ui/shape_explorer_widget.h"

#include <QGraphicsScene>
#include <QTreeWidget>

class ShapeExplorerTest : public QObject
{
    Q_OBJECT

private slots:
    void testListsShapesByDescendingLayer()
    {
        QGraphicsScene scene;
        auto *lower = new RectShape(QPointF(0, 0), QSizeF(40, 40));
        auto *upper = new EllipseShape(QPointF(60, 0), QSizeF(40, 40));
        lower->setName(QStringLiteral("Lower"));
        upper->setName(QStringLiteral("Upper"));
        lower->setLayer(2);
        upper->setLayer(8);
        scene.addItem(lower);
        scene.addItem(upper);

        ShapeExplorerWidget explorer;
        explorer.setScene(&scene);

        QTreeWidget *tree = explorer.treeWidget();
        QCOMPARE(tree->topLevelItemCount(), 2);
        QCOMPARE(tree->topLevelItem(0)->data(ShapeExplorerWidget::NameColumn,
                                             ShapeExplorerWidget::ShapeIdRole).toInt(),
                 upper->getID());
        QCOMPARE(tree->topLevelItem(1)->data(ShapeExplorerWidget::NameColumn,
                                             ShapeExplorerWidget::ShapeIdRole).toInt(),
                 lower->getID());
    }

    void testVisibilityAndLockTogglesUpdateShape()
    {
        QGraphicsScene scene;
        auto *shape = new RectShape(QPointF(0, 0), QSizeF(40, 40));
        scene.addItem(shape);

        ShapeExplorerWidget explorer;
        explorer.setScene(&scene);
        QTreeWidgetItem *item = explorer.treeWidget()->topLevelItem(0);

        item->setCheckState(ShapeExplorerWidget::VisibleColumn, Qt::Unchecked);
        QVERIFY(!shape->isVisible());

        item->setCheckState(ShapeExplorerWidget::LockedColumn, Qt::Checked);
        QVERIFY(shape->isLocked());
    }

    void testCanvasBackedChangesParticipateInUndo()
    {
        Canvas canvas;
        auto *shape = new RectShape(QPointF(0, 0), QSizeF(40, 40));
        canvas.scene()->addItem(shape);

        ShapeExplorerWidget explorer;
        explorer.setCanvas(&canvas);
        QTreeWidgetItem *item = explorer.treeWidget()->topLevelItem(0);
        QVERIFY(item);

        item->setCheckState(ShapeExplorerWidget::VisibleColumn, Qt::Unchecked);
        QVERIFY(!shape->isVisible());
        QCOMPARE(canvas.undoManager()->stack()->count(), 1);

        canvas.undo();
        QVERIFY(shape->isVisible());
    }

    void testExplicitSceneDetachesCanvasUndoContext()
    {
        Canvas canvas;
        QGraphicsScene externalScene;
        auto *shape = new RectShape(QPointF(0, 0), QSizeF(40, 40));
        externalScene.addItem(shape);

        ShapeExplorerWidget explorer;
        explorer.setCanvas(&canvas);
        explorer.setScene(&externalScene);

        QTreeWidgetItem *item = explorer.treeWidget()->topLevelItem(0);
        QVERIFY(item);
        item->setCheckState(ShapeExplorerWidget::VisibleColumn, Qt::Unchecked);

        QVERIFY(!shape->isVisible());
        QCOMPARE(canvas.undoManager()->stack()->count(), 0);
        QVERIFY(!explorer.canvas());
    }

    void testSelectionIsSynchronizedBothWays()
    {
        QGraphicsScene scene;
        auto *shape = new RectShape(QPointF(0, 0), QSizeF(40, 40));
        scene.addItem(shape);

        ShapeExplorerWidget explorer;
        explorer.setScene(&scene);
        QTreeWidgetItem *item = explorer.treeWidget()->topLevelItem(0);

        item->setSelected(true);
        QCoreApplication::processEvents();
        QVERIFY(shape->isSelected());

        item->setSelected(false);
        shape->setSelected(true);
        QCoreApplication::processEvents();
        QVERIFY(item->isSelected());
    }

    void testNameCanBeEditedAndRowsHaveShapeIcons()
    {
        QGraphicsScene scene;
        auto *shape = new RectShape(QPointF(0, 0), QSizeF(40, 40));
        scene.addItem(shape);

        ShapeExplorerWidget explorer;
        explorer.setScene(&scene);
        QTreeWidgetItem *item = explorer.treeWidget()->topLevelItem(0);
        QVERIFY(item);
        QVERIFY(item->flags().testFlag(Qt::ItemIsEditable));
        QVERIFY(!item->icon(ShapeExplorerWidget::NameColumn).isNull());

        item->setText(ShapeExplorerWidget::NameColumn, QStringLiteral("入口节点"));
        QCoreApplication::processEvents();
        QCOMPARE(shape->getName(), QStringLiteral("入口节点"));
    }

    void testDropOnItemMergesLayers()
    {
        QGraphicsScene scene;
        auto *source = new RectShape(QPointF(0, 0), QSizeF(40, 40));
        auto *target = new EllipseShape(QPointF(60, 0), QSizeF(40, 40));
        source->setLayer(9);
        target->setLayer(3);
        scene.addItem(source);
        scene.addItem(target);

        ShapeExplorerWidget explorer;
        explorer.setScene(&scene);
        auto *tree = static_cast<ShapeExplorerTree *>(explorer.treeWidget());
        emit tree->itemDropped(source->getID(), target->getID(), true);

        QCOMPARE(source->getLayer(), target->getLayer());
    }

    void testGapDropAtViewportBottomMovesShapeToLowestLayer()
    {
        QGraphicsScene scene;
        auto *source = new RectShape(QPointF(0, 0), QSizeF(40, 40));
        auto *middle = new RectShape(QPointF(50, 0), QSizeF(40, 40));
        auto *bottom = new EllipseShape(QPointF(100, 0), QSizeF(40, 40));
        source->setLayer(9);
        middle->setLayer(5);
        bottom->setLayer(1);
        scene.addItem(source);
        scene.addItem(middle);
        scene.addItem(bottom);

        ShapeExplorerWidget explorer;
        explorer.setScene(&scene);
        auto *tree = static_cast<ShapeExplorerTree *>(explorer.treeWidget());

        QTreeWidgetItem *sourceItem = tree->takeTopLevelItem(0);
        tree->addTopLevelItem(sourceItem);
        emit tree->itemDropped(source->getID(), -1, false);

        QVERIFY(source->getLayer() < middle->getLayer());
        QVERIFY(source->getLayer() < bottom->getLayer());
    }

    void testGapDropPreservesOtherMergedLayerGroups()
    {
        QGraphicsScene scene;
        auto *top = new RectShape(QPointF(0, 0), QSizeF(40, 40));
        auto *groupA = new RectShape(QPointF(50, 0), QSizeF(40, 40));
        auto *groupB = new EllipseShape(QPointF(100, 0), QSizeF(40, 40));
        auto *source = new EllipseShape(QPointF(150, 0), QSizeF(40, 40));
        top->setLayer(10);
        groupA->setLayer(6);
        groupB->setLayer(6);
        source->setLayer(1);
        scene.addItem(top);
        scene.addItem(groupA);
        scene.addItem(groupB);
        scene.addItem(source);

        ShapeExplorerWidget explorer;
        explorer.setScene(&scene);
        auto *tree = static_cast<ShapeExplorerTree *>(explorer.treeWidget());

        int sourceRow = -1;
        for (int row = 0; row < tree->topLevelItemCount(); ++row) {
            if (tree->topLevelItem(row)->data(ShapeExplorerWidget::NameColumn,
                                              ShapeExplorerWidget::ShapeIdRole).toInt()
                == source->getID()) {
                sourceRow = row;
                break;
            }
        }
        QVERIFY(sourceRow >= 0);
        QTreeWidgetItem *sourceItem = tree->takeTopLevelItem(sourceRow);
        tree->insertTopLevelItem(1, sourceItem);
        emit tree->itemDropped(source->getID(), groupA->getID(), false);

        QCOMPARE(groupA->getLayer(), groupB->getLayer());
        QVERIFY(top->getLayer() > source->getLayer());
        QVERIFY(source->getLayer() > groupA->getLayer());
    }
};

QTEST_MAIN(ShapeExplorerTest)
#include "shape_explorer_test.moc"
