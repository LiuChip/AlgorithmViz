#include "auto_hiding_dock_widget.h"

#include <QChildEvent>
#include <QCursor>
#include <QEvent>
#include <QHBoxLayout>
#include <QHoverEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QTimer>

HoverTitleBarWidget::HoverTitleBarWidget(const QString &title, bool showButtons,
                                         QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(0);
    setObjectName(QStringLiteral("HoverDockTitleBar"));
    setStyleSheet(QStringLiteral(
        "QWidget#HoverDockTitleBar { background: #303133; }"
        "QLabel { color: #f2f6fc; font-size: 11px; font-weight: 600; background: transparent; }"
        "QPushButton { background: transparent; border: none; color: #c0c4cc; "
        "font-size: 15px; font-weight: bold; }"
        "QPushButton:hover { color: #f56c6c; }"));

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 2, 8, 2);
    layout->setSpacing(4);

    if (showButtons) {
        m_titleLabel = new QLabel(title, this);
        layout->addWidget(m_titleLabel);
        layout->addStretch();

        m_closeButton = new QPushButton(QStringLiteral("×"), this);
        m_closeButton->setObjectName(QStringLiteral("HoverDockCloseButton"));
        m_closeButton->setToolTip(QStringLiteral("关闭面板"));
        m_closeButton->setAccessibleName(QStringLiteral("关闭面板"));
        m_closeButton->setFixedSize(18, 18);
        layout->addWidget(m_closeButton);
    } else {
        // 工具栏纯净模式仅保留中央拖拽把手，不显示标题文字或按钮。
        layout->addStretch();
    }
}

void HoverTitleBarWidget::setTitle(const QString &title)
{
    if (m_titleLabel)
        m_titleLabel->setText(title);
}

void HoverTitleBarWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor(QStringLiteral("#c0c4cc")), 1.2));
    const int midY = height() / 2;
    const int midX = width() / 2;
    painter.drawLine(midX - 10, midY - 2, midX + 10, midY - 2);
    painter.drawLine(midX - 10, midY + 2, midX + 10, midY + 2);
}

AutoHidingDockWidget::AutoHidingDockWidget(const QString &title, bool showButtons,
                                           QWidget *parent)
    : QDockWidget(title, parent), m_showButtons(showButtons)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);
    setStyleSheet(QStringLiteral(
        "QDockWidget { border: none; titlebar-close-icon: none; titlebar-normal-icon: none; }"));

    m_hoverTitleBar = new HoverTitleBarWidget(title, showButtons, this);
    if (m_hoverTitleBar->closeButton()) {
        connect(m_hoverTitleBar->closeButton(), &QPushButton::clicked,
                this, &QDockWidget::close);
    }

    setTitleBarWidget(m_hoverTitleBar);
    connect(this, &QDockWidget::topLevelChanged,
            this, &AutoHidingDockWidget::onTopLevelChanged);
    connect(this, &QDockWidget::windowTitleChanged,
            m_hoverTitleBar, &HoverTitleBarWidget::setTitle);

    installHoverTracking(this);
    installHoverTracking(m_hoverTitleBar);
}

void AutoHidingDockWidget::setWidget(QWidget *widget)
{
    if (QWidget *oldWidget = QDockWidget::widget())
        removeHoverTracking(oldWidget);

    QDockWidget::setWidget(widget);
    if (widget)
        installHoverTracking(widget);
}

bool AutoHidingDockWidget::usesHoverTitleBar() const
{
    return !isFloating() || !m_showButtons;
}

void AutoHidingDockWidget::onTopLevelChanged(bool topLevel)
{
    if (topLevel && m_showButtons) {
        // 普通浮动面板使用 Qt/系统原生窗口框架与操作按钮。
        setTitleBarWidget(nullptr);
        m_isHoverExpanded = false;
        m_hoverTitleBar->setFixedHeight(0);
        return;
    }

    // 停靠面板与纯净工具栏（包括浮动状态）使用无文字/可悬停的自定义标题栏。
    setTitleBarWidget(m_hoverTitleBar);
    m_isHoverExpanded = false;
    m_hoverTitleBar->setFixedHeight(0);
}

void AutoHidingDockWidget::installHoverTracking(QWidget *root)
{
    if (!root)
        return;

    root->setMouseTracking(true);
    root->setAttribute(Qt::WA_Hover, true);
    root->installEventFilter(this);
    const auto children = root->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly);
    for (QWidget *child : children)
        installHoverTracking(child);
}

void AutoHidingDockWidget::removeHoverTracking(QWidget *root)
{
    if (!root)
        return;

    root->removeEventFilter(this);
    const auto children = root->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly);
    for (QWidget *child : children)
        removeHoverTracking(child);
}

void AutoHidingDockWidget::updateHoverFromGlobalPosition()
{
    if (!usesHoverTitleBar()) {
        updateTitleBarState(false);
        return;
    }

    const QPoint dockPosition = mapFromGlobal(QCursor::pos());
    const bool inside = rect().contains(dockPosition);
    updateTitleBarState(inside && dockPosition.y() >= 0 &&
                        dockPosition.y() <= kHoverTitleHeight);
}

bool AutoHidingDockWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::ChildAdded) {
        auto *childEvent = static_cast<QChildEvent *>(event);
        if (auto *childWidget = qobject_cast<QWidget *>(childEvent->child()))
            installHoverTracking(childWidget);
    }

    if (usesHoverTitleBar()) {
        if (event->type() == QEvent::HoverMove || event->type() == QEvent::MouseMove ||
            event->type() == QEvent::Enter) {
            auto *watchedWidget = qobject_cast<QWidget *>(watched);
            if (watchedWidget) {
                QPoint localPosition;
                if (event->type() == QEvent::HoverMove) {
                    localPosition = static_cast<QHoverEvent *>(event)->position().toPoint();
                } else if (event->type() == QEvent::MouseMove) {
                    localPosition = static_cast<QMouseEvent *>(event)->position().toPoint();
                } else {
                    localPosition = watchedWidget->mapFromGlobal(QCursor::pos());
                }
                const QPoint dockPosition = watchedWidget->mapTo(this, localPosition);
                updateTitleBarState(rect().contains(dockPosition) &&
                                    dockPosition.y() >= 0 &&
                                    dockPosition.y() <= kHoverTitleHeight);
            }
        } else if (event->type() == QEvent::Leave || event->type() == QEvent::Hide) {
            // Leave 在父子控件之间切换时也会触发。延迟到事件队列尾部后读取全局位置，
            // 避免从编辑器移到标题栏/按钮时出现闪烁收回。
            QTimer::singleShot(0, this, &AutoHidingDockWidget::updateHoverFromGlobalPosition);
        }
    }

    return QDockWidget::eventFilter(watched, event);
}

void AutoHidingDockWidget::updateTitleBarState(bool expanded)
{
    if (!usesHoverTitleBar())
        expanded = false;
    if (expanded == m_isHoverExpanded)
        return;

    m_isHoverExpanded = expanded;
    m_hoverTitleBar->setFixedHeight(expanded ? kHoverTitleHeight : 0);
}
