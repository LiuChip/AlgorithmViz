#include <QtTest/QTest>
#include <QList>
#include <QPointF>
#include <QSizeF>
#include "../src/core/layout_engine/layout_engine.h"
#include "../src/shapes/rect_shape.h"
#include "../src/shapes/line_shape.h"
#include "../src/shapes/connector/connector.h"

// 专门用来测试运行时中途执行失败并触发事务逆序回滚的测试用例图元
class FailingShape : public RectShape {
public:
    FailingShape(QPointF p, QSizeF s) : RectShape(p.x(), p.y(), s.width(), s.height()) { setPosition(p); }
    bool failOnSetPosition = true;
    bool setPosition(QPointF p) override {
        if (failOnSetPosition) return false;
        return RectShape::setPosition(p);
    }
    bool setPosition(qreal x, qreal y) override {
        if (failOnSetPosition) return false;
        return RectShape::setPosition(x, y);
    }
};

class LayoutEngineTest : public QObject
{
    Q_OBJECT

private slots:
    // 测试 1: 水平对齐与平铺 - 严格保持用户列表顺序 & 正向/默认间距感知
    void testAlignHorizontalOrderAndSpacing() {
        RectShape shapeA(0, 0, 100, 50); // width 100, pos (0,0)
        RectShape shapeB(130, 20, 80, 40); // width 80, pos (130,20), 右边缘净间距 = 130 - 100 = 30
        RectShape shapeC(50, 100, 60, 60); // 乱序位置 (50,100)

        // 列表顺序：A -> B -> C。不按X轴自动重排
        QList<Shape*> items = { &shapeA, &shapeB, &shapeC };

        LayoutConfig config;
        config.mode = LayoutConfig::Mode::HorizontalAlignment;
        config.spacing = -1.0; // 智能感知 A 和 B 的间距 (应为 30.0)

        LayoutEngine engine(config);
        LayoutSnapshot snapshot = engine.alignHorizontal(items);

        // A 作为起算点，坐标不变
        // B 在 A 右边：x = 0 + 100 + 30 = 130
        // C 在 B 右边：x = 130 + 80 + 30 = 240
        // 中轴线 Y 为 A 的中轴线：y + height/2 = 0 + 25 = 25
        // B 目标 Y = 25 - 40/2 = 5, C 目标 Y = 25 - 60/2 = -5
        bool foundB = false, foundC = false;
        for (const auto& change : snapshot) {
            if (change.shape == &shapeB) {
                foundB = true;
                QCOMPARE(change.newPos.x(), 130.0);
                QCOMPARE(change.newPos.y(), 5.0);
            } else if (change.shape == &shapeC) {
                foundC = true;
                QCOMPARE(change.newPos.x(), 240.0);
                QCOMPARE(change.newPos.y(), -5.0);
            }
        }
        QVERIFY(foundB);
        QVERIFY(foundC);
    }

    // 测试 2: 负向相对位置时的智能感知间距（即选择的第二个在第一个左侧）
    void testAlignHorizontalSmartSpacingNegativeRelative() {
        RectShape shape1(200, 0, 100, 50); // x=200, width=100
        RectShape shape2(50, 0, 120, 50);  // x=50, width=120, 在左边。净间距 = 200 - (50+120) = 30
        RectShape shape3(0, 0, 80, 50);

        QList<Shape*> items = { &shape1, &shape2, &shape3 };

        LayoutEngine engine;
        LayoutSnapshot snapshot = engine.alignHorizontal(items);

        // 有效间距应感知为 30.0
        // shape1 原地保持 (200, 0)
        // shape2 目标 x = 200 + 100 + 30 = 330
        // shape3 目标 x = 330 + 120 + 30 = 480
        for (const auto& change : snapshot) {
            if (change.shape == &shape2) {
                QCOMPARE(change.newPos.x(), 330.0);
            } else if (change.shape == &shape3) {
                QCOMPARE(change.newPos.x(), 480.0);
            }
        }
    }

    // 测试 3: 垂直对齐与平铺 - Primary 优先为基准
    void testAlignVerticalWithPrimary() {
        RectShape shapeA(0, 0, 100, 40);
        RectShape shapeB(10, 60, 80, 50); // A 与 B 的净垂直间距 = 60 - 40 = 20
        RectShape shapeC(20, 150, 60, 60);

        QList<Shape*> items = { &shapeA, &shapeB, &shapeC };

        LayoutConfig config;
        config.mode = LayoutConfig::Mode::VerticalAlignment;
        config.spacing = -1.0;

        LayoutEngine engine(config);
        // 将 shapeB 设为 primary
        LayoutSnapshot snapshot = engine.alignVertical(items, &shapeB);

        // 基准中轴线 X 为 B 的中轴线 = 10 + 80/2 = 50
        // 有效垂直间距由 items[0] 和 items[1] 感知：60 - (0+40) = 20
        // 由于 shapeB 是 primary，基准自 shapeB 开始！
        // 且由于 shapeB 的 Y 为 60：
        // shapeB 原地 x = 50 - 40 = 10, y = 60
        // shapeC 为 next：y = 60 + 50 + 20 = 130, x = 50 - 30 = 20
        bool foundA = false, foundC = false;
        for (const auto& change : snapshot) {
            if (change.shape == &shapeA) {
                foundA = true;
                QCOMPARE(change.newPos.x(), 50.0 - 50.0); // 0.0
            } else if (change.shape == &shapeC) {
                foundC = true;
                QCOMPARE(change.newPos.x(), 50.0 - 30.0); // 20.0
                QCOMPARE(change.newPos.y(), 60.0 + 50.0 + 20.0); // 130.0
            }
        }
        QVERIFY(foundA || foundC);
    }

    // 测试 4: 网格排布 - 行优先与列优先，及两维度最小间距感知
    void testArrangeGridRowMajorAndColumnMajor() {
        RectShape s1(0, 0, 50, 50);
        RectShape s2(65, 70, 50, 50); // 水平间距 65-50=15, 垂直间距 70-50=20 -> min=15
        RectShape s3(0, 0, 50, 50);
        RectShape s4(0, 0, 50, 50);

        QList<Shape*> items = { &s1, &s2, &s3, &s4 };

        LayoutConfig config;
        config.mode = LayoutConfig::Mode::GridAlignment;
        config.gridCols = 2; // 2x2 网格
        config.gridHorizontalSpacing = -1.0;
        config.gridVerticalSpacing = -1.0;
        config.gridOrder = LayoutConfig::GridOrder::RowMajor;

        LayoutEngine engine(config);
        LayoutSnapshot snapRow = engine.arrangeGrid(items);

        // 有效间距均感知为 min(15, 20) = 15
        // 列宽均为 50，行高均为 50
        // s1(0,0), s2(65,0), s3(0,65), s4(65,65)
        for (const auto& change : snapRow) {
            if (change.shape == &s2) {
                QCOMPARE(change.newPos, QPointF(65.0, 0.0));
            } else if (change.shape == &s3) {
                QCOMPARE(change.newPos, QPointF(0.0, 65.0));
            } else if (change.shape == &s4) {
                QCOMPARE(change.newPos, QPointF(65.0, 65.0));
            }
        }

        // 测试列优先
        config.gridOrder = LayoutConfig::GridOrder::ColumnMajor;
        engine.setConfig(config);
        LayoutSnapshot snapCol = engine.arrangeGrid(items);
        // 列优先：s1(0,0), s2(0,65), s3(65,0), s4(65,65)
        for (const auto& change : snapCol) {
            if (change.shape == &s2) {
                QCOMPARE(change.newPos, QPointF(0.0, 65.0));
            } else if (change.shape == &s3) {
                QCOMPARE(change.newPos, QPointF(65.0, 0.0));
            }
        }
    }

    // 测试 5: 锁定图元不可变更及应用快照测试
    void testLockedShapeSkippedAndApplySnapshot() {
        RectShape s1(0, 0, 50, 50);
        RectShape s2(100, 0, 50, 50);
        s2.setLocked(true); // 锁定 s2
        RectShape s3(200, 0, 50, 50);

        QList<Shape*> items = { &s1, &s2, &s3 };

        LayoutConfig config;
        config.spacing = 20.0;
        LayoutEngine engine(config);

        LayoutSnapshot snapshot = engine.alignHorizontal(items);
        // 因为 s2 锁定，它不会出现在 snapshot 中
        for (const auto& change : snapshot) {
            QVERIFY(change.shape != &s2);
        }

        // 验证 applyLayoutSnapshot 在图元已锁定时的安全处理
        LayoutItemChange dummyChange = { &s2, s2.pos(), QPointF(999, 999), s2.getSize(), s2.getSize() };
        LayoutEngine::applyLayoutSnapshot({ dummyChange });
        QCOMPARE(s2.pos(), QPointF(100, 0)); // 保持不变
    }

    // 测试 6: 尺寸统一 (MatchWidth / MatchHeight / MatchSize) 与快照应用
    void testMatchSizeModes() {
        RectShape sA(0, 0, 120, 80);    // 基准图元 sA
        RectShape sB(60, 60, 40, 100);  // 待统一尺寸图元 sB
        RectShape sC(100, 100, 50, 50);
        sC.setLocked(true);             // 锁定 sC，测试锁定过滤

        QList<Shape*> items = { &sA, &sB, &sC };

        LayoutConfig config;
        LayoutEngine engine(config);

        // 1. 测试统一等宽 (MatchWidth)
        config.mode = LayoutConfig::Mode::MatchWidth;
        LayoutSnapshot snapW = engine.matchSize(items, &sA, config);
        QCOMPARE(snapW.size(), 1); // 仅 sB 变更 (sA为基准，sC为锁定)
        QCOMPARE(snapW[0].shape, &sB);
        QCOMPARE(snapW[0].newSize, QSizeF(120.0, 100.0)); // 宽度变为120，高度保持100
        // 左上角偏移：60 + (40 - 120)*0.5 = 20.0
        QCOMPARE(snapW[0].newPos, QPointF(20.0, 60.0));

        // 2. 测试统一等高 (MatchHeight)
        config.mode = LayoutConfig::Mode::MatchHeight;
        LayoutSnapshot snapH = engine.matchSize(items, &sA, config);
        QCOMPARE(snapH.size(), 1);
        QCOMPARE(snapH[0].newSize, QSizeF(40.0, 80.0)); // 宽度保持40，高度变为80
        // 顶部 Y 偏移：60 + (100 - 80)*0.5 = 70.0
        QCOMPARE(snapH[0].newPos, QPointF(60.0, 70.0));

        // 3. 测试完全统一大小 (MatchSize) 并且执行应用与重放
        config.mode = LayoutConfig::Mode::MatchSize;
        LayoutSnapshot snapAll = engine.matchSize(items, &sA, config);
        QCOMPARE(snapAll[0].newSize, QSizeF(120.0, 80.0));
        QCOMPARE(snapAll[0].newPos, QPointF(20.0, 70.0));

        // 调用 applyLayoutSnapshot 真正应用修改
        bool applied = LayoutEngine::applyLayoutSnapshot(snapAll);
        QVERIFY(applied);
        QCOMPARE(sB.getSize(), QSizeF(120.0, 80.0));
        QCOMPARE(sB.pos(), QPointF(20.0, 70.0));
    }

    void testApplyLayoutSnapshotTransactionalRollback() {
        RectShape s1(QPointF(0.0, 0.0), QSizeF(50.0, 50.0));
        RectShape s2(QPointF(100.0, 100.0), QSizeF(50.0, 50.0));

        LayoutSnapshot snapshot;
        snapshot.append({&s1, s1.pos(), QPointF(10.0, 10.0), s1.getSize(), QSizeF(80.0, 80.0)});
        snapshot.append({&s2, s2.pos(), QPointF(200.0, 200.0), s2.getSize(), QSizeF(100.0, 100.0)});

        // 模拟 s2 被锁定，此时应用快照应当触发预校验或事务回滚
        s2.setLocked(true);
        bool applied = LayoutEngine::applyLayoutSnapshot(snapshot);
        QVERIFY(!applied);
        // 验证 s1 保持原有位置和尺寸，没有被部分执行所污染
        QCOMPARE(s1.pos(), QPointF(0.0, 0.0));
        QCOMPARE(s1.getSize(), QSizeF(50.0, 50.0));

        s2.setLocked(false);
        // 测试快照中包含已经析构的悬空 QPointer 的处理情况
        snapshot.clear();
        snapshot.append({&s1, s1.pos(), QPointF(20.0, 20.0), s1.getSize(), QSizeF(90.0, 90.0)});
        {
            RectShape tempShape(QPointF(0.0, 0.0), QSizeF(10.0, 10.0));
            snapshot.append({&tempShape, tempShape.pos(), QPointF(30.0, 30.0), tempShape.getSize(), tempShape.getSize()});
        } // tempShape 离开作用域被销毁，快照中的 QPointer 应自动置空

        QVERIFY(!LayoutEngine::applyLayoutSnapshot(snapshot));
        // s1 同样应保持未变，被原子回滚处理保护
        QCOMPARE(s1.pos(), QPointF(0.0, 0.0));
        QCOMPARE(s1.getSize(), QSizeF(50.0, 50.0));
    }

    void testAlignHorizontalAndVerticalLockedShapeCursorMax() {
        LayoutEngine engine;
        LayoutConfig cfg;
        cfg.spacing = 20.0;

        // 1. 锁定的图元位于当前排布游标左侧 (倒退场景)
        RectShape s1(QPointF(100.0, 0.0), QSizeF(50.0, 50.0));
        RectShape sLockedLeft(QPointF(-200.0, 0.0), QSizeF(50.0, 50.0));
        sLockedLeft.setLocked(true);
        RectShape s2(QPointF(300.0, 0.0), QSizeF(50.0, 50.0));

        LayoutSnapshot snapH1 = engine.alignHorizontal({&s1, &sLockedLeft, &s2}, nullptr, cfg);
        // s1 处理完后 lastXPos = 100 + 50 + 20 = 170
        // 遇到 sLockedLeft (x=-200)，无 std::max 会倒退到 -130；经过 std::max 维持在 170
        QCOMPARE(snapH1.size(), 1);
        QCOMPARE(snapH1[0].shape.data(), &s2);
        QCOMPARE(snapH1[0].newPos.x(), 170.0);

        // 2. 锁定的图元位于当前排布游标右方较远处 (前方障碍场景)
        RectShape sLockedRight(QPointF(250.0, 0.0), QSizeF(50.0, 50.0));
        sLockedRight.setLocked(true);
        LayoutSnapshot snapH2 = engine.alignHorizontal({&s1, &sLockedRight, &s2}, nullptr, cfg);
        // s1 处理完后 lastXPos = 170
        // 遇到 sLockedRight (x=250)，经过 std::max 跳跃至 250 + 50 + 20 = 320
        QCOMPARE(snapH2.size(), 1);
        QCOMPARE(snapH2[0].shape.data(), &s2);
        QCOMPARE(snapH2[0].newPos.x(), 320.0);
    }

    void testArrangeGridPrimaryAnchorFixed() {
        LayoutEngine engine;
        QList<Shape*> items;
        RectShape s0(QPointF(0.0, 0.0), QSizeF(50.0, 50.0));
        RectShape s1(QPointF(100.0, 0.0), QSizeF(50.0, 50.0));
        RectShape s2(QPointF(350.0, 450.0), QSizeF(50.0, 50.0)); // 选择 s2 作为网格的真正锚定基准 (primary)
        RectShape s3(QPointF(300.0, 0.0), QSizeF(50.0, 50.0));
        items << &s0 << &s1 << &s2 << &s3;

        LayoutSnapshot snap = engine.arrangeGrid(items, &s2);
        // 验证快照中任何移动的图元都不会移动 primary (s2) 本身，即 primary 图元所在单元格严格锚固于 (350, 450)
        for (const auto& change : snap) {
            QVERIFY(change.shape != &s2);
        }
    }

    void testOptionalLayoutConfigOverrideAndSanitization() {
        LayoutEngine engine;
        RectShape s1(QPointF(0.0, 0.0), QSizeF(50.0, 50.0));
        RectShape s2(QPointF(100.0, 0.0), QSizeF(50.0, 50.0));

        // 传参时不设置 mode，仅通过 optional 传入自定义间距 80.0
        LayoutConfig overrideConfig;
        overrideConfig.spacing = 80.0;
        LayoutSnapshot snap = engine.alignHorizontal({&s1, &s2}, nullptr, overrideConfig);
        QCOMPARE(snap.size(), 1);
        QCOMPARE(snap[0].newPos.x(), 0.0 + 50.0 + 80.0); // 130.0

        // 测试网格列数过大时的钳制安全保护
        LayoutConfig clampConfig;
        clampConfig.gridCols = 999999;
        LayoutSnapshot gridSnap = engine.arrangeGrid({&s1, &s2}, nullptr, clampConfig);
        QVERIFY(gridSnap.size() <= 2); // 应平稳运行无超大内存越界
    }

    // 测试运行时执行中途失败时的逆序事务回滚 (先尺寸后位置 & 当前图元已修改部分彻底清退)
    void testApplyLayoutSnapshotRuntimeFailureRollback() {
        RectShape s1(0.0, 0.0, 50.0, 50.0);
        FailingShape s2(QPointF(100.0, 100.0), QSizeF(50.0, 50.0));
        s2.failOnSetPosition = true;

        LayoutSnapshot snapshot;
        // 尝试修改 s1 的尺寸与位置
        snapshot.append({&s1, QPointF(0.0, 0.0), QPointF(20.0, 20.0), QSizeF(50.0, 50.0), QSizeF(80.0, 80.0)});
        // 尝试修改 s2 的尺寸与位置：在真正调用 setPosition 时必败，但在此之前的 setSize(90, 90) 会先执行成功并改变 s2 状态
        snapshot.append({&s2, QPointF(100.0, 100.0), QPointF(200.0, 200.0), QSizeF(50.0, 50.0), QSizeF(90.0, 90.0)});

        bool applied = LayoutEngine::applyLayoutSnapshot(snapshot);
        QVERIFY(!applied);

        // 验证由于 s2 的 setPosition 失败，s2 已经执行成功的 setSize 连同 s1 被完全逆序回滚为最初值！
        QCOMPARE(s1.pos(), QPointF(0.0, 0.0));
        QCOMPARE(s1.getSize(), QSizeF(50.0, 50.0));
        QCOMPARE(s2.pos(), QPointF(100.0, 100.0));
        QCOMPARE(s2.getSize(), QSizeF(50.0, 50.0));
    }

    // 测试可布局能力模型判断与过滤 (支持性过滤及混合图元处理)
    void testShapeLayoutCapabilitiesAndFiltering() {
        RectShape rect(0.0, 0.0, 50.0, 50.0);
        LineShape line(QPointF(0.0, 0.0), QPointF(50.0, 50.0));
        Connector conn(QPointF(0.0, 0.0), QPointF(100.0, 100.0));

        QVERIFY(rect.supportsLayoutPosition() && rect.supportsLayoutSize());
        QVERIFY(line.supportsLayoutPosition() && !line.supportsLayoutSize());
        QVERIFY(!conn.supportsLayoutPosition() && !conn.supportsLayoutSize());

        LayoutEngine engine;
        QList<Shape*> mixedItems = { &rect, &line, &conn };

        // 1. matchSize 应当把 line 和 conn 过滤掉，不生成也不修改它们的尺寸快照
        LayoutSnapshot sizeSnap = engine.matchSize(mixedItems, &rect);
        for (const auto& item : sizeSnap) {
            QVERIFY(item.shape != &line && item.shape != &conn);
        }

        // 2. alignHorizontal 应当把 conn (不支持平移) 自动过滤
        LayoutSnapshot posSnap = engine.alignHorizontal(mixedItems, &rect);
        for (const auto& item : posSnap) {
            QVERIFY(item.shape != &conn);
        }
    }

    // 测试 LayoutEngine 构造函数与 setConfig 保存已清洗后的合法配置
    void testSanitizedConfigPersistence() {
        LayoutConfig rawCfg;
        rawCfg.spacing = -100.0; // 非法负数应在初始化时就被清洗为 -1.0
        rawCfg.gridHorizontalSpacing = std::numeric_limits<qreal>::quiet_NaN(); // NaN 应清洗为 -1.0

        LayoutEngine engine(rawCfg);
        QCOMPARE(engine.config().spacing, -1.0);
        QCOMPARE(engine.config().gridHorizontalSpacing, -1.0);

        rawCfg.spacing = -50.0;
        engine.setConfig(rawCfg);
        QCOMPARE(engine.config().spacing, -1.0);
    }
};

QTEST_MAIN(LayoutEngineTest)
#include "layout_engine_test.moc"
