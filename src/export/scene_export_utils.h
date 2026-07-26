#ifndef SCENE_EXPORT_UTILS_H
#define SCENE_EXPORT_UTILS_H

#include <QList>
#include <QRectF>
#include <QSize>
#include <QSizeF>

class QGraphicsItem;
class QGraphicsScene;

// 场景导出公共约定。交互控制框、橡皮筋等临时图元可通过 QGraphicsItem::setData()
// 标记为不参与导出，避免导出器反向依赖具体的控制器或控件类型。
namespace SceneExport {

inline constexpr int ExcludeFromExportRole = 0x41565A01; // "AVZ" + role 1

// 在一次同步导出期间隐藏所有带 ExcludeFromExportRole 标记的图元，并在析构时
// 恢复各自原有的可见状态。使用 RAII 可覆盖导出失败和提前返回路径。
class ScopedOverlayHider
{
public:
    explicit ScopedOverlayHider(QGraphicsScene *scene);
    ~ScopedOverlayHider();

    ScopedOverlayHider(const ScopedOverlayHider &) = delete;
    ScopedOverlayHider &operator=(const ScopedOverlayHider &) = delete;

private:
    struct ItemVisibility {
        QGraphicsItem *item = nullptr;
        bool wasVisible = false;
    };

    QList<ItemVisibility> m_items;
};

// 只计算当前实际可见图元的场景边界；临时叠加层应先由 ScopedOverlayHider 隐藏。
QRectF visibleItemsBoundingRect(QGraphicsScene *scene);

// 按首选倍率生成输出尺寸，同时把最长边限制在 maxOutputDimension 以内。
// 无效尺寸或参数返回空 QSize，供位图和矢量导出共享同一套防溢出规则。
QSize boundedOutputSize(const QSizeF &sourceSize,
                        qreal preferredScale,
                        int maxOutputDimension);

} // namespace SceneExport

#endif // SCENE_EXPORT_UTILS_H
