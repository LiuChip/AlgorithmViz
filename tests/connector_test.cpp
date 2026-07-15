#include <QtTest/QTest>
#include <QtTest/QSignalSpy>
#include <QGraphicsScene>
#include <QPointer>
#include "../src/shapes/connector/connector.h"
#include "../src/shapes/connector/connector_anchor.h"
#include "../src/shapes/rect_shape.h"
#include "../src/shapes/line_shape.h"

class ConnectorTest : public QObject
{
    Q_OBJECT

private slots:
    // 测试 1: 基础自由连线坐标计算与外接框
    void testConnectorBasicFree() {
        Connector conn(QPointF(10, 20), QPointF(100, 150));
        QCOMPARE(conn.getStartAnchor().mode, ConnectorAnchor::Mode::Free);
        QCOMPARE(conn.getEndAnchor().mode, ConnectorAnchor::Mode::Free);
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
        QCOMPARE(conn.getStartAnchor().mode, ConnectorAnchor::Mode::Free);
        QVERIFY(conn.getStartAnchor().targetShape.isNull());
        QCOMPARE(conn.getStartAnchor().resolveScenePoint(), validFallback);
    }

    // 测试 5: 起点与终点同时绑定同一目标图形，修改一端或目标销毁时去重逻辑验证
    void testConnectorDualBindingSameTarget() {
        auto* rect = new RectShape(QPointF(100, 100), QSizeF(100, 100));
        Connector conn(QPointF(0, 0), QPointF(0, 0));

        conn.setStartAnchor(ConnectorAnchor::createBoundary(rect, 0.0));
        conn.setEndAnchor(ConnectorAnchor::createBoundary(rect, 3.14159));

        QVERIFY(conn.getStartAnchor().targetShape == rect);
        QVERIFY(conn.getEndAnchor().targetShape == rect);

        // 删除目标后两头均需独立并准确降级为 Free
        delete rect;
        QCOMPARE(conn.getStartAnchor().mode, ConnectorAnchor::Mode::Free);
        QCOMPARE(conn.getEndAnchor().mode, ConnectorAnchor::Mode::Free);
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
        QCOMPARE(copy->getStartAnchor().mode, ConnectorAnchor::Mode::Free);
        QCOMPARE(copy->getEndAnchor().mode, ConnectorAnchor::Mode::Free);

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
        QCOMPARE(conn.getStartAnchor().mode, ConnectorAnchor::Mode::Free);
        QVERIFY(conn.getStartAnchor().targetShape.isNull());

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
        QVERIFY(anchor.fallbackScenePoint != QPointF(0, 0));
        QCOMPARE(anchor.fallbackScenePoint, QPointF(140, 130));

        delete rect;
    }
};

QTEST_MAIN(ConnectorTest)
#include "connector_test.moc"
