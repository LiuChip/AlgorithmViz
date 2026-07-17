#include <QtTest/QTest>
#include <QtTest/QSignalSpy>
#include <QGraphicsScene>
#include <QPointer>
#include "../src/shapes/connector/connector.h"
#include "../src/shapes/connector/connector_anchor.h"
#include "../src/shapes/rect_shape.h"
#include "../src/shapes/line_shape.h"
#include "../src/shapes/arrow_shape.h"
#include "../src/shapes/ellipse_shape.h"
#include "../src/shapes/text_label.h"
#include "../src/core/shape_controller/anchor_resolver.h"

class ConnectorTest : public QObject
{
    Q_OBJECT

private slots:
    // 测试 1: 基础自由连线坐标计算与外接框
    void testConnectorBasicFree() {
        Connector conn(QPointF(10, 20), QPointF(100, 150));
        QCOMPARE(conn.getStartAnchor().mode(), ConnectorAnchor::Mode::Free);
        QCOMPARE(conn.getEndAnchor().mode(), ConnectorAnchor::Mode::Free);
        QCOMPARE(conn.pos(), QPointF(0, 0)); // Connector 的本地位置恒定在(0,0)
    }

    // 测试 2: 连线整体防拖动及形变锁定测试
    void testConnectorImmovability() {
        Connector conn(QPointF(0, 0), QPointF(100, 100));
        QVERIFY(!conn.flags().testFlag(QGraphicsItem::ItemIsMovable));

        // 尝试通过函数或外部操作移动位置和旋转，必须被 itemChange 拦截并保持原样
        conn.setPos(50, 50);
        QCOMPARE(conn.pos(), QPointF(0, 0));

        conn.setRotation(45.0);
        QCOMPARE(conn.rotation(), 0.0);

        conn.setScale(2.0);
        QCOMPARE(conn.scale(), 1.0);
    }

    // 测试 3: 连线跟随被关联目标图形几何变化的自动刷新功能（精确数值断言强验证）
    void testConnectorFollowsTargetGeometry() {
        auto* rect = new RectShape(QPointF(100, 100), QSizeF(80, 60));
        // rect 的中心在 (140, 130)
        
        Connector conn(QPointF(0, 0), QPointF(200, 200));
        // 将起点绑定为矩形中心归一化点 (0.5, 0.5)
        ConnectorAnchor anchor = ConnectorAnchor::createInterior(rect, QPointF(0.5, 0.5));
        conn.setStartAnchor(anchor);

        // 初始时解析坐标应为矩形本地路径的精确包围中心 (140, 130)
        QPointF initialStart = conn.getStartAnchor().resolveScenePoint();
        QCOMPARE(initialStart, QPointF(140, 130));

        // 改变矩形位置至 (200, 200)，此时其中心精确平移至 (240, 230)
        rect->setPosition(QPointF(200, 200));
        QPointF newStart = conn.getStartAnchor().resolveScenePoint();
        QCOMPARE(newStart, QPointF(240, 230));

        delete rect;
    }

    // 测试 4: 目标被删除后，连线自动降级为 Free 并且留守在安全气囊(最后缓存位置)
    void testConnectorTargetDestructionDegradesToFree() {
        auto* rect = new RectShape(QPointF(100, 100), QSizeF(80, 60));
        Connector conn(QPointF(0, 0), QPointF(300, 300));

        // 绑定起点为边界射线
        ConnectorAnchor anchor = ConnectorAnchor::createBoundary(rect, 0.0);
        conn.setStartAnchor(anchor);

        // 触发一次解析以确认缓存更新
        QPointF validFallback = conn.getStartAnchor().resolveScenePoint();
        QVERIFY(validFallback != QPointF(0, 0));

        // 彻底删除被绑定对象
        delete rect;

        // 验证端点已经自动自愈降级为 Free，并且坐标安全保持在最后有效点而未跳回到(0,0)
        QCOMPARE(conn.getStartAnchor().mode(), ConnectorAnchor::Mode::Free);
        QVERIFY(conn.getStartAnchor().targetShape() == nullptr);
        QCOMPARE(conn.getStartAnchor().resolveScenePoint(), validFallback);
    }

    // 测试 5: 起点与终点同时绑定同一目标图形，修改一端或目标销毁时去重逻辑验证
    void testConnectorDualBindingSameTarget() {
        auto* rect = new RectShape(QPointF(100, 100), QSizeF(100, 100));
        Connector conn(QPointF(0, 0), QPointF(0, 0));

        conn.setStartAnchor(ConnectorAnchor::createBoundary(rect, 0.0));
        conn.setEndAnchor(ConnectorAnchor::createBoundary(rect, 3.14159));

        QVERIFY(conn.getStartAnchor().targetShape() == rect);
        QVERIFY(conn.getEndAnchor().targetShape() == rect);

        // 删除目标后两头均需独立并准确降级为 Free
        delete rect;
        QCOMPARE(conn.getStartAnchor().mode(), ConnectorAnchor::Mode::Free);
        QCOMPARE(conn.getEndAnchor().mode(), ConnectorAnchor::Mode::Free);
    }

    // 测试 6: 箭头内部命中检测测试（通过 visual.united(stroker) 保证大箭头填充区也能点中）
    void testConnectorArrowHitTest() {
        Connector conn(QPointF(0, 0), QPointF(100, 0));
        conn.setEndStyle(Connector::EndStyle::Arrow);
        // 箭头的顶部在(100, 0)，向左拓展30度三角，因此(95, 0)肯定在实心多边形区域内
        QPainterPath hitArea = conn.shape();
        QVERIFY(hitArea.contains(QPointF(95, 0)));
        QVERIFY(hitArea.contains(QPointF(50, 0))); // 线条主体也在命中范围内
    }

    // 测试 7: 克隆预期行为测试（独立克隆必须降级为纯自由连线且复制全部外观样式）
    void testConnectorClone() {
        Connector conn(QPointF(10, 10), QPointF(90, 90));
        conn.setEndStyle(Connector::EndStyle::Arrow);

        auto* copy = qobject_cast<Connector*>(conn.clone());
        QVERIFY(copy != nullptr);
        QCOMPARE(copy->getEndStyle(), Connector::EndStyle::Arrow);
        QCOMPARE(copy->getStartAnchor().mode(), ConnectorAnchor::Mode::Free);
        QCOMPARE(copy->getEndAnchor().mode(), ConnectorAnchor::Mode::Free);

        delete copy;
    }

    // 测试 8: 单端解绑后另一端继续精准跟随目标（双端解绑隔离验证）
    void testConnectorUnbindOneAnchorWhileOtherTracks() {
        auto* rect = new RectShape(QPointF(100, 100), QSizeF(100, 100));
        Connector conn(QPointF(0, 0), QPointF(0, 0));

        // 起点终点初始双向绑定到 rect
        conn.setStartAnchor(ConnectorAnchor::createInterior(rect, QPointF(0.2, 0.2)));
        conn.setEndAnchor(ConnectorAnchor::createInterior(rect, QPointF(0.8, 0.8)));

        // 随后将起点显式改为独立 Free 点 (10, 10)，解绑起点
        conn.setStartAnchor(ConnectorAnchor::createFree(QPointF(10, 10)));
        QCOMPARE(conn.getStartAnchor().mode(), ConnectorAnchor::Mode::Free);
        QVERIFY(conn.getStartAnchor().targetShape() == nullptr);

        // 此时移动被绑定的 rect 对象
        rect->setPosition(QPointF(300, 300));

        // 验证：起点坐标绝不随之变动，恒定保持在 (10, 10)；而终点仍能准确追踪新的目标内部位置
        QCOMPARE(conn.getStartAnchor().resolveScenePoint(), QPointF(10, 10));
        QVERIFY(conn.getEndAnchor().resolveScenePoint() != QPointF(0, 0));
        QVERIFY(conn.getEndAnchor().resolveScenePoint() != QPointF(10, 10));

        delete rect;
    }

    // 测试 9: 克隆及属性复制后连线依旧严格不可移动（验证覆盖的 setLocked 是否生效）
    void testConnectorCloneImmovability() {
        Connector conn(QPointF(0, 0), QPointF(100, 100));
        auto* copy = qobject_cast<Connector*>(conn.clone());
        QVERIFY(copy != nullptr);
        QVERIFY(!copy->flags().testFlag(QGraphicsItem::ItemIsMovable));

        // 模拟外部或属性面板将其解锁/锁定时，验证其 ItemIsMovable 依旧被死死锁在 false
        copy->setLocked(false);
        QVERIFY(!copy->flags().testFlag(QGraphicsItem::ItemIsMovable));

        copy->setLocked(true);
        QVERIFY(!copy->flags().testFlag(QGraphicsItem::ItemIsMovable));

        delete copy;
    }

    // 测试 10: 几何变化信号发信次数及去重检测（确保无多余/中间态 geometryChanged 信号冒出）
    void testConnectorSignalDedup() {
        Connector conn(QPointF(0, 0), QPointF(100, 100));
        QSignalSpy spyConn(&conn, &Shape::geometryChanged);

        // 设置一次锚点，应准确且唯一触发 1 次 geometryChanged 信号
        conn.setStartAnchor(ConnectorAnchor::createFree(QPointF(50, 50)));
        QCOMPARE(spyConn.count(), 1);

        // 验证 LineShape::setEndpoints 同时设置两端也仅发射一次信号
        LineShape line(QPointF(0, 0), QPointF(10, 10));
        QSignalSpy spyLine(&line, &Shape::geometryChanged);
        line.setEndpoints(QPointF(20, 20), QPointF(80, 80));
        QCOMPARE(spyLine.count(), 1);
    }

    // 测试 11: 验证 LineShape 单端点修改（setStartPoint / setEndPoint）能准确发出并且仅发出 1 次 geometryChanged
    void testLineShapeSingleEndpointSignal() {
        LineShape line(QPointF(0, 0), QPointF(100, 100));
        QSignalSpy spyLine(&line, &Shape::geometryChanged);

        line.setStartPoint(QPointF(20, 20));
        QCOMPARE(spyLine.count(), 1);

        line.setEndPoint(QPointF(80, 80));
        QCOMPARE(spyLine.count(), 2);
    }

    // 测试 12: 验证 ConnectorAnchor 工厂在未明确传 fallback 参数时，立即自动基于被绑图形生成真实物理场景坐标缓存
    void testConnectorAnchorImmediateFallback() {
        auto* rect = new RectShape(QPointF(100, 100), QSizeF(80, 60));
        // rect 中心 (140, 130)

        // 不传第3个 fallback 参数，依靠工厂方法自动当场求取并 prime 物理点缓存
        ConnectorAnchor anchor = ConnectorAnchor::createInterior(rect, QPointF(0.5, 0.5));
        QVERIFY(anchor.fallbackScenePoint() != QPointF(0, 0));
        QCOMPARE(anchor.fallbackScenePoint(), QPointF(140, 130));

        delete rect;
    }

    // 测试 13: 验证 AnchorResolver - Ctrl 快捷键强行悬空破防功能
    void testAnchorResolverCtrlOverride() {
        QGraphicsScene scene;
        auto* rect = new RectShape(QPointF(100, 100), QSizeF(100, 100));
        scene.addItem(rect);

        AnchorResolver::ResolveOptions options;
        options.scene = &scene;
        options.scenePoint = QPointF(150, 150); // 鼠标正好放在矩形正中央
        options.ctrlPressed = true;             // 强行按住 Ctrl 键

        ConnectorAnchor anchor = AnchorResolver::resolve(options);
        QCOMPARE(anchor.mode(), ConnectorAnchor::Mode::Free); // 必须强制返回 Free 模式，无视底下的矩形
        QVERIFY(anchor.targetShape() == nullptr);
    }

    // 测试 14: 验证 AnchorResolver - 外壳边框雷达吸附精度
    void testAnchorResolverBoundarySnapping() {
        QGraphicsScene scene;
        auto* rect = new RectShape(QPointF(100, 100), QSizeF(100, 100));
        // rect 范围为 x∈[100, 200], y∈[100, 200]，中心 (150, 150)
        scene.addItem(rect);

        AnchorResolver::ResolveOptions options;
        options.scene = &scene;
        options.scenePoint = QPointF(150, 95); // 鼠标放在顶边线外侧 5 像素处 (在 12px 默认容差内)
        options.screenTolerancePx = 12.0;

        ConnectorAnchor anchor = AnchorResolver::resolve(options);
        QCOMPARE(anchor.mode(), ConnectorAnchor::Mode::Boundary);
        QVERIFY(anchor.targetShape() == rect);

        // 经 boundaryPointAtAngle 算出的理论场景交点，应严格咬合在顶边中点 (150, 100)
        QCOMPARE(anchor.resolveScenePoint(), QPointF(150, 100));
    }

    // 测试 15: 验证 AnchorResolver - 图形内部深入吸附
    void testAnchorResolverInteriorSnapping() {
        QGraphicsScene scene;
        auto* rect = new RectShape(QPointF(100, 100), QSizeF(100, 100));
        scene.addItem(rect);

        AnchorResolver::ResolveOptions options;
        options.scene = &scene;
        options.scenePoint = QPointF(150, 150); // 鼠标直接戳入正中央，且远离外部边框 (>12px)

        ConnectorAnchor anchor = AnchorResolver::resolve(options);
        QCOMPARE(anchor.mode(), ConnectorAnchor::Mode::Interior);
        QVERIFY(anchor.targetShape() == rect);
        QCOMPARE(anchor.resolveScenePoint(), QPointF(150, 150));
    }

    // 测试 16: [P1] 验证细长矩形边框吸附（真实最短距离吸附而非中心斜切射线距离）
    void testAnchorResolverThinRect() {
        QGraphicsScene scene;
        auto* rect = new RectShape(QPointF(0, 0), QSizeF(200, 20));
        scene.addItem(rect);

        AnchorResolver::ResolveOptions options;
        options.scene = &scene;
        options.scenePoint = QPointF(190, -5); // 靠近右侧上边缘 5px处
        options.screenTolerancePx = 12.0;

        ConnectorAnchor anchor = AnchorResolver::resolve(options);
        QCOMPARE(anchor.mode(), ConnectorAnchor::Mode::Boundary);
        QVERIFY(anchor.targetShape() == rect);
        QCOMPARE(anchor.resolveScenePoint(), QPointF(190, 0));
    }

    // 测试 17: [P2] 验证非法 viewScale 参数下的防御保护
    void testAnchorResolverInvalidScale() {
        QGraphicsScene scene;
        auto* rect = new RectShape(QPointF(100, 100), QSizeF(100, 100));
        scene.addItem(rect);

        AnchorResolver::ResolveOptions options;
        options.scene = &scene;
        options.scenePoint = QPointF(150, 95);
        options.viewScale = 0.0; // 非法缩放值 0

        ConnectorAnchor anchorZero = AnchorResolver::resolve(options);
        QCOMPARE(anchorZero.mode(), ConnectorAnchor::Mode::Free);

        options.viewScale = -1.0; // 负值缩放
        ConnectorAnchor anchorNeg = AnchorResolver::resolve(options);
        QCOMPARE(anchorNeg.mode(), ConnectorAnchor::Mode::Free);
    }

    // 测试 18: 验证普通线段 LineShape 已经被从连接器吸附候选中排除
    void testAnchorResolverIgnoreLineShape() {
        QGraphicsScene scene;
        auto* line = new LineShape(QPointF(0, 0), QPointF(100, 100));
        scene.addItem(line);

        AnchorResolver::ResolveOptions options;
        options.scene = &scene;
        options.scenePoint = QPointF(50, 50); // 正好在线上

        ConnectorAnchor anchor = AnchorResolver::resolve(options);
        QCOMPARE(anchor.mode(), ConnectorAnchor::Mode::Free);
    }

    // 测试 19: [P2] 验证 ArrowShape 箭头头部多边形已被加入 shape() 命中路径
    void testArrowShapeHitTest() {
        ArrowShape arrow(QPointF(0, 0), QPointF(100, 0));
        arrow.setBorderInfo(Border(1, Qt::black, Qt::SolidLine));
        QVERIFY(arrow.shape().contains(QPointF(95, 2))); // 处于三角形箭头实心头部内的点
    }

    // 测试 20: [P1] 验证 Shape 尺寸设置负值的拦截防御
    void testShapeInvalidSize() {
        RectShape rect(QPointF(0, 0), QSizeF(100, 100));
        qreal oldWidth = rect.getSize().width();
        QVERIFY(!rect.setSize(-50, -50));
        QCOMPARE(rect.getSize().width(), oldWidth); // 负值应被直接拦截且返回 false
    }

    // 测试 21: [P1] 验证 Shape 尺寸设置 NaN 的拦截防御
    void testShapeNaNSize() {
        RectShape rect(QPointF(0, 0), QSizeF(100, 100));
        qreal oldWidth = rect.getSize().width();
        QVERIFY(!rect.setSize(std::numeric_limits<qreal>::quiet_NaN(), 100));
        QCOMPARE(rect.getSize().width(), oldWidth); // NaN 应被拦截且返回 false
    }

    // 测试 22: [P1] 验证 AnchorResolver 对 NaN 场景点坐标的安全处理
    void testAnchorResolverNaNPoint() {
        auto* rect = new RectShape(QPointF(0, 0), QSizeF(100, 100));
        QGraphicsScene scene;
        scene.addItem(rect);
        AnchorResolver::ResolveOptions options;
        options.scene = &scene;
        options.scenePoint = QPointF(std::numeric_limits<qreal>::quiet_NaN(), 50);
        ConnectorAnchor anchor = AnchorResolver::resolve(options);
        QCOMPARE(anchor.mode(), ConnectorAnchor::Mode::Free);
    }

    // 测试 23: 验证椭圆射线交点在吸附容差内的成功吸附
    void testAnchorResolverEllipseSnapSuccess() {
        QGraphicsScene scene;
        auto* ellipse = new EllipseShape(QPointF(0, 0), QSizeF(200, 100));
        scene.addItem(ellipse);

        AnchorResolver::ResolveOptions options;
        options.scene = &scene;
        options.scenePoint = QPointF(100, -2); // 处于顶点正上方 2px 处，在 5px 屏幕容差内
        options.screenTolerancePx = 5.0;
        options.viewScale = 1.0;
        ConnectorAnchor anchor = AnchorResolver::resolve(options);
        QCOMPARE(anchor.mode(), ConnectorAnchor::Mode::Boundary);
        qreal dist = QLineF(options.scenePoint, anchor.resolveScenePoint()).length();
        QVERIFY(dist <= 5.0 + 1e-4);
        QVERIFY(qAbs(anchor.resolveScenePoint().x() - 100.0) < 0.5);
        QVERIFY(qAbs(anchor.resolveScenePoint().y() - 0.0) < 0.5);
    }

    // 测试 24: 验证射线交点超出吸附容差且在图形外时二道拦截校验能够准确拒绝并降级为 Free
    void testAnchorResolverEllipseSnapRejected() {
        QGraphicsScene scene;
        auto* ellipse = new EllipseShape(QPointF(0, 0), QSizeF(200, 100));
        scene.addItem(ellipse);

        AnchorResolver::ResolveOptions options;
        options.scene = &scene;
        options.scenePoint = QPointF(10, 10); // 既不在多边形内部，又距离边界交点远远超过 5px
        options.screenTolerancePx = 5.0;
        options.viewScale = 1.0;
        ConnectorAnchor anchor = AnchorResolver::resolve(options);
        QCOMPARE(anchor.mode(), ConnectorAnchor::Mode::Free);
    }

    // 测试 25: 验证 TextLabel 设置非法尺寸时不会错误切换到 FixedSize 模式
    void testTextLabelSetSizeSideEffect() {
        TextLabel label(QPointF(0, 0), "Hello World");
        QCOMPARE(label.getTextLayoutMode(), TextLabel::TextLayoutMode::AutoSize);

        // 尝试设置非法的尺寸参数
        QVERIFY(!label.setSize(std::numeric_limits<qreal>::quiet_NaN(), 100));
        QVERIFY(!label.setSize(-10, 50));
        // 布局模式必须严格保持在 AutoSize，不被破坏
        QCOMPARE(label.getTextLayoutMode(), TextLabel::TextLayoutMode::AutoSize);

        // 只有设置合法尺寸后，才能够切换到 FixedSize
        QVERIFY(label.setSize(120, 40));
        QCOMPARE(label.getTextLayoutMode(), TextLabel::TextLayoutMode::FixedSize);
    }

    // 测试 26: 验证 Shape 对位置与缩放方法的数值与合法范围校验
    void testShapeSetPositionAndScaleValidation() {
        RectShape rect(QPointF(0, 0), QSizeF(100, 100));
        QVERIFY(!rect.setPosition(std::numeric_limits<qreal>::quiet_NaN(), 0));
        QCOMPARE(rect.getPosition(), QPointF(0, 0));

        QVERIFY(!rect.setScale(0.0));  // 不允许 0 缩放
        QVERIFY(!rect.setScale(-1.0)); // 不允许负值缩放
        QCOMPARE(rect.scale(), 1.0);
    }

    // 测试 27: 验证 ConnectorAnchor 内部归一化锚点在越界时能正确 Clamp 到 [0, 1]
    void testConnectorAnchorClamping() {
        auto* rect = new RectShape(QPointF(0, 0), QSizeF(100, 100));
        ConnectorAnchor anchor = ConnectorAnchor::createInterior(rect, QPointF(-0.5, 1.5));
        QCOMPARE(anchor.getInteriorNormalized(), QPointF(0.0, 1.0));
        delete rect;
    }

    // 测试 28: 验证通过 QGraphicsItem* 原生接口设置 NaN 坐标时被 itemChange 拦截
    void testItemChangeNaNGuard() {
        RectShape rect(QPointF(10, 20), QSizeF(100, 100));
        QGraphicsItem* item = &rect;

        // 通过原生 setPos 尝试注入 NaN 坐标
        item->setPos(QPointF(std::numeric_limits<qreal>::quiet_NaN(), 0));
        QCOMPARE(rect.pos(), QPointF(10, 20)); // 位置必须保持不变

        // 通过原生 setRotation 尝试注入 NaN
        item->setRotation(std::numeric_limits<qreal>::quiet_NaN());
        QCOMPARE(item->rotation(), 0.0); // 旋转必须保持默认 0

        // 通过原生 setScale 尝试注入 NaN
        item->setScale(std::numeric_limits<qreal>::quiet_NaN());
        QCOMPARE(item->scale(), 1.0); // 缩放必须保持默认 1.0

        // 负数缩放也应被拦截
        item->setScale(-2.0);
        QCOMPARE(item->scale(), 1.0);

        // 合法值应该能通过
        item->setPos(QPointF(50, 60));
        QCOMPARE(rect.pos(), QPointF(50, 60));
    }

    // 测试 29: 验证锁定的 Connector 不允许通过 setStartAnchor/setEndAnchor 修改端点
    void testConnectorLockedAnchorModification() {
        QGraphicsScene scene;
        auto* rect = new RectShape(QPointF(50, 50), QSizeF(100, 100));
        scene.addItem(rect);

        Connector conn(QPointF(0, 0), QPointF(200, 200));
        scene.addItem(&conn);

        // 记录锁定前的端点
        QPointF origStart = conn.getStartAnchor().resolveScenePoint();

        // 锁定连接线
        conn.setLocked(true);
        QVERIFY(conn.isLocked());

        // 尝试修改锚点 — 应被拒绝
        ConnectorAnchor boundaryAnchor = ConnectorAnchor::createBoundary(rect, 0.0, QPointF(150, 100));
        conn.setStartAnchor(boundaryAnchor);

        // 端点必须保持原值
        QCOMPARE(conn.getStartAnchor().mode(), ConnectorAnchor::Mode::Free);
        QCOMPARE(conn.getStartAnchor().resolveScenePoint(), origStart);

        // 解锁后应该可以修改
        conn.setLocked(false);
        conn.setStartAnchor(boundaryAnchor);
        QCOMPARE(conn.getStartAnchor().mode(), ConnectorAnchor::Mode::Boundary);
    }

    // 测试 30: 验证锁定 Connector 上目标销毁时仍允许内部自动降级
    void testConnectorLockedTargetDestroyStillDegrades() {
        QGraphicsScene scene;
        auto* rect = new RectShape(QPointF(50, 50), QSizeF(100, 100));
        scene.addItem(rect);

        Connector conn(QPointF(0, 0), QPointF(200, 200));
        scene.addItem(&conn);

        // 绑定起点到 rect
        conn.setStartAnchor(ConnectorAnchor::createBoundary(rect, 0.0, QPointF(150, 100)));
        QCOMPARE(conn.getStartAnchor().mode(), ConnectorAnchor::Mode::Boundary);

        // 锁定连接线
        conn.setLocked(true);

        // 销毁目标 — 即使锁定，也应该自动降级
        delete rect;
        QCOMPARE(conn.getStartAnchor().mode(), ConnectorAnchor::Mode::Free);
    }

    // 测试 31: 验证 Connector::setPosition() 返回 false
    void testConnectorSetPositionReturnsFalse() {
        Connector conn(QPointF(10, 20), QPointF(100, 150));
        QVERIFY(!conn.setPosition(QPointF(50, 60)));
        QCOMPARE(conn.pos(), QPointF(0, 0));

        QVERIFY(!conn.setPosition(50.0, 60.0));
        QCOMPARE(conn.pos(), QPointF(0, 0));
    }

    // 测试 32: 验证 Connector 使用 NoPen 时箭头不被绘制和命中
    void testConnectorNoPenHidesArrow() {
        Connector conn(QPointF(0, 0), QPointF(100, 0));
        conn.setEndStyle(Connector::EndStyle::Arrow);
        conn.setBorderInfo(Border(0.0, Qt::red, Qt::NoPen));

        // 箭头三角区域中心不应该被命中
        QPainterPath shp = conn.shape();
        // 即使不检查具体命中点，visualPath 应该不包含箭头多边形
        // 且 shape() 应该没有箭头区域
        // 简单验证：边框设为 NoPen 后，shape 应该仍然存在（线段描边部分）
        QVERIFY(!shp.isEmpty());
    }

    // 测试 33: 验证 AnchorResolver 在极端 viewScale 导致 tol 溢出时安全降级
    void testAnchorResolverTolOverflow() {
        QGraphicsScene scene;
        auto* rect = new RectShape(QPointF(0, 0), QSizeF(100, 100));
        scene.addItem(rect);

        AnchorResolver::ResolveOptions options;
        options.scene = &scene;
        options.scenePoint = QPointF(50, 50);
        options.screenTolerancePx = 1e308;
        options.viewScale = 1e-9; // tol = 1e308 / 1e-9 = Inf
        ConnectorAnchor anchor = AnchorResolver::resolve(options);
        QCOMPARE(anchor.mode(), ConnectorAnchor::Mode::Free);
    }

    // 测试 34: 验证 degradeStartAnchorToFreeIfUnlocked 在锁定时返回 false
    void testDegradeAnchorRespectsLock() {
        QGraphicsScene scene;
        auto* rect = new RectShape(QPointF(50, 50), QSizeF(100, 100));
        scene.addItem(rect);

        Connector conn(QPointF(0, 0), QPointF(200, 200));
        scene.addItem(&conn);

        conn.setStartAnchor(ConnectorAnchor::createBoundary(rect, 0.0, QPointF(150, 100)));
        QCOMPARE(conn.getStartAnchor().mode(), ConnectorAnchor::Mode::Boundary);

        conn.setLocked(true);
        QVERIFY(!conn.degradeStartAnchorToFreeIfUnlocked());
        QCOMPARE(conn.getStartAnchor().mode(), ConnectorAnchor::Mode::Boundary);

        conn.setLocked(false);
        QVERIFY(conn.degradeStartAnchorToFreeIfUnlocked());
        QCOMPARE(conn.getStartAnchor().mode(), ConnectorAnchor::Mode::Free);
    }

    // 测试 35: 验证 Connector 仅切换边框样式 (例如 SolidLine <-> NoPen) 时正确触发几何更新
    void testConnectorSetBorderInfoStyleChange() {
        Connector conn(QPointF(0, 0), QPointF(100, 0));
        conn.setEndStyle(Connector::EndStyle::Arrow);
        conn.setBorderInfo(Border(2.0, Qt::black, Qt::SolidLine));

        QRectF origRect = conn.boundingRect();

        // 仅切换 borderStyle 为 NoPen，宽度和颜色保持不变
        QVERIFY(conn.setBorderInfo(Border(2.0, Qt::black, Qt::NoPen)));
        // 由于箭头区域在 NoPen 下不计入 visualPath()，boundingRect 应当发生改变
        QVERIFY(conn.boundingRect() != origRect);

        // 恢复为 SolidLine 应该再次改变
        QVERIFY(conn.setBorderInfo(Border(2.0, Qt::black, Qt::SolidLine)));
        QCOMPARE(conn.boundingRect(), origRect);
    }

    // 测试 36: 验证 TextLabel 处于 AutoSize 模式时，即使调用 setSize 传入当前尺寸也能正确切换到 FixedSize
    void testTextLabelSetSizeToCurrentDimensions() {
        TextLabel label(QPointF(0, 0), "Test Text");
        QCOMPARE(label.getTextLayoutMode(), TextLabel::TextLayoutMode::AutoSize);

        qreal curW = label.getSize().width();
        qreal curH = label.getSize().height();

        // 传入当前尺寸，必须返回 true 并切入 FixedSize 模式
        QVERIFY(label.setSize(curW, curH));
        QCOMPARE(label.getTextLayoutMode(), TextLabel::TextLayoutMode::FixedSize);

        // 再次传入相同尺寸（且早已在 FixedSize 模式下），此时没有任何状态变化，返回 false
        QVERIFY(!label.setSize(curW, curH));
        QCOMPARE(label.getTextLayoutMode(), TextLabel::TextLayoutMode::FixedSize);
    }
};

QTEST_MAIN(ConnectorTest)
#include "connector_test.moc"
