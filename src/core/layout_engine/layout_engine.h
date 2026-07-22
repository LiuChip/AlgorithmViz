#ifndef LAYOUT_ENGINE_H
#define LAYOUT_ENGINE_H

#include <QList>
#include <QPointF>
#include <QSizeF>
#include <QPointer>
#include <optional>
#include "shapes/shape.h"

// 记录单图元布局变更快照项
struct LayoutItemChange {
    QPointer<Shape> shape;
    QPointF oldPos;
    QPointF newPos;
    QSizeF oldSize;
    QSizeF newSize;
};

using LayoutSnapshot = QList<LayoutItemChange>;

struct LayoutConfig {
    enum class Mode {
        HorizontalAlignment,
        VerticalAlignment,
        GridAlignment,
        MatchWidth,
        MatchHeight,
        MatchSize,
        Null
    };

    enum class GridOrder {
        RowMajor,    // 行优先：自左向右，再下行
        ColumnMajor  // 列优先：自上而下，再下列
    };

    Mode mode = Mode::Null;
    GridOrder gridOrder = GridOrder::RowMajor;
    qreal spacing = -1.0;               // -1.0 代表自动感应第1和第2个图元的净间距
    qreal gridHorizontalSpacing = -1.0; // -1.0 代表自动感应
    qreal gridVerticalSpacing = -1.0;   // -1.0 代表自动感应
    int gridCols = 0;                   // 网格列数 (0 表示自动计算平方根)

    LayoutConfig() = default;
};

class LayoutEngine {
public:
    explicit LayoutEngine(const LayoutConfig& config = LayoutConfig());
    ~LayoutEngine();

    void setConfig(const LayoutConfig& config);
    LayoutConfig config() const { return m_config; }

    static bool isFiniteSize(const QSizeF& size);
    static bool isFinitePoint(const QPointF& point);
    static bool isValidShapeGeometry(const Shape* shape);

    static bool applyLayoutSnapshot(const LayoutSnapshot& snapshot);

    LayoutSnapshot alignHorizontal(const QList<Shape*>& items,
                                   Shape* primary = nullptr,
                                   std::optional<LayoutConfig> layoutConfig = std::nullopt) const;
    LayoutSnapshot alignVertical(const QList<Shape*>& items,
                                 Shape* primary = nullptr,
                                 std::optional<LayoutConfig> layoutConfig = std::nullopt) const;
    LayoutSnapshot arrangeGrid(const QList<Shape*>& items,
                               Shape* primary = nullptr,
                               std::optional<LayoutConfig> layoutConfig = std::nullopt) const;
    LayoutSnapshot matchSize(const QList<Shape*>& items,
                             Shape* primary = nullptr,
                             std::optional<LayoutConfig> layoutConfig = std::nullopt) const;

private:
    LayoutConfig m_config;
};

#endif // LAYOUT_ENGINE_H