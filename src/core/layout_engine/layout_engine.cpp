#include "layout_engine.h"
#include <cmath>
#include <algorithm>

// 辅助函数：计算图形的 Y 轴中心线坐标
static qreal CalculateHorizontalCenterLinePos(const Shape* shape)
{
    if (!shape) return 0.0;
    return shape->getPosition().y() + shape->getSize().height() / 2.0;
}

static qreal CalculateVerticalCenterLinePos(const Shape* shape)
{
    if (!shape) return 0.0;
    return shape->getPosition().x() + shape->getSize().width() / 2.0;
}

// 辅助函数：计算两个图元在指定方向上的净边缘间距（Clearance Gap）
static qreal CalculateSensedSpacing(const Shape* first, const Shape* second, bool isHorizontal, qreal defaultGap = 20.0)
{
    if (!first || !second || first == second) return defaultGap;

    qreal gap = 0.0;
    if (isHorizontal) {
        qreal x1 = first->pos().x();
        qreal w1 = first->getSize().width();
        qreal x2 = second->pos().x();
        qreal w2 = second->getSize().width();

        if (x2 >= x1) {
            // second 在 first 右侧（或左边界对齐）
            gap = x2 - (x1 + w1);
        } else {
            // second 在 first 左侧
            gap = x1 - (x2 + w2);
        }
    } else {
        qreal y1 = first->pos().y();
        qreal h1 = first->getSize().height();
        qreal y2 = second->pos().y();
        qreal h2 = second->getSize().height();

        if (y2 >= y1) {
            // second 在 first 下方
            gap = y2 - (y1 + h1);
        } else {
            // second 在 first 上方
            gap = y1 - (y2 + h2);
        }
    }

    // 若重合或相交（gap <= 0），采用默认间距
    return (gap > 0.0) ? gap : defaultGap;
}

static LayoutConfig SanitizeConfig(const LayoutConfig& cfg)
{
    LayoutConfig sanitized = cfg;
    if (!std::isfinite(sanitized.spacing) || sanitized.spacing < 0.0) {
        sanitized.spacing = -1.0;
    }
    if (!std::isfinite(sanitized.gridHorizontalSpacing) || sanitized.gridHorizontalSpacing < 0.0) {
        sanitized.gridHorizontalSpacing = -1.0;
    }
    if (!std::isfinite(sanitized.gridVerticalSpacing) || sanitized.gridVerticalSpacing < 0.0) {
        sanitized.gridVerticalSpacing = -1.0;
    }
    return sanitized;
}

LayoutEngine::LayoutEngine(const LayoutConfig& config)
    : m_config(SanitizeConfig(config))
{}

LayoutEngine::~LayoutEngine() = default;

void LayoutEngine::setConfig(const LayoutConfig& config)
{
    m_config = SanitizeConfig(config);
}

bool LayoutEngine::isFiniteSize(const QSizeF& size)
{
    return std::isfinite(size.width()) && std::isfinite(size.height()) &&
           size.width() >= 0.0 && size.height() >= 0.0;
}

bool LayoutEngine::isFinitePoint(const QPointF& point)
{
    return std::isfinite(point.x()) && std::isfinite(point.y());
}

bool LayoutEngine::isValidShapeGeometry(const Shape* shape)
{
    if (!shape) return false;
    return isFinitePoint(shape->pos()) && isFiniteSize(shape->getSize());
}

LayoutSnapshot LayoutEngine::alignHorizontal(const QList<Shape*>& items,
                                             Shape* primary,
                                             std::optional<LayoutConfig> layoutConfig) const
{
    LayoutSnapshot snapshot;
    QList<Shape*> validItems;
    for (Shape* s : items) {
        if (isValidShapeGeometry(s) && s->supportsLayoutPosition()) {
            validItems.append(s);
        }
    }
    if (validItems.isEmpty()) return snapshot;

    LayoutConfig effectiveConfig = SanitizeConfig(layoutConfig.has_value() ? layoutConfig.value() : m_config);

    // 1. 确定起算基准图元与中轴 Y 坐标
    Shape* baseShape = (primary && validItems.contains(primary)) ? primary : validItems.first();
    qreal refCenterY = CalculateHorizontalCenterLinePos(baseShape);

    // 2. 智能感知有效间距
    qreal effectiveSpacing = 20.0;
    if (effectiveConfig.spacing >= 0.0) {
        effectiveSpacing = effectiveConfig.spacing;
    } else if (validItems.size() >= 2) {
        effectiveSpacing = CalculateSensedSpacing(validItems[0], validItems[1], true);
    }

    // 3. 计算起始 X 坐标，确保 baseShape (primary 或首位) 沿平铺轴方向位置保持不变
    qsizetype baseIndex = validItems.indexOf(baseShape);
    qreal startXPos = baseShape->pos().x();
    for (qsizetype i = 0; i < baseIndex; ++i) {
        startXPos -= (validItems[i]->getSize().width() + effectiveSpacing);
    }

    qreal lastXPos = startXPos;
    for (Shape* s : validItems) {
        if (s->isLocked()) {
            // 遇到锁定的图元，平铺 X 坐标不得倒退，确保跳至锁定元素右侧或当前位置之后
            lastXPos = std::max(lastXPos, s->pos().x() + s->getSize().width() + effectiveSpacing);
            continue;
        }

        QPointF newPos(lastXPos, refCenterY - s->getSize().height() * 0.5);
        if (isFinitePoint(newPos) && (std::abs(newPos.x() - s->pos().x()) > 1e-4 || std::abs(newPos.y() - s->pos().y()) > 1e-4)) {
            snapshot.append({s, s->pos(), newPos, s->getSize(), s->getSize()});
        }

        lastXPos += s->getSize().width() + effectiveSpacing;
    }

    return snapshot;
}

LayoutSnapshot LayoutEngine::alignVertical(const QList<Shape*>& items,
                                           Shape* primary,
                                           std::optional<LayoutConfig> layoutConfig) const
{
    LayoutSnapshot snapshot;
    QList<Shape*> validItems;
    for (Shape* s : items) {
        if (isValidShapeGeometry(s) && s->supportsLayoutPosition()) {
            validItems.append(s);
        }
    }
    if (validItems.isEmpty()) return snapshot;

    LayoutConfig effectiveConfig = SanitizeConfig(layoutConfig.has_value() ? layoutConfig.value() : m_config);

    // 1. 确定起算基准图元与中轴 X 坐标
    Shape* baseShape = (primary && validItems.contains(primary)) ? primary : validItems.first();
    qreal refCenterX = CalculateVerticalCenterLinePos(baseShape);

    // 2. 智能感知有效间距
    qreal effectiveSpacing = 20.0;
    if (effectiveConfig.spacing >= 0.0) {
        effectiveSpacing = effectiveConfig.spacing;
    } else if (validItems.size() >= 2) {
        effectiveSpacing = CalculateSensedSpacing(validItems[0], validItems[1], false);
    }

    // 3. 计算起始 Y 坐标，确保 baseShape (primary 或首位) 沿平铺轴方向位置保持不变
    qsizetype baseIndex = validItems.indexOf(baseShape);
    qreal startYPos = baseShape->pos().y();
    for (qsizetype i = 0; i < baseIndex; ++i) {
        startYPos -= (validItems[i]->getSize().height() + effectiveSpacing);
    }

    qreal lastYPos = startYPos;
    for (Shape* s : validItems) {
        if (s->isLocked()) {
            // 遇到锁定的图元，平铺 Y 坐标不得倒退
            lastYPos = std::max(lastYPos, s->pos().y() + s->getSize().height() + effectiveSpacing);
            continue;
        }

        QPointF newPos(refCenterX - s->getSize().width() * 0.5, lastYPos);
        if (isFinitePoint(newPos) && (std::abs(newPos.x() - s->pos().x()) > 1e-4 || std::abs(newPos.y() - s->pos().y()) > 1e-4)) {
            snapshot.append({s, s->pos(), newPos, s->getSize(), s->getSize()});
        }

        lastYPos += s->getSize().height() + effectiveSpacing;
    }

    return snapshot;
}

LayoutSnapshot LayoutEngine::arrangeGrid(const QList<Shape*>& items,
                                         Shape* primary,
                                         std::optional<LayoutConfig> layoutConfig) const
{
    LayoutSnapshot snapshot;
    QList<Shape*> validItems;
    for (Shape* s : items) {
        if (isValidShapeGeometry(s) && s->supportsLayoutPosition()) {
            validItems.append(s);
        }
    }
    const qsizetype count = validItems.size();
    if (count == 0) return snapshot;

    LayoutConfig effectiveConfig = SanitizeConfig(layoutConfig.has_value() ? layoutConfig.value() : m_config);

    Shape* baseShape = (primary && validItems.contains(primary)) ? primary : validItems.first();
    QPointF startPos = baseShape->pos();

    // 1. 智能感知网格间距
    qreal rawSpacingX = effectiveConfig.gridHorizontalSpacing;
    qreal rawSpacingY = effectiveConfig.gridVerticalSpacing;

    qreal effSpacingX = 20.0;
    qreal effSpacingY = 20.0;
    if (rawSpacingX >= 0.0 && rawSpacingY >= 0.0) {
        effSpacingX = rawSpacingX;
        effSpacingY = rawSpacingY;
    } else if (count >= 2) {
        qreal hGap = CalculateSensedSpacing(validItems[0], validItems[1], true);
        qreal vGap = CalculateSensedSpacing(validItems[0], validItems[1], false);
        qreal minGap = std::min(hGap, vGap);
        effSpacingX = (rawSpacingX >= 0.0) ? rawSpacingX : minGap;
        effSpacingY = (rawSpacingY >= 0.0) ? rawSpacingY : minGap;
    }

    // 2. 确定行列数与排布顺序（强行钳制 cols 范围防大内存越界）
    int cols = effectiveConfig.gridCols;
    if (cols <= 0) {
        cols = static_cast<int>(std::ceil(std::sqrt(static_cast<double>(count))));
    }
    cols = std::clamp(cols, 1, static_cast<int>(count));
    const int rows = (static_cast<int>(count) + cols - 1) / cols;

    bool isRowMajor = (effectiveConfig.gridOrder == LayoutConfig::GridOrder::RowMajor);

    // 3. 严格按用户列表顺序计算每行/每列的最大宽度与高度
    QVector<qreal> colWidths(cols, 0.0);
    QVector<qreal> rowHeights(rows, 0.0);

    for (qsizetype i = 0; i < count; ++i) {
        int r = isRowMajor ? (static_cast<int>(i) / cols) : (static_cast<int>(i) % rows);
        int c = isRowMajor ? (static_cast<int>(i) % cols) : (static_cast<int>(i) / rows);
        colWidths[c] = std::max(colWidths[c], validItems[i]->getSize().width());
        rowHeights[r] = std::max(rowHeights[r], validItems[i]->getSize().height());
    }

    // 4. 累加计算各单元格左上角初始偏移
    QVector<qreal> colX(cols, startPos.x());
    QVector<qreal> rowY(rows, startPos.y());

    qreal currX = startPos.x();
    for (int c = 0; c < cols; ++c) {
        colX[c] = currX;
        currX += colWidths[c] + effSpacingX;
    }

    qreal currY = startPos.y();
    for (int r = 0; r < rows; ++r) {
        rowY[r] = currY;
        currY += rowHeights[r] + effSpacingY;
    }

    // 5. 对齐基准项：平移所有单元格偏移，使得 baseShape 所在行列单元精确保持在 startPos
    qsizetype baseIndex = validItems.indexOf(baseShape);
    int baseRow = isRowMajor ? (static_cast<int>(baseIndex) / cols) : (static_cast<int>(baseIndex) % rows);
    int baseCol = isRowMajor ? (static_cast<int>(baseIndex) % cols) : (static_cast<int>(baseIndex) / rows);

    qreal shiftX = startPos.x() - colX[baseCol];
    qreal shiftY = startPos.y() - rowY[baseRow];
    for (int c = 0; c < cols; ++c) colX[c] += shiftX;
    for (int r = 0; r < rows; ++r) rowY[r] += shiftY;

    // 6. 组装网格快照
    for (qsizetype i = 0; i < count; ++i) {
        Shape* s = validItems[i];
        if (s->isLocked()) continue;

        int r = isRowMajor ? (static_cast<int>(i) / cols) : (static_cast<int>(i) % rows);
        int c = isRowMajor ? (static_cast<int>(i) % cols) : (static_cast<int>(i) / rows);

        QPointF targetPos(colX[c], rowY[r]);
        if (isFinitePoint(targetPos) && targetPos != s->pos()) {
            snapshot.append({s, s->pos(), targetPos, s->getSize(), s->getSize()});
        }
    }

    return snapshot;
}

LayoutSnapshot LayoutEngine::matchSize(const QList<Shape*>& items,
                                       Shape* primary,
                                       std::optional<LayoutConfig> layoutConfig) const
{
    LayoutSnapshot snapshot;
    QList<Shape*> validItems;
    for (Shape* s : items) {
        if (isValidShapeGeometry(s) && s->supportsLayoutSize()) {
            validItems.append(s);
        }
    }
    if (validItems.isEmpty()) return snapshot;

    Shape* baseShape = (primary && validItems.contains(primary)) ? primary : validItems.first();
    if (!baseShape) return snapshot;

    LayoutConfig effectiveConfig = SanitizeConfig(layoutConfig.has_value() ? layoutConfig.value() : m_config);

    QSizeF targetSize = baseShape->getSize();
    LayoutConfig::Mode mode = effectiveConfig.mode;
    bool matchW = (mode == LayoutConfig::Mode::MatchWidth || mode == LayoutConfig::Mode::MatchSize || mode == LayoutConfig::Mode::Null);
    bool matchH = (mode == LayoutConfig::Mode::MatchHeight || mode == LayoutConfig::Mode::MatchSize || mode == LayoutConfig::Mode::Null);

    for (Shape* s : validItems) {
        if (s == baseShape || s->isLocked()) continue;

        qreal newW = matchW ? targetSize.width() : s->getSize().width();
        qreal newH = matchH ? targetSize.height() : s->getSize().height();
        QSizeF newSize(newW, newH);

        if (isFiniteSize(newSize) && (std::abs(newW - s->getSize().width()) > 1e-4 || std::abs(newH - s->getSize().height()) > 1e-4)) {
            QPointF expectedNewPos = s->pos() + QPointF((s->getSize().width() - newW) * 0.5, (s->getSize().height() - newH) * 0.5);
            if (isFinitePoint(expectedNewPos)) {
                snapshot.append({s, s->pos(), expectedNewPos, s->getSize(), newSize});
            }
        }
    }

    return snapshot;
}

bool LayoutEngine::applyLayoutSnapshot(const LayoutSnapshot& snapshot)
{
    // 1. 预校验阶段：检查快照项有效性以及待修改图元是否锁定/不支持布局/数值非法
    for (const LayoutItemChange& change : snapshot) {
        if (!change.shape || !isValidShapeGeometry(change.shape)) return false; // 目标图元已被销毁、空悬或几何非法
        if (!isFinitePoint(change.newPos) || !isFiniteSize(change.newSize) ||
            !isFinitePoint(change.oldPos) || !isFiniteSize(change.oldSize)) {
            return false;
        }

        bool sizeChanges = (std::abs(change.newSize.width() - change.shape->getSize().width()) > 1e-4 ||
                            std::abs(change.newSize.height() - change.shape->getSize().height()) > 1e-4);
        bool posChanges  = (std::abs(change.newPos.x() - change.shape->pos().x()) > 1e-4 ||
                            std::abs(change.newPos.y() - change.shape->pos().y()) > 1e-4);

        if ((sizeChanges || posChanges) && change.shape->isLocked()) {
            return false; // 严禁修改已被锁定的图元
        }
        if (sizeChanges && !change.shape->supportsLayoutSize()) return false;
        if (posChanges && !change.shape->supportsLayoutPosition()) return false;
    }

    // 2. 事务执行阶段与回滚准备
    bool modified = false;
    QList<LayoutItemChange> appliedChanges;

    // [P1 Fix] 回滚必须遵循“先恢复尺寸，再恢复位置”，避免 Shape::setSize() 为保持几何中心导致 pos 再次偏移
    auto rollback = [](const QList<LayoutItemChange>& applied) -> bool {
        bool allRolledBack = true;
        for (auto it = applied.rbegin(); it != applied.rend(); ++it) {
            if (it->shape) {
                bool sOk = it->shape->setSize(it->oldSize);
                bool pOk = it->shape->setPosition(it->oldPos);
                if (!sOk || !pOk) allRolledBack = false;
            }
        }
        return allRolledBack;
    };

    for (const LayoutItemChange& change : snapshot) {
        if (!change.shape) continue;

        bool sizeChanges = (std::abs(change.newSize.width() - change.shape->getSize().width()) > 1e-4 ||
                            std::abs(change.newSize.height() - change.shape->getSize().height()) > 1e-4);
        bool posChanges  = (std::abs(change.newPos.x() - change.shape->pos().x()) > 1e-4 ||
                            std::abs(change.newPos.y() - change.shape->pos().y()) > 1e-4);

        if (!sizeChanges && !posChanges) continue;

        QPointF prevPos = change.shape->pos();
        QSizeF prevSize = change.shape->getSize();

        // [P1 Fix] 修改当前项之前，先在 appliedChanges 中记录当前项的旧状态。
        // 若当前项后续的 setSize 或 setPosition 失败，rollback(appliedChanges) 才能包含该项本身并彻底恢复。
        appliedChanges.append({change.shape, prevPos, prevPos, prevSize, prevSize});

        if (sizeChanges) {
            if (!change.shape->setSize(change.newSize)) {
                rollback(appliedChanges);
                return false;
            }
            modified = true;
        }

        // [P1 Fix] 无论是因为原始项位置变化，还是因为上面调用了 Shape::setSize() 保持场景中心而引发 pos() 自动偏移，
        // 只要当前实际 pos() 偏离了目标 newPos，就务必调用 setPosition(newPos) 精准校核应用。
        if (std::abs(change.newPos.x() - change.shape->pos().x()) > 1e-4 ||
            std::abs(change.newPos.y() - change.shape->pos().y()) > 1e-4) {
            if (!change.shape->setPosition(change.newPos)) {
                rollback(appliedChanges);
                return false;
            }
            modified = true;
        }

        // 成功应用后更新 appliedChanges 记录为实际的新几何状态
        appliedChanges.last().newPos = change.shape->pos();
        appliedChanges.last().newSize = change.shape->getSize();
    }

    return modified;
}