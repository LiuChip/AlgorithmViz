#ifndef DIAGNOSTICS_WIDGET_H
#define DIAGNOSTICS_WIDGET_H

#include <QWidget>
#include <QTreeWidget>

struct DiagnosticMessage {
    enum class Level {
        Info,
        Warning,
        Error
    };

    Level level;
    int lineNumber;
    QString message;
};

class DiagnosticsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DiagnosticsWidget(QWidget *parent = nullptr);
    ~DiagnosticsWidget() override;

    void addMessage(const DiagnosticMessage &msg);
    void clearMessages();
    QTreeWidget *treeWidget() const { return m_treeWidget; }

signals:
    void lineDoubleClicked(int lineNumber);

private slots:
    void onItemDoubleClicked(QTreeWidgetItem *item, int column);

private:
    QTreeWidget *m_treeWidget;
};

#endif // DIAGNOSTICS_WIDGET_H
