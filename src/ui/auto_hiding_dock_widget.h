#ifndef AUTO_HIDING_DOCK_WIDGET_H
#define AUTO_HIDING_DOCK_WIDGET_H

#include <QDockWidget>
#include <QPointer>

class QLabel;
class QPushButton;
class QEvent;
class QWidget;

// 停靠时显示的紧凑标题栏。普通面板显示标题和关闭按钮；纯净工具栏只显示拖拽把手。
class HoverTitleBarWidget : public QWidget
{
    Q_OBJECT
public:
    explicit HoverTitleBarWidget(const QString &title, bool showButtons,
                                 QWidget *parent = nullptr);
    ~HoverTitleBarWidget() override = default;

    void setTitle(const QString &title);
    QPushButton *closeButton() const { return m_closeButton; }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QLabel *m_titleLabel = nullptr;
    QPushButton *m_closeButton = nullptr;
};

// 停靠时隐藏标题栏，鼠标进入面板顶部激活区后再显示。
// 普通面板浮动时交还给 Qt/系统原生标题栏；纯净工具栏浮动时仍不显示按钮和标题文字。
class AutoHidingDockWidget : public QDockWidget
{
    Q_OBJECT
public:
    explicit AutoHidingDockWidget(const QString &title, bool showButtons = true,
                                  QWidget *parent = nullptr);
    ~AutoHidingDockWidget() override = default;

    void setWidget(QWidget *widget);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onTopLevelChanged(bool topLevel);

private:
    static constexpr int kHoverTitleHeight = 24;

    void installHoverTracking(QWidget *root);
    void removeHoverTracking(QWidget *root);
    void updateHoverFromGlobalPosition();
    void updateTitleBarState(bool expanded);
    bool usesHoverTitleBar() const;

    HoverTitleBarWidget *m_hoverTitleBar = nullptr;
    bool m_showButtons = true;
    bool m_isHoverExpanded = false;
};

#endif // AUTO_HIDING_DOCK_WIDGET_H
