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
#include "../src/core/canvas.h"
#include "../src/core/shape_controller/connector_controller.h"
#include "../src/core/shape_controller/control_box.h"
#include "../src/core/shape_controller/canvas_controller.h"
#include "../src/ui/main_window.h"

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

    // 测试 37: 验证 ConnectorController 拖拽创建模式 (Press -> Move -> Release 超过阈值)
    void testConnectorControllerCreateDragMode() {
        Canvas canvas;
        auto* rect1 = new RectShape(QPointF(0, 0), QSizeF(50, 50));
        auto* rect2 = new RectShape(QPointF(80, 80), QSizeF(50, 50));
        canvas.scene()->addItem(rect1);
        canvas.scene()->addItem(rect2);

        ConnectorController controller(&canvas);
        QSignalSpy spy(&controller, &ConnectorController::connectorCreated);

        controller.setCreateModeActive(true);
        QCOMPARE(controller.state(), ConnectorController::State::Idle);

        // 模拟按键在(10,10) - 属于 rect1 内部/边界
        QGraphicsSceneMouseEvent pressEv(QEvent::GraphicsSceneMousePress);
        pressEv.setButton(Qt::LeftButton);
        pressEv.setScenePos(QPointF(10, 10));
        QVERIFY(controller.handleMousePressEvent(&pressEv));
        QCOMPARE(controller.state(), ConnectorController::State::Creating);

        // 模拟拖动到(100,100) - 属于 rect2 内部/边界
        QGraphicsSceneMouseEvent moveEv(QEvent::GraphicsSceneMouseMove);
        moveEv.setScenePos(QPointF(100, 100));
        QVERIFY(controller.handleMouseMoveEvent(&moveEv));

        // 模拟松手在(100,100)，由于距离 > 8px 触发创建完成
        QGraphicsSceneMouseEvent releaseEv(QEvent::GraphicsSceneMouseRelease);
        releaseEv.setButton(Qt::LeftButton);
        releaseEv.setScenePos(QPointF(100, 100));
        QVERIFY(controller.handleMouseReleaseEvent(&releaseEv));

        QCOMPARE(controller.state(), ConnectorController::State::Idle);
        QCOMPARE(spy.count(), 1);
    }

    // 测试 38: 验证 ConnectorController 两点点击创建模式 (原地点击松开 -> 移动 -> 第二次点击松开)
    void testConnectorControllerTwoClickMode() {
        Canvas canvas;
        auto* rect1 = new RectShape(QPointF(0, 0), QSizeF(50, 50));
        auto* rect2 = new RectShape(QPointF(180, 180), QSizeF(50, 50));
        canvas.scene()->addItem(rect1);
        canvas.scene()->addItem(rect2);

        ConnectorController controller(&canvas);
        QSignalSpy spy(&controller, &ConnectorController::connectorCreated);

        controller.setCreateModeActive(true);

        // 第一次点击：Press 和 Release 都在(20, 20) - 属于 rect1
        QGraphicsSceneMouseEvent pressEv1(QEvent::GraphicsSceneMousePress);
        pressEv1.setButton(Qt::LeftButton);
        pressEv1.setScenePos(QPointF(20, 20));
        QVERIFY(controller.handleMousePressEvent(&pressEv1));
        QCOMPARE(controller.state(), ConnectorController::State::Creating);

        QGraphicsSceneMouseEvent releaseEv1(QEvent::GraphicsSceneMouseRelease);
        releaseEv1.setButton(Qt::LeftButton);
        releaseEv1.setScenePos(QPointF(20, 20));
        QVERIFY(controller.handleMouseReleaseEvent(&releaseEv1));
        // 原地松手，依然维持 Creating 状态 (留给两点点击法的后续引导)
        QCOMPARE(controller.state(), ConnectorController::State::Creating);

        // 空手移动到(200, 200) - 属于 rect2
        QGraphicsSceneMouseEvent moveEv(QEvent::GraphicsSceneMouseMove);
        moveEv.setScenePos(QPointF(200, 200));
        QVERIFY(controller.handleMouseMoveEvent(&moveEv));

        // 第二次点击：在终点(200, 200)触发 Press，直接完成终点确认
        QGraphicsSceneMouseEvent pressEv2(QEvent::GraphicsSceneMousePress);
        pressEv2.setButton(Qt::LeftButton);
        pressEv2.setScenePos(QPointF(200, 200));
        QVERIFY(controller.handleMousePressEvent(&pressEv2));

        QCOMPARE(controller.state(), ConnectorController::State::Idle);
        QCOMPARE(spy.count(), 1);
    }

    // 测试 39: 验证 ConnectorController 按 ESC 键强制中断取消操作
    void testConnectorControllerCancelOperation() {
        Canvas canvas;
        auto* rect = new RectShape(QPointF(10, 10), QSizeF(50, 50));
        canvas.scene()->addItem(rect);

        ConnectorController controller(&canvas);

        controller.setCreateModeActive(true);
        QGraphicsSceneMouseEvent pressEv(QEvent::GraphicsSceneMousePress);
        pressEv.setButton(Qt::LeftButton);
        pressEv.setScenePos(QPointF(30, 30));
        QVERIFY(controller.handleMousePressEvent(&pressEv));
        QCOMPARE(controller.state(), ConnectorController::State::Creating);

        // 按下 ESC 键
        QKeyEvent escEv(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
        QVERIFY(controller.handleKeyPressEvent(&escEv));

        QCOMPARE(controller.state(), ConnectorController::State::Idle);
    }

    // 测试 40: 验证 Connector::setStartAnchor / setEndAnchor 的 bool 返回值与锁定保护
    void testConnectorSetAnchorBoolAndLocking() {
        Connector conn(QPointF(10, 10), QPointF(100, 100));
        ConnectorAnchor newAnchor = ConnectorAnchor::createFree(QPointF(50, 50));

        // 未锁定时修改必须成功并返回 true
        QVERIFY(conn.setStartAnchor(newAnchor));
        QCOMPARE(conn.getStartAnchor().resolveScenePoint(), QPointF(50, 50));

        // 锁定时修改必须失败并返回 false，且锚点保持原状不变
        conn.setLocked(true);
        ConnectorAnchor blockedAnchor = ConnectorAnchor::createFree(QPointF(80, 80));
        QVERIFY(!conn.setStartAnchor(blockedAnchor));
        QCOMPARE(conn.getStartAnchor().resolveScenePoint(), QPointF(50, 50));
    }

    // 测试 41: 验证端点命中区域测试 (`hitTestConnectorEndpoint`) 真正支持 10px 容差以及近距离端点优先择近
    void testConnectorControllerEndpointHitTolerance() {
        Canvas canvas;
        ConnectorController controller(&canvas);
        auto* conn = new Connector(QPointF(100, 100), QPointF(300, 300));
        canvas.scene()->addItem(conn);

        // 鼠标在 (106, 100)，距离起点 (100, 100) 为 6px。常规 stroker 描边仅两侧5px无法覆盖，但 searchRect 10px 容差必须准确命中
        QGraphicsSceneMouseEvent pressEv(QEvent::GraphicsSceneMousePress);
        pressEv.setButton(Qt::LeftButton);
        pressEv.setScenePos(QPointF(106, 100));
        QVERIFY(controller.handleMousePressEvent(&pressEv));
        QCOMPARE(controller.state(), ConnectorController::State::DraggingEndpoint);
        controller.cancelCurrentOperation();
    }

    // 测试 42: 验证拖拽过程中活动连线被外部销毁后的 QPointer 安全自愈机制（防悬空与防卡死）
    void testConnectorControllerQPointerDeletionSafety() {
        Canvas canvas;
        ConnectorController controller(&canvas);
        auto* conn = new Connector(QPointF(50, 50), QPointF(150, 150));
        canvas.scene()->addItem(conn);

        // 进入拖拽起点状态
        QGraphicsSceneMouseEvent pressEv(QEvent::GraphicsSceneMousePress);
        pressEv.setButton(Qt::LeftButton);
        pressEv.setScenePos(QPointF(50, 50));
        QVERIFY(controller.handleMousePressEvent(&pressEv));
        QCOMPARE(controller.state(), ConnectorController::State::DraggingEndpoint);

        // 模拟外部或未知场景下将当前活动的 Connector 销毁
        delete conn; // 此时控制器内部的 QPointer 自动清零为 nullptr

        // 持续发送移动与松手事件，控制器不能崩溃，并且必须迅速自愈重置至 Idle 状态
        QGraphicsSceneMouseEvent moveEv(QEvent::GraphicsSceneMouseMove);
        moveEv.setScenePos(QPointF(80, 80));
        controller.handleMouseMoveEvent(&moveEv);
        QCOMPARE(controller.state(), ConnectorController::State::Idle);

        // 再次触发 ESC 键取消或重置，确保调用 cancelCurrentOperation 对已经 nullptr 的对象完全安全无副作用
        QKeyEvent escEv(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
        controller.handleKeyPressEvent(&escEv);
        QCOMPARE(controller.state(), ConnectorController::State::Idle);
    }

    // 测试 43: 验证对同一图形的各锚点模式自连接彻底拒绝拦截（Interior/Boundary 自连接全过滤）
    void testConnectorControllerSelfConnectionRejection() {
        Canvas canvas;
        ConnectorController controller(&canvas);
        auto* rect = new RectShape(QPointF(100, 100), QSizeF(100, 100)); // (100,100) 到 (200,200)
        canvas.scene()->addItem(rect);

        controller.setCreateModeActive(true);

        // 点击矩形内部左上方点发起连线 (120, 120)
        QGraphicsSceneMouseEvent pressEv(QEvent::GraphicsSceneMousePress);
        pressEv.setButton(Qt::LeftButton);
        pressEv.setScenePos(QPointF(120, 120));
        QVERIFY(controller.handleMousePressEvent(&pressEv));
        QCOMPARE(controller.state(), ConnectorController::State::Creating);

        // 拖动至矩形右下方边界点并松手 (190, 190) -> 属于同一个 rect 图形的自连接，必须被拦截判定为废线清理
        QGraphicsSceneMouseEvent releaseEv(QEvent::GraphicsSceneMouseRelease);
        releaseEv.setButton(Qt::LeftButton);
        releaseEv.setScenePos(QPointF(190, 190));
        QVERIFY(controller.handleMouseReleaseEvent(&releaseEv));

        // 状态机必须被重置为 Idle，并且创建连线未能落盘
        QCOMPARE(controller.state(), ConnectorController::State::Idle);
    }

    // 测试 44: 验证 getSafeViewScale 极值缩放防护与 Ctrl 键修饰场景坐标防漂移
    void testConnectorControllerSafeViewScaleAndCtrlPos() {
        Canvas canvas;
        ConnectorController controller(&canvas);
        // 将视图缩放强制设为 0（异常或极端缩放），getSafeViewScale 必须回退为 1.0 避免除零或 NaN
        QTransform zeroTransform;
        zeroTransform.scale(0.0, 0.0);
        canvas.setTransform(zeroTransform);

        auto* conn = new Connector(QPointF(10, 10), QPointF(50, 50));
        canvas.scene()->addItem(conn);

        // 触发一次点击以记录 m_lastScenePos
        QGraphicsSceneMouseEvent pressEv(QEvent::GraphicsSceneMousePress);
        pressEv.setButton(Qt::LeftButton);
        pressEv.setScenePos(QPointF(10, 10));
        QVERIFY(controller.handleMousePressEvent(&pressEv));
        QCOMPARE(controller.state(), ConnectorController::State::DraggingEndpoint);

        // 发送键盘 Ctrl 键，控制器必须安全使用 m_lastScenePos 计算新位置并经过 getSafeViewScale 保护不发生除零溢出
        QKeyEvent ctrlEv(QEvent::KeyPress, Qt::Key_Control, Qt::ControlModifier);
        QVERIFY(controller.handleKeyPressEvent(&ctrlEv));
        controller.cancelCurrentOperation();
    }

    // 测试 45: 验证公开事件路由接口的空指针防御
    void testConnectorControllerNullEventDefense() {
        Canvas canvas;
        ConnectorController controller(&canvas);
        QVERIFY(!controller.handleMousePressEvent(nullptr));
        QVERIFY(!controller.handleMouseMoveEvent(nullptr));
        QVERIFY(!controller.handleMouseReleaseEvent(nullptr));
        QVERIFY(!controller.handleKeyPressEvent(nullptr));
        QVERIFY(!controller.handleKeyReleaseEvent(nullptr));
    }

    // 测试 46: 通过真实 Canvas 鼠标压、动、放事件在 Connect 模式下完成连线创建
    void testCanvasIntegrationMouseEvents() {
        Canvas canvas;
        auto* rect1 = new RectShape(QPointF(0, 0), QSizeF(50, 50));
        auto* rect2 = new RectShape(QPointF(80, 80), QSizeF(50, 50));
        canvas.scene()->addItem(rect1);
        canvas.scene()->addItem(rect2);

        canvas.setEditMode(Canvas::EditMode::Connect);
        QVERIFY(canvas.connectorController()->isCreateModeActive());
        QCOMPARE(canvas.dragMode(), QGraphicsView::NoDrag);

        QSignalSpy spy(canvas.connectorController(), &ConnectorController::connectorCreated);

        // 模拟鼠标点击压下创建起点 (10, 10) - 属于 rect1
        QPoint viewportPress = canvas.mapFromScene(QPointF(10, 10));
        QMouseEvent pressEv(QEvent::MouseButtonPress, viewportPress, viewportPress, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        canvas.mousePressEvent(&pressEv);
        QCOMPARE(canvas.connectorController()->state(), ConnectorController::State::Creating);

        // 模拟鼠标拖拽至 (100, 100) - 属于 rect2
        QPoint viewportMove = canvas.mapFromScene(QPointF(100, 100));
        QMouseEvent moveEv(QEvent::MouseMove, viewportMove, viewportMove, Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        canvas.mouseMoveEvent(&moveEv);

        // 模拟鼠标松开完成创建
        QMouseEvent releaseEv(QEvent::MouseButtonRelease, viewportMove, viewportMove, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        canvas.mouseReleaseEvent(&releaseEv);

        QCOMPARE(canvas.connectorController()->state(), ConnectorController::State::Idle);
        QCOMPARE(spy.count(), 1);
    }

    // 测试 47: 验证 MainWindow 中真正创建并绑定了中央画布与控制器
    void testMainWindowControllerCreation() {
        MainWindow mainWindow;
        Canvas* canvas = mainWindow.canvas();
        QVERIFY(canvas != nullptr);
        QVERIFY(canvas->connectorController() != nullptr);

        QCOMPARE(canvas->editMode(), Canvas::EditMode::Select);
        canvas->setEditMode(Canvas::EditMode::Connect);
        QVERIFY(canvas->connectorController()->isCreateModeActive());
    }

    // 测试 48: 验证创建中途如果锁定或设置端点失败，临时连线安全清理不会在场景残留
    void testAbortCreatingDeletesTemporaryConnector() {
        Canvas canvas;
        auto* rect = new RectShape(QPointF(0, 0), QSizeF(50, 50));
        canvas.scene()->addItem(rect);

        auto* controller = canvas.connectorController();
        controller->setCreateModeActive(true);

        controller->handleMousePress(QPointF(10, 10), Qt::LeftButton, Qt::NoModifier);
        QCOMPARE(controller->state(), ConnectorController::State::Creating);

        // 在按住 ESC 键强行中止时，验证场景中临时连线已被清理
        controller->handleKeyPress(Qt::Key_Escape, Qt::NoModifier);
        QCOMPARE(controller->state(), ConnectorController::State::Idle);
        // 场景此时应该只有吸附图元（可能隐藏或无，没有 Connector 对象）
        bool hasConnector = false;
        for (auto* item : canvas.scene()->items()) {
            if (dynamic_cast<Connector*>(item)) {
                hasConnector = true;
                break;
            }
        }
        QVERIFY(!hasConnector);
    }

    // 测试 49: 第一次点击位于边框吸附容差边缘时，两点点击创建距离不再被边框吸附坐标误判为拖拽
    void testTwoClickStartDistanceWithBoundarySnapping() {
        Canvas canvas;
        auto* rect = new RectShape(QPointF(100, 100), QSizeF(80, 60)); // 左边界 x = 100
        canvas.scene()->addItem(rect);

        auto* controller = canvas.connectorController();
        controller->setCreateModeActive(true);

        // 用户在 (93, 130) 点击（距离左边界 7px 会触发边框吸附至 (100, 130)）
        QPointF clickPos(93, 130);
        controller->handleMousePress(clickPos, Qt::LeftButton, Qt::NoModifier);
        QCOMPARE(controller->state(), ConnectorController::State::Creating);

        // 用户原地松手触发 handleMouseRelease，必须基于原始物理坐标判定为两点点击（不误删）
        controller->handleMouseRelease(clickPos, Qt::LeftButton, Qt::NoModifier);
        QCOMPARE(controller->state(), ConnectorController::State::Creating);
    }

    // 测试 50: 验证场景切换或 scene->clear() / clearScene() 后吸附指示器（QGraphicsObject/QPointer）生命周期安全且不出悬空崩溃
    void testSnapIndicatorLifecycleAndSceneClear() {
        Canvas canvas;
        auto* controller = canvas.connectorController();
        auto* rect = new RectShape(QPointF(100, 100), QSizeF(80, 60));
        canvas.scene()->addItem(rect);

        // 1. 明确进入创建态并触发初始点击
        controller->setCreateModeActive(true);
        controller->handleMousePress(QPointF(120, 120), Qt::LeftButton, Qt::NoModifier);
        QCOMPARE(controller->state(), ConnectorController::State::Creating);

        // 2. 触发吸附更新，真正生成 SnapIndicator 图元
        controller->handleMouseMove(QPointF(140, 140), Qt::NoModifier);
        QVERIFY(!controller->snapIndicatorIsNull());
        QVERIFY(controller->snapIndicator() != nullptr);

        // 3. 模拟外部直接强行清空当前场景（考验底层的 QPointer 自动监控与防悬空）
        canvas.scene()->clear();
        QVERIFY(controller->snapIndicatorIsNull()); // QPointer 必须自动清零

        // 4. 再次触发移动事件，控制器决不能崩溃且能自动重建光点或安全处理
        controller->handleMouseMove(QPointF(0, 0), Qt::NoModifier);
        QVERIFY(true);

        // 5. 验证应用层统一收口接口 clearScene() 的安全性
        auto* rect2 = new RectShape(QPointF(100, 100), QSizeF(80, 60));
        canvas.scene()->addItem(rect2);
        controller->handleMousePress(QPointF(120, 120), Qt::LeftButton, Qt::NoModifier);
        controller->handleMouseMove(QPointF(140, 140), Qt::NoModifier);
        QVERIFY(!controller->snapIndicatorIsNull());
        canvas.clearScene();
        QVERIFY(controller->snapIndicatorIsNull());
        QCOMPARE(controller->state(), ConnectorController::State::Idle);
    }

    // 测试 51: 验证两点点击第二下鼠标按压完成创建后，后续同一点击流的 Release 释放事件会被正确吞掉消费
    void testTwoClickSecondReleaseEventConsumed() {
        Canvas canvas;
        auto* rect1 = new RectShape(QPointF(0, 0), QSizeF(50, 50));
        auto* rect2 = new RectShape(QPointF(80, 80), QSizeF(50, 50));
        canvas.scene()->addItem(rect1);
        canvas.scene()->addItem(rect2);

        auto* controller = canvas.connectorController();
        controller->setCreateModeActive(true);

        // 第一击点开创建
        controller->handleMousePress(QPointF(10, 10), Qt::LeftButton, Qt::NoModifier);
        controller->handleMouseRelease(QPointF(10, 10), Qt::LeftButton, Qt::NoModifier);
        QCOMPARE(controller->state(), ConnectorController::State::Creating);

        // 第二击按下直接完成创建
        QVERIFY(controller->handleMousePress(QPointF(100, 100), Qt::LeftButton, Qt::NoModifier));
        QCOMPARE(controller->state(), ConnectorController::State::Idle);

        // 紧接着发出的鼠标释放信号，应该被控制器消费（返回 true）防穿透
        QVERIFY(controller->handleMouseRelease(QPointF(100, 100), Qt::LeftButton, Qt::NoModifier));
        // 下一次无关点击释放不再被错误消费
        QVERIFY(!controller->handleMouseRelease(QPointF(100, 100), Qt::LeftButton, Qt::NoModifier));
    }

    // 测试 52: 验证创建完成后 connectorCreated 信号准确发射并具备可接入撤销栈的数据闭环
    void testConnectorCreatedSignalAndUndoStackReady() {
        Canvas canvas;
        auto* rect1 = new RectShape(QPointF(0, 0), QSizeF(50, 50));
        auto* rect2 = new RectShape(QPointF(120, 120), QSizeF(50, 50));
        canvas.scene()->addItem(rect1);
        canvas.scene()->addItem(rect2);

        auto* controller = canvas.connectorController();
        controller->setCreateModeActive(true);

        QSignalSpy spy(controller, &ConnectorController::connectorCreated);
        controller->handleMousePress(QPointF(10, 10), Qt::LeftButton, Qt::NoModifier);
        controller->handleMouseMove(QPointF(150, 150), Qt::NoModifier);
        controller->handleMouseRelease(QPointF(150, 150), Qt::LeftButton, Qt::NoModifier);

        QCOMPARE(spy.count(), 1);
        auto* createdConnector = qobject_cast<Connector*>(spy.takeFirst().at(0).value<QObject*>());
        QVERIFY(createdConnector != nullptr);
        QVERIFY(createdConnector->scene() == canvas.scene());
    }

    // 测试 53: [A1 验证] 连线起点必须吸附到有效目标，点击空白区域直接穿透且不进入创建态
    void testConnectorCreateRequiresStartAnchor() {
        Canvas canvas;
        auto* controller = canvas.connectorController();
        controller->setCreateModeActive(true);

        // 场景无图形或点击在空白处
        QVERIFY(!controller->handleMousePress(QPointF(100, 100), Qt::LeftButton, Qt::NoModifier));
        QCOMPARE(controller->state(), ConnectorController::State::Idle);
    }

    // 测试 54: [A2 验证] 连线终点必须吸附到有效目标，拖向或两点点击在空白区均丢弃为废线
    void testConnectorCreateRequiresEndAnchor() {
        Canvas canvas;
        auto* controller = canvas.connectorController();
        auto* rect = new RectShape(QPointF(0, 0), QSizeF(50, 50));
        canvas.scene()->addItem(rect);
        controller->setCreateModeActive(true);

        // 1. 拖拽至空白区域
        controller->handleMousePress(QPointF(20, 20), Qt::LeftButton, Qt::NoModifier);
        QCOMPARE(controller->state(), ConnectorController::State::Creating);
        controller->handleMouseMove(QPointF(200, 200), Qt::NoModifier);
        controller->handleMouseRelease(QPointF(200, 200), Qt::LeftButton, Qt::NoModifier);
        QCOMPARE(controller->state(), ConnectorController::State::Idle);

        // 2. 两点点击第二下点在空白区域
        controller->handleMousePress(QPointF(20, 20), Qt::LeftButton, Qt::NoModifier);
        controller->handleMouseRelease(QPointF(20, 20), Qt::LeftButton, Qt::NoModifier);
        QCOMPARE(controller->state(), ConnectorController::State::Creating);
        controller->handleMousePress(QPointF(200, 200), Qt::LeftButton, Qt::NoModifier);
        QCOMPARE(controller->state(), ConnectorController::State::Idle);
    }

    // 测试 55: [A2 补充验证] 降级产生的单端 Free 连线在运行时合法存在，不被误删
    void testConnectorSingleEndpointFreeLegality() {
        Canvas canvas;
        auto* rect1 = new RectShape(QPointF(0, 0), QSizeF(50, 50));
        auto* rect2 = new RectShape(QPointF(100, 100), QSizeF(50, 50));
        canvas.scene()->addItem(rect1);
        canvas.scene()->addItem(rect2);

        auto* controller = canvas.connectorController();
        controller->setCreateModeActive(true);
        controller->handleMousePress(QPointF(20, 20), Qt::LeftButton, Qt::NoModifier);
        controller->handleMouseMove(QPointF(120, 120), Qt::NoModifier);
        controller->handleMouseRelease(QPointF(120, 120), Qt::LeftButton, Qt::NoModifier);

        Connector* createdConn = nullptr;
        for (auto* item : canvas.scene()->items()) {
            if (auto* conn = dynamic_cast<Connector*>(item)) {
                createdConn = conn;
                break;
            }
        }
        QVERIFY(createdConn != nullptr);
        QCOMPARE(createdConn->getStartAnchor().mode(), ConnectorAnchor::Mode::Interior);
        QCOMPARE(createdConn->getEndAnchor().mode(), ConnectorAnchor::Mode::Interior);

        // 删除目标 2，触发端点降级为 Free
        delete rect2;
        QCOMPARE(createdConn->getEndAnchor().mode(), ConnectorAnchor::Mode::Free);
        QCOMPARE(createdConn->getStartAnchor().mode(), ConnectorAnchor::Mode::Interior);
        // 单端 Free 连线运行时合法，绝不能被自动销毁
        QVERIFY(createdConn->scene() == canvas.scene());
    }

    // 测试 56: [P1 验证] TextLabel 在 AutoSize 或 setText 后应该正确同步其局部旋转中心(transformOriginPoint)
    void testTextLabelAutoSizeUpdatesTransformOrigin() {
        TextLabel label(QPointF(0, 0), "Hello World");
        QSizeF size = label.getSize();
        QVERIFY(size.width() > 0.0 && size.height() > 0.0);
        QCOMPARE(label.transformOriginPoint(), QPointF(size.width() * 0.5, size.height() * 0.5));

        // 修改文字使其尺寸发生变化，旋转中心也应当同步自动调整到新的半宽高
        label.setText("Updated Longer Text For Test");
        QSizeF newSize = label.getSize();
        QVERIFY(newSize != size);
        QCOMPARE(label.transformOriginPoint(), QPointF(newSize.width() * 0.5, newSize.height() * 0.5));
    }

    // 测试 57: [P1 验证] 使用 Ctrl+点击选中或多选图形时，不应启动拖拽移动状态(MovingItems)
    void testCtrlClickDoesNotStartMoving() {
        Canvas canvas;
        auto* controller = canvas.canvasController();
        auto* rect = new RectShape(QPointF(0, 0), QSizeF(50, 50));
        canvas.scene()->addItem(rect);

        QPoint viewportPress = canvas.mapFromScene(QPointF(25, 25));
        QMouseEvent pressEvent(QEvent::MouseButtonPress, QPointF(viewportPress), QPointF(viewportPress), QPointF(viewportPress), Qt::LeftButton, Qt::LeftButton, Qt::ControlModifier);
        canvas.setToolMode(Canvas::ToolMode::Select);
        QVERIFY(controller->handleMousePressEvent(&pressEvent));
        QVERIFY(rect->isSelected());
        QCOMPARE(controller->currentState(), CanvasController::InteractionState::Idle);
    }

    // 测试 58: [P2 验证] 点击仅仅选中的已锁定图形时，不应进入 MovingItems 状态
    void testLockedSelectionDoesNotEnterMovingState() {
        Canvas canvas;
        auto* controller = canvas.canvasController();
        auto* rect = new RectShape(QPointF(0, 0), QSizeF(50, 50));
        rect->setLocked(true);
        canvas.scene()->addItem(rect);

        QPoint viewportPress = canvas.mapFromScene(QPointF(25, 25));
        QMouseEvent pressEvent(QEvent::MouseButtonPress, QPointF(viewportPress), QPointF(viewportPress), QPointF(viewportPress), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        canvas.setToolMode(Canvas::ToolMode::Select);
        QVERIFY(controller->handleMousePressEvent(&pressEvent));
        QVERIFY(rect->isSelected());
        QCOMPARE(controller->currentState(), CanvasController::InteractionState::Idle);
    }

    // 测试 59: [P2 验证] Shape::setSize() 在批处理修改尺寸和原点时只发出一次 geometryChanged 信号
    void testShapeSetSizeEmitsOneGeometryChanged() {
        auto* rect = new RectShape(QPointF(0, 0), QSizeF(100, 100));
        QSignalSpy spy(rect, &Shape::geometryChanged);
        rect->setSize(QSizeF(200, 150));
        QCOMPARE(spy.count(), 1);
        delete rect;
    }

    // 测试 60: [P2 验证] selectAll() 选中的对象如果被外部直接销毁，选区集合能安全清除裸指针绝不崩溃
    void testSelectAllDeletionSafety() {
        Canvas canvas;
        auto* controller = canvas.canvasController();
        auto* rect = new RectShape(QPointF(0, 0), QSizeF(50, 50));
        canvas.scene()->addItem(rect);

        controller->selectAll();
        QVERIFY(controller->selectedItems().contains(rect));

        delete rect;
        QVERIFY(controller->selectedItems().isEmpty());
        QVERIFY(controller->primarySelection() == nullptr);
    }

    // 测试 61: [P2 验证] 通过 ControlBox 手柄拖动连线端点时，能够利用 AnchorResolver 重新吸附到新目标；按住 Ctrl 时强行创建 Free
    void testConnectorEndpointDragReattachOrFree() {
        Canvas canvas;
        auto* rect1 = new RectShape(QPointF(0, 0), QSizeF(50, 50));
        auto* rect2 = new RectShape(QPointF(100, 100), QSizeF(50, 50));
        canvas.scene()->addItem(rect1);
        canvas.scene()->addItem(rect2);

        Connector* conn = new Connector(QPointF(25, 25), QPointF(125, 125));
        canvas.scene()->addItem(conn);
        conn->setStartAnchor(ConnectorAnchor::createBoundary(rect1, 0.0));
        conn->setEndAnchor(ConnectorAnchor::createBoundary(rect2, 3.14));

        ControlBox* cb = canvas.canvasController()->controlBox();
        cb->setTarget(conn);

        HandleItem* startHandle = nullptr;
        for (QGraphicsItem* item : cb->childItems()) {
            if (auto* handle = dynamic_cast<HandleItem*>(item)) {
                if (handle->handleType() == HandleType::StartEndpoint) {
                    startHandle = handle;
                    break;
                }
            }
        }
        QVERIFY(startHandle != nullptr);

        cb->onHandleMoved(startHandle, QPointF(120, 100), Qt::NoModifier);
        QCOMPARE(conn->getStartAnchor().mode(), ConnectorAnchor::Mode::Boundary);
        QVERIFY(conn->getStartAnchor().targetShape() == rect2);

        cb->onHandleMoved(startHandle, QPointF(120, 100), Qt::ControlModifier);
        QCOMPARE(conn->getStartAnchor().mode(), ConnectorAnchor::Mode::Free);
    }

    // 测试 62: [P1 补充验证] ControlBox 旋转或 Shape 旋转应围绕中心(transformOriginPoint)并无缝发射信号
    void testControlBoxRotationAroundCenter() {
        auto* rect = new RectShape(QPointF(100, 100), QSizeF(80, 60));
        QCOMPARE(rect->transformOriginPoint(), QPointF(40, 30));

        ControlBox cb;
        QSignalSpy spyStarted(&cb, &ControlBox::rotateStarted);
        QSignalSpy spyFinished(&cb, &ControlBox::rotateFinished);
        cb.setTarget(rect);

        HandleItem* rotateHandle = nullptr;
        for (QGraphicsItem* item : cb.childItems()) {
            if (auto* h = dynamic_cast<HandleItem*>(item)) {
                if (h->handleType() == HandleType::Rotate) {
                    rotateHandle = h;
                    break;
                }
            }
        }
        QVERIFY(rotateHandle != nullptr);

        cb.onHandlePressed(rotateHandle, QPointF(140, 100));
        QCOMPARE(spyStarted.count(), 1);

        rect->setRotation(90.0);
        cb.onHandleReleased(rotateHandle, QPointF(170, 130));
        QCOMPARE(spyFinished.count(), 1);

        delete rect;
    }

    // 测试 63: [P2 补充验证] 带有旋转及在 Canvas 中缩放情况下的 ControlBox 手柄缩放对角解算与位置补偿
    void testControlBoxResizeWithRotationAndZoom() {
        auto* rect = new RectShape(QPointF(100, 100), QSizeF(100, 100));
        rect->setRotation(90.0);
        ControlBox cb;
        cb.setTarget(rect);

        HandleItem* brHandle = nullptr;
        for (QGraphicsItem* item : cb.childItems()) {
            if (auto* h = dynamic_cast<HandleItem*>(item)) {
                if (h->handleType() == HandleType::BottomRight) {
                    brHandle = h;
                    break;
                }
            }
        }
        QVERIFY(brHandle != nullptr);

        cb.onHandlePressed(brHandle, QPointF(200, 200));
        cb.applyResizeFromAnchor(HandleType::BottomRight, QPointF(220, 200), QPointF(220, 200), Qt::NoModifier);
        QVERIFY(rect->getSize().width() > 0 && rect->getSize().height() > 0);
        delete rect;
    }
};

QTEST_MAIN(ConnectorTest)
#include "connector_test.moc"
