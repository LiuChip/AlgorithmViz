#ifndef TOOLBAR_WIDGET_H
#define TOOLBAR_WIDGET_H

#include <QWidget>
#include <QButtonGroup>
#include <QString>

enum class ToolType {
    Select = 0,
    BoxSelect,
    Line,
    Arrow,
    DualArrow,
    Rectangle,
    Circle,
    Diamond,
    Text
};

class ToolBarWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ToolBarWidget(QWidget *parent = nullptr);
    ~ToolBarWidget() override;

    ToolType currentTool() const;
    void setCurrentTool(ToolType tool);

    static QString toolName(ToolType tool);

signals:
    void toolChanged(ToolType tool);
    void toolClicked(ToolType tool);

private:
    QButtonGroup *m_buttonGroup = nullptr;
    ToolType m_currentTool = ToolType::Select;
    
    // Tracks the currently selected line variant (Line, Arrow, or DualArrow)
    ToolType m_currentLineSubTool = ToolType::Line;
    class ToolButton* m_lineGroupButton = nullptr;

    // Tracks the currently selected selection variant (Select or BoxSelect)
    ToolType m_currentSelectSubTool = ToolType::Select;
    class ToolButton* m_selectGroupButton = nullptr;
};

#endif // TOOLBAR_WIDGET_H
