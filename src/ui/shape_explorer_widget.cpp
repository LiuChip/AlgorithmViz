#include "shape_explorer_widget.h"

#include "../core/canvas.h"
#include "../core/commands/undo_commands.h"
#include "../shapes/arrow_shape.h"
#include "../shapes/diamond_shape.h"
#include "../shapes/dual_arrow_shape.h"
#include "../shapes/ellipse_shape.h"
#include "../shapes/line_shape.h"
#include "../shapes/rect_shape.h"
#include "../shapes/shape.h"
#include "../shapes/text_label.h"
#include "../shapes/connector/connector.h"

#include <QAbstractItemView>
#include <QDropEvent>
#include <QGraphicsScene>
#include <QHeaderView>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QTimer>
#include <QVBoxLayout>
#include <QUndoCommand>

#include <algorithm>

namespace {

QIcon iconForShape(const Shape *shape)
{
    QPixmap pixmap(20, 20);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    const QPen outline(QColor(QStringLiteral("#52616f")), 1.5, Qt::SolidLine,
                       Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(outline);
    painter.setBrush(QColor(QStringLiteral("#eaf3ff")));
    const QRectF box(3.0, 4.0, 14.0, 12.0);

    if (dynamic_cast<const Connector *>(shape)) {
        painter.setBrush(Qt::NoBrush);
        painter.drawLine(QPointF(4.0, 15.0), QPointF(16.0, 5.0));
        painter.drawEllipse(QPointF(4.0, 15.0), 2.0, 2.0);
        painter.drawEllipse(QPointF(16.0, 5.0), 2.0, 2.0);
    } else if (dynamic_cast<const DualArrowShape *>(shape)) {
        painter.setBrush(Qt::NoBrush);
        painter.drawLine(QPointF(4.0, 10.0), QPointF(16.0, 10.0));
        painter.drawLine(QPointF(4.0, 10.0), QPointF(8.0, 6.0));
        painter.drawLine(QPointF(4.0, 10.0), QPointF(8.0, 14.0));
        painter.drawLine(QPointF(16.0, 10.0), QPointF(12.0, 6.0));
        painter.drawLine(QPointF(16.0, 10.0), QPointF(12.0, 14.0));
    } else if (dynamic_cast<const ArrowShape *>(shape)) {
        painter.setBrush(Qt::NoBrush);
        painter.drawLine(QPointF(3.0, 14.0), QPointF(16.0, 6.0));
        painter.drawLine(QPointF(16.0, 6.0), QPointF(11.0, 6.0));
        painter.drawLine(QPointF(16.0, 6.0), QPointF(14.0, 11.0));
    } else if (dynamic_cast<const LineShape *>(shape)) {
        painter.setBrush(Qt::NoBrush);
        painter.drawLine(QPointF(3.0, 15.0), QPointF(17.0, 5.0));
    } else if (dynamic_cast<const EllipseShape *>(shape)) {
        painter.drawEllipse(box);
    } else if (dynamic_cast<const DiamondShape *>(shape)) {
        QPolygonF diamond;
        diamond << QPointF(10.0, 2.5) << QPointF(17.0, 10.0)
                << QPointF(10.0, 17.5) << QPointF(3.0, 10.0);
        painter.drawPolygon(diamond);
    } else if (dynamic_cast<const TextLabel *>(shape)) {
        painter.setBrush(Qt::NoBrush);
        QFont font = painter.font();
        font.setBold(true);
        font.setPixelSize(15);
        painter.setFont(font);
        painter.drawText(QRectF(1.0, 1.0, 18.0, 18.0), Qt::AlignCenter, QStringLiteral("T"));
    } else if (dynamic_cast<const RectShape *>(shape)) {
        painter.drawRoundedRect(box, 1.5, 1.5);
    } else {
        painter.drawRoundedRect(box, 3.0, 3.0);
    }
    return QIcon(pixmap);
}

} // namespace

ShapeExplorerTree::ShapeExplorerTree(QWidget *parent)
    : QTreeWidget(parent)
{
    setColumnCount(3);
    setHeaderLabels({QStringLiteral("显示"), QStringLiteral("图形"), QStringLiteral("锁定")});
    header()->setSectionResizeMode(ShapeExplorerWidget::VisibleColumn, QHeaderView::ResizeToContents);
    header()->setSectionResizeMode(ShapeExplorerWidget::NameColumn, QHeaderView::Stretch);
    header()->setSectionResizeMode(ShapeExplorerWidget::LockedColumn, QHeaderView::ResizeToContents);
    setRootIsDecorated(false);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setDragEnabled(true);
    viewport()->setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::InternalMove);
    setDefaultDropAction(Qt::MoveAction);
}

void ShapeExplorerTree::startDrag(Qt::DropActions supportedActions)
{
    // 列表允许多选以便批量操作画布，但层级重排一次只接受一个来源。
    // QTreeWidget 的 InternalMove 会同时移动所有选中行，而层级模型需要明确
    // 的单一来源；直接拒绝多项拖动可避免视图顺序与实际 Z 值不一致。
    const QList<QTreeWidgetItem *> selection = selectedItems();
    if (selection.size() != 1) {
        m_draggedShapeId = -1;
        return;
    }

    // currentItem() 并不保证仍属于 selection（例如程序化同步选择后）；拖拽来源
    // 必须以唯一选中项为准，否则可能重排错误的图形。
    QTreeWidgetItem *source = selection.constFirst();
    setCurrentItem(source);
    m_draggedShapeId = source
        ? source->data(ShapeExplorerWidget::NameColumn,
                       ShapeExplorerWidget::ShapeIdRole).toInt()
        : -1;
    if (m_draggedShapeId >= 0)
        QTreeWidget::startDrag(supportedActions);
}

void ShapeExplorerTree::dropEvent(QDropEvent *event)
{
    QTreeWidgetItem *target = itemAt(event->position().toPoint());
    QTreeWidgetItem *source = currentItem();
    const int sourceId = m_draggedShapeId >= 0
        ? m_draggedShapeId
        : (source ? source->data(ShapeExplorerWidget::NameColumn,
                                ShapeExplorerWidget::ShapeIdRole).toInt() : -1);
    if (sourceId < 0) {
        event->ignore();
        m_draggedShapeId = -1;
        return;
    }

    const int targetId = target
        ? target->data(ShapeExplorerWidget::NameColumn,
                       ShapeExplorerWidget::ShapeIdRole).toInt()
        : -1;
    if (target && sourceId == targetId) {
        event->ignore();
        m_draggedShapeId = -1;
        return;
    }

    const bool mergeWithTarget =
        target && dropIndicatorPosition() == QAbstractItemView::OnItem;
    if (mergeWithTarget) {
        event->acceptProposedAction();
    } else {
        // 交给 Qt 原生 InternalMove 更新行顺序；target 为空时代表拖到
        // 视口底部空白区，同样是合法的“移到最底层”。
        QTreeWidget::dropEvent(event);
        if (!event->isAccepted()) {
            m_draggedShapeId = -1;
            return;
        }
    }

    emit itemDropped(sourceId, targetId, mergeWithTarget);
    m_draggedShapeId = -1;
}

ShapeExplorerWidget::ShapeExplorerWidget(QWidget *parent)
    : QWidget(parent)
    , m_refreshTimer(new QTimer(this))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    auto *title = new QLabel(QStringLiteral("图形与图层"), this);
    title->setStyleSheet(QStringLiteral(
        "font-size: 14px; font-weight: 600; color: #303133; padding: 4px;"));
    layout->addWidget(title);

    m_tree = new ShapeExplorerTree(this);
    m_tree->setObjectName(QStringLiteral("ShapeExplorerTree"));
    m_tree->setAlternatingRowColors(false);
    m_tree->setStyleSheet(QStringLiteral(
        "QTreeWidget { border: 1px solid #e4e7ed; border-radius: 8px; "
        "background: #ffffff; color: #303133; }"
        "QTreeWidget::item { min-height: 28px; }"
        "QTreeWidget::item:hover { background: #ecf5ff; }"
        "QTreeWidget::item:selected { background: #409eff; color: white; }"));
    m_tree->setToolTip(QStringLiteral(
        "拖到项目上可合并图层；拖到项目间隙或列表底部可改变层级；"
        "双击名称可重命名。层级拖动时请仅选择一个图形。"));
    layout->addWidget(m_tree);

    m_refreshTimer->setSingleShot(true);
    m_refreshTimer->setInterval(0);
    connect(m_refreshTimer, &QTimer::timeout, this, &ShapeExplorerWidget::refresh);

    connect(m_tree, &QTreeWidget::itemChanged, this,
            [this](QTreeWidgetItem *item, int column) {
                if (m_synchronizing)
                    return;
                Shape *shape = shapeForItem(item);
                if (!shape)
                    return;

                m_synchronizing = true;
                ShapePropertyCommand *command = nullptr;
                if (column == VisibleColumn) {
                    const bool next = item->checkState(VisibleColumn) == Qt::Checked;
                    if (next != shape->isVisible())
                        command = new ShapePropertyCommand(shape, ShapePropertyCommand::Property::Visible,
                                                           shape->isVisible(), next,
                                                           QStringLiteral("Toggle Shape Visibility"));
                } else if (column == LockedColumn) {
                    const bool next = item->checkState(LockedColumn) == Qt::Checked;
                    if (next != shape->isLocked())
                        command = new ShapePropertyCommand(shape, ShapePropertyCommand::Property::Locked,
                                                           shape->isLocked(), next,
                                                           next ? QStringLiteral("Lock Shape")
                                                                : QStringLiteral("Unlock Shape"));
                } else if (column == NameColumn) {
                    const QString next = item->text(NameColumn).trimmed();
                    if (next != shape->getName())
                        command = new ShapePropertyCommand(shape, ShapePropertyCommand::Property::Name,
                                                           shape->getName(), next,
                                                           QStringLiteral("Rename Shape"));
                }
                if (command) {
                    if (m_canvas && m_canvas->undoManager())
                        m_canvas->undoManager()->push(command);
                    else {
                        command->redo();
                        delete command;
                    }
                }
                m_synchronizing = false;
                scheduleRefresh();
            });

    connect(m_tree, &QTreeWidget::itemSelectionChanged,
            this, &ShapeExplorerWidget::synchronizeSelectionToScene);
    connect(m_tree, &ShapeExplorerTree::itemDropped,
            this, &ShapeExplorerWidget::applyDrop);
}

void ShapeExplorerWidget::setCanvas(Canvas *canvas)
{
    if (m_canvas == canvas)
        return;

    if (m_canvas) {
        if (m_canvas->undoManager())
            disconnect(m_canvas->undoManager(), nullptr, this, nullptr);
        disconnect(m_canvas, nullptr, this, nullptr);
    }
    m_canvas = canvas;
    setScene(m_canvas ? m_canvas->scene() : nullptr);
    if (m_canvas && m_canvas->undoManager()) {
        connect(m_canvas->undoManager(), &UndoManager::historyChanged,
                this, &ShapeExplorerWidget::scheduleRefresh);
        connect(m_canvas, &QObject::destroyed, this, [this]() {
            m_canvas = nullptr;
            setScene(nullptr);
        });
    }
}

void ShapeExplorerWidget::setScene(QGraphicsScene *scene)
{
    if (m_canvas && scene != m_canvas->scene()) {
        if (m_canvas->undoManager())
            disconnect(m_canvas->undoManager(), nullptr, this, nullptr);
        disconnect(m_canvas, nullptr, this, nullptr);
        m_canvas = nullptr;
    }

    if (m_scene == scene)
        return;

    if (m_scene)
        disconnect(m_scene, nullptr, this, nullptr);

    m_scene = scene;
    if (m_scene) {
        connect(m_scene, &QGraphicsScene::changed, this,
                [this](const QList<QRectF> &) {
                    if (!m_synchronizing)
                        scheduleRefresh();
                });
        connect(m_scene, &QGraphicsScene::selectionChanged,
                this, &ShapeExplorerWidget::synchronizeSelectionFromScene);
        connect(m_scene, &QObject::destroyed, this, [this]() {
            m_scene = nullptr;
            scheduleRefresh();
        });
    }
    refresh();
}

void ShapeExplorerWidget::scheduleRefresh()
{
    if (!m_synchronizing && !m_refreshTimer->isActive())
        m_refreshTimer->start();
}

void ShapeExplorerWidget::refresh()
{
    if (m_synchronizing)
        return;

    QList<int> selectedIds;
    if (m_scene) {
        for (QGraphicsItem *selectedItem : m_scene->selectedItems()) {
            if (auto *shape = dynamic_cast<Shape *>(selectedItem))
                selectedIds.append(shape->getID());
        }
    }

    QList<Shape *> shapes;
    if (m_scene) {
        for (QGraphicsItem *item : m_scene->items()) {
            auto *shape = dynamic_cast<Shape *>(item);
            if (shape && !shape->parentItem())
                shapes.append(shape);
        }
    }
    std::sort(shapes.begin(), shapes.end(), [](const Shape *left, const Shape *right) {
        if (left->zValue() != right->zValue())
            return left->zValue() > right->zValue();
        return left->getID() > right->getID();
    });

    m_synchronizing = true;
    m_tree->clear();
    m_shapes.clear();

    int previousLayer = 0;
    bool firstLayer = true;
    int layerGroup = -1;
    for (Shape *shape : shapes) {
        if (firstLayer || shape->getLayer() != previousLayer) {
            ++layerGroup;
            previousLayer = shape->getLayer();
            firstLayer = false;
        }

        auto *item = new QTreeWidgetItem(m_tree);
        item->setFlags(item->flags() | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled |
                       Qt::ItemIsEditable | Qt::ItemIsUserCheckable |
                       Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        item->setCheckState(VisibleColumn, shape->isVisible() ? Qt::Checked : Qt::Unchecked);
        item->setCheckState(LockedColumn, shape->isLocked() ? Qt::Checked : Qt::Unchecked);

        const QString displayName = shape->getName().trimmed().isEmpty()
            ? QStringLiteral("图形 #%1").arg(shape->getID())
            : shape->getName();
        item->setText(NameColumn, displayName);
        item->setIcon(NameColumn, iconForShape(shape));
        item->setToolTip(VisibleColumn, QStringLiteral("显示或隐藏图形"));
        item->setToolTip(LockedColumn, QStringLiteral("锁定位置、大小、旋转和缩放"));
        item->setToolTip(NameColumn,
                         QStringLiteral("ID: %1\n图层: %2\n双击名称可重命名")
                             .arg(shape->getID()).arg(shape->getLayer()));
        item->setData(NameColumn, ShapeIdRole, shape->getID());
        item->setData(NameColumn, LayerRole, shape->getLayer());
        item->setTextAlignment(VisibleColumn, Qt::AlignCenter);
        item->setTextAlignment(LockedColumn, Qt::AlignCenter);

        const QColor background = (layerGroup % 2 == 0)
            ? QColor(QStringLiteral("#ffffff"))
            : QColor(QStringLiteral("#f2f6fc"));
        for (int column = 0; column < m_tree->columnCount(); ++column)
            item->setBackground(column, background);

        item->setSelected(selectedIds.contains(shape->getID()));
        m_shapes.insert(shape->getID(), shape);
    }
    m_synchronizing = false;
}

Shape *ShapeExplorerWidget::shapeForItem(const QTreeWidgetItem *item) const
{
    if (!item)
        return nullptr;
    return shapeById(item->data(NameColumn, ShapeIdRole).toInt());
}

Shape *ShapeExplorerWidget::shapeById(int shapeId) const
{
    const auto iterator = m_shapes.constFind(shapeId);
    return iterator == m_shapes.constEnd() ? nullptr : iterator.value().data();
}

void ShapeExplorerWidget::synchronizeSelectionFromScene()
{
    if (m_synchronizing || !m_scene)
        return;

    m_synchronizing = true;
    for (int row = 0; row < m_tree->topLevelItemCount(); ++row) {
        QTreeWidgetItem *item = m_tree->topLevelItem(row);
        Shape *shape = shapeForItem(item);
        item->setSelected(shape && shape->isSelected());
    }
    m_synchronizing = false;
}

void ShapeExplorerWidget::synchronizeSelectionToScene()
{
    if (m_synchronizing || !m_scene)
        return;

    m_synchronizing = true;
    const QList<QTreeWidgetItem *> selectedItems = m_tree->selectedItems();
    for (auto iterator = m_shapes.constBegin(); iterator != m_shapes.constEnd(); ++iterator) {
        if (Shape *shape = iterator.value().data())
            shape->setSelected(false);
    }
    for (QTreeWidgetItem *item : selectedItems) {
        if (Shape *shape = shapeForItem(item))
            shape->setSelected(true);
    }
    m_synchronizing = false;
}

void ShapeExplorerWidget::applyDrop(int sourceShapeId, int targetShapeId,
                                    bool mergeWithTarget)
{
    Shape *source = shapeById(sourceShapeId);
    Shape *target = shapeById(targetShapeId);
    if (!source || (mergeWithTarget && !target))
        return;

    QList<QPair<Shape *, int>> changes;
    if (mergeWithTarget) {
        if (source->getLayer() != target->getLayer())
            changes.append({source, target->getLayer()});
    } else {
        changes = normalizedLayerChanges(sourceShapeId);
    }

    m_synchronizing = true;
    if (!changes.isEmpty()) {
        auto *group = new QUndoCommand(mergeWithTarget
                                          ? QStringLiteral("Merge Shape Layer")
                                          : QStringLiteral("Reorder Shape Layers"));
        for (const auto &change : changes) {
            new ShapePropertyCommand(change.first, ShapePropertyCommand::Property::Layer,
                                     change.first->getLayer(), change.second,
                                     QStringLiteral("Change Shape Layer"), group);
        }
        if (m_canvas && m_canvas->undoManager())
            m_canvas->undoManager()->push(group);
        else {
            group->redo();
            delete group;
        }
    }
    m_synchronizing = false;

    refresh();
    for (int row = 0; row < m_tree->topLevelItemCount(); ++row) {
        QTreeWidgetItem *item = m_tree->topLevelItem(row);
        if (item->data(NameColumn, ShapeIdRole).toInt() == sourceShapeId) {
            item->setSelected(true);
            m_tree->setCurrentItem(item);
            break;
        }
    }
}

QList<QPair<Shape *, int>> ShapeExplorerWidget::normalizedLayerChanges(int sourceShapeId) const
{
    QList<QPair<Shape *, int>> changes;
    const int count = m_tree->topLevelItemCount();
    int nextLayer = count + 1;
    QHash<int, int> preservedLayerGroups;

    for (int row = 0; row < count; ++row) {
        QTreeWidgetItem *item = m_tree->topLevelItem(row);
        Shape *shape = shapeForItem(item);
        if (!shape)
            continue;

        const int shapeId = item->data(NameColumn, ShapeIdRole).toInt();
        int newLayer = 0;
        if (shapeId == sourceShapeId) {
            newLayer = nextLayer--;
        } else {
            const int oldLayer = item->data(NameColumn, LayerRole).toInt();
            if (!preservedLayerGroups.contains(oldLayer))
                preservedLayerGroups.insert(oldLayer, nextLayer--);
            newLayer = preservedLayerGroups.value(oldLayer);
        }
        if (newLayer != shape->getLayer())
            changes.append({shape, newLayer});
    }
    return changes;
}
