#ifndef CANVAS_H
#define CANVAS_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QPointer>

#include "shape_controller/canvas_controller.h"
#include "shape_controller/connector_controller.h"
#include "undo_manager.h"

class Canvas : public QGraphicsView
{
    Q_OBJECT
    friend class ConnectorTest;

public:
    // 1. 工具模式扩展 (涵盖原 EditMode，并新增具体图元创建模式)
    enum class ToolMode {
        Select,          // 选择/移动/改变大小/旋转模式
        Connect,         // 连线模式 (等同于通用连线工具)
        CreateRect,      // 创建矩形
        CreateEllipse,   // 创建椭圆
        CreateDiamond,   // 创建菱形
        CreateLine,      // 创建直线
        CreateArrow,     // 创建单箭头直线
        CreateDualArrow, // 创建双箭头直线
        CreateText       // 创建文本标签
    };
    // 兼容性类型别名：保证既有代码及单测对 EditMode::Select / Connect 的引用依然有效
    using EditMode = ToolMode;

    explicit Canvas(QWidget *parent = nullptr);
    ~Canvas() override;

    // 访问器
    QGraphicsScene *scene() const { return m_scene; }
    ConnectorController *connectorController() const { return m_connectorController; }
    CanvasController *canvasController() const { return m_canvasController; }
    UndoManager *undoManager() const { return m_undoManager; }

    ToolMode toolMode() const { return m_toolMode; }
    EditMode editMode() const { return m_toolMode; } // 兼容接口

    qreal zoomScale() const { return m_zoomScale; }
    bool isLineCreationMode() const; // 辅助判断当前是否处于任何线工具模式下

    void clearScene();

public slots:
    // 2. 模式与视图控制 Slots
    void setToolMode(ToolMode mode);
    void setEditMode(EditMode mode) { setToolMode(mode); } // 兼容槽

    void zoomIn();
    void zoomOut();
    void resetZoom();
    void fitToScene();
    void fitToSelection();

    // 3. 业务指令槽 (对外对接 UI 菜单与快捷键分发)
    void undo() { if (m_undoManager) m_undoManager->undo(); }
    void redo() { if (m_undoManager) m_undoManager->redo(); }
    void copy() { if (m_canvasController) m_canvasController->copy(); }
    void paste() { if (m_canvasController) m_canvasController->paste(); }
    void cut() { if (m_canvasController) m_canvasController->cut(); }
    void deleteSelected() { if (m_canvasController) m_canvasController->deleteSelected(); }
    void selectAll() { if (m_canvasController) m_canvasController->selectAll(); }

signals:
    // 4. 对外状态广播信号
    void toolModeChanged(Canvas::ToolMode mode);
    void editModeChanged(Canvas::EditMode mode); // 兼容信号
    void zoomScaleChanged(qreal scale);
    void cursorScenePositionChanged(const QPointF &pos);
    void selectionChanged();

protected:
    // 5. 重载事件以拦截与分流
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void applyZoom(qreal scaleFactor, const QPoint &viewportAnchor);

    QGraphicsScene *m_scene = nullptr;
    ConnectorController *m_connectorController = nullptr;
    CanvasController *m_canvasController = nullptr;
    UndoManager *m_undoManager = nullptr;

    ToolMode m_toolMode = ToolMode::Select;
    qreal m_zoomScale = 1.0;

    // 平移画布相关状态
    bool m_isPanning = false;
    QPoint m_lastPanPoint;
};

#endif // CANVAS_H