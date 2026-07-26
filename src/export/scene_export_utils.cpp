#include "scene_export_utils.h"

#include <QGraphicsItem>
#include <QGraphicsScene>

#include <algorithm>
#include <cmath>

namespace SceneExport {

ScopedOverlayHider::ScopedOverlayHider(QGraphicsScene *scene)
{
    if (!scene)
        return;

    const QList<QGraphicsItem *> items = scene->items();
    for (QGraphicsItem *item : items) {
        if (!item || !item->data(ExcludeFromExportRole).toBool())
            continue;

        m_items.append({item, item->isVisible()});
        item->setVisible(false);
    }
}

ScopedOverlayHider::~ScopedOverlayHider()
{
    // 子图元可能与父图元同时被标记；逆序恢复可避免父项先显示时短暂暴露子项。
    for (auto iterator = m_items.crbegin(); iterator != m_items.crend(); ++iterator) {
        if (iterator->item)
            iterator->item->setVisible(iterator->wasVisible);
    }
}

QRectF visibleItemsBoundingRect(QGraphicsScene *scene)
{
    if (!scene)
        return {};

    QRectF bounds;
    bool hasVisibleItem = false;
    const QList<QGraphicsItem *> items = scene->items();
    for (QGraphicsItem *item : items) {
        if (!item || !item->isVisible())
            continue;

        const QRectF itemBounds = item->sceneBoundingRect();
        if (!itemBounds.isValid() || itemBounds.isEmpty())
            continue;

        bounds = hasVisibleItem ? bounds.united(itemBounds) : itemBounds;
        hasVisibleItem = true;
    }
    return bounds;
}

QSize boundedOutputSize(const QSizeF &sourceSize,
                        qreal preferredScale,
                        int maxOutputDimension)
{
    if (!sourceSize.isValid() || sourceSize.isEmpty()
        || !std::isfinite(sourceSize.width()) || !std::isfinite(sourceSize.height())
        || !std::isfinite(preferredScale) || preferredScale <= 0.0
        || maxOutputDimension <= 0) {
        return {};
    }

    const qreal maximumSide = std::max(sourceSize.width(), sourceSize.height());
    const qreal scale = std::min(preferredScale, qreal(maxOutputDimension) / maximumSide);
    if (!std::isfinite(scale) || scale <= 0.0)
        return {};

    const auto scaledDimension = [scale, maxOutputDimension](qreal dimension) {
        return std::clamp(int(std::ceil(dimension * scale)), 1, maxOutputDimension);
    };
    return QSize(scaledDimension(sourceSize.width()), scaledDimension(sourceSize.height()));
}

} // namespace SceneExport
