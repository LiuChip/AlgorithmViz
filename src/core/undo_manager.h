#ifndef UNDO_MANAGER_H
#define UNDO_MANAGER_H

#include <QObject>
#include <QUndoStack>
#include <QUndoCommand>

class UndoManager : public QObject
{
    Q_OBJECT
public:
    explicit UndoManager(QObject *parent = nullptr);
    ~UndoManager() override;

    QUndoStack *stack() const { return m_stack; }

public slots:
    void push(QUndoCommand *cmd);
    void undo();
    void redo();
    void clear();

signals:
    void canUndoChanged(bool canUndo);
    void canRedoChanged(bool canRedo);
    void cleanChanged(bool clean);

private:
    QUndoStack *m_stack = nullptr;
};

#endif // UNDO_MANAGER_H
