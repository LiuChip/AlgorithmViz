#ifndef SHAPE_EXPLORER_WIDGET_H
#define SHAPE_EXPLORER_WIDGET_H

#include <QHash>
#include <QPointer>
#include <QTreeWidget>
#include <QWidget>

class Canvas;
class QGraphicsScene;
class QDropEvent;
class QTimer;
class Shape;

class ShapeExplorerTree final : public QTreeWidget
{
    Q_OBJECT

public:
    explicit ShapeExplorerTree(QWidget *parent = nullptr);

signals:
    void itemDropped(int sourceShapeId, int targetShapeId, bool mergeWithTarget);

protected:
    void startDrag(Qt::DropActions supportedActions) override;
    void dropEvent(QDropEvent *event) override;

private:
    int m_draggedShapeId = -1;
};

// 图形列表与图层管理器。列表顶部代表较高 Z 值，并与画布选择、可见性及锁定状态同步。
class ShapeExplorerWidget : public QWidget
{
    Q_OBJECT

public:
    enum Column {
        VisibleColumn = 0,
        NameColumn = 1,
        LockedColumn = 2
    };
    enum DataRole {
        ShapeIdRole = Qt::UserRole + 1,
        LayerRole
    };

    explicit ShapeExplorerWidget(QWidget *parent = nullptr);
    ~ShapeExplorerWidget() override = default;

    void setCanvas(Canvas *canvas);
    void setScene(QGraphicsScene *scene);
    Canvas *canvas() const { return m_canvas; }
    QGraphicsScene *scene() const { return m_scene; }
    QTreeWidget *treeWidget() const { return m_tree; }

public slots:
    void refresh();

private:
    void scheduleRefresh();
    Shape *shapeForItem(const QTreeWidgetItem *item) const;
    Shape *shapeById(int shapeId) const;
    void synchronizeSelectionFromScene();
    void synchronizeSelectionToScene();
    void applyDrop(int sourceShapeId, int targetShapeId, bool mergeWithTarget);
    QList<QPair<Shape *, int>> normalizedLayerChanges(int sourceShapeId) const;

    ShapeExplorerTree *m_tree = nullptr;
    QTimer *m_refreshTimer = nullptr;
    QPointer<Canvas> m_canvas;
    QPointer<QGraphicsScene> m_scene;
    QHash<int, QPointer<Shape>> m_shapes;
    bool m_synchronizing = false;
};

#endif // SHAPE_EXPLORER_WIDGET_H
