#include "diagnostics_widget.h"
#include <QVBoxLayout>
#include <QHeaderView>

namespace {
int severityPriority(DiagnosticMessage::Level level)
{
    switch (level) {
    case DiagnosticMessage::Level::Error:
        return 0;
    case DiagnosticMessage::Level::Warning:
        return 1;
    case DiagnosticMessage::Level::Info:
        return 2;
    }
    return 3;
}
} // namespace

DiagnosticsWidget::DiagnosticsWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setColumnCount(3);
    m_treeWidget->setHeaderLabels({"Level", "Line", "Message"});
    
    m_treeWidget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_treeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_treeWidget->header()->setSectionResizeMode(2, QHeaderView::Stretch);

    m_treeWidget->setAlternatingRowColors(true);
    m_treeWidget->setRootIsDecorated(false);

    layout->addWidget(m_treeWidget);

    connect(m_treeWidget, &QTreeWidget::itemDoubleClicked, this, &DiagnosticsWidget::onItemDoubleClicked);
}

DiagnosticsWidget::~DiagnosticsWidget() = default;

void DiagnosticsWidget::addMessage(const DiagnosticMessage &msg)
{
    auto *item = new QTreeWidgetItem;
    
    QString levelStr;
    switch (msg.level) {
        case DiagnosticMessage::Level::Info:
            levelStr = "Info";
            break;
        case DiagnosticMessage::Level::Warning:
            levelStr = "Warning";
            item->setForeground(0, Qt::darkYellow);
            item->setForeground(2, Qt::darkYellow);
            break;
        case DiagnosticMessage::Level::Error:
            levelStr = "Error";
            item->setForeground(0, Qt::red);
            item->setForeground(2, Qt::red);
            break;
    }

    item->setText(0, levelStr);
    item->setData(0, Qt::UserRole, severityPriority(msg.level));
    
    if (msg.lineNumber > 0) {
        item->setText(1, QString::number(msg.lineNumber));
    } else {
        item->setText(1, "-");
    }
    
    item->setText(2, msg.message);
    item->setData(1, Qt::UserRole, msg.lineNumber);

    // Problems 面板始终优先显示错误，其次是警告和提示；同级消息保持产生顺序。
    const int priority = severityPriority(msg.level);
    int insertionIndex = m_treeWidget->topLevelItemCount();
    for (int row = 0; row < m_treeWidget->topLevelItemCount(); ++row) {
        if (m_treeWidget->topLevelItem(row)->data(0, Qt::UserRole).toInt() > priority) {
            insertionIndex = row;
            break;
        }
    }
    m_treeWidget->insertTopLevelItem(insertionIndex, item);
}

void DiagnosticsWidget::clearMessages()
{
    m_treeWidget->clear();
}

void DiagnosticsWidget::onItemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    if (!item) return;
    
    int lineNum = item->data(1, Qt::UserRole).toInt();
    if (lineNum > 0) {
        emit lineDoubleClicked(lineNum);
    }
}
