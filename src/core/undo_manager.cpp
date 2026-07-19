#include "undo_manager.h"

UndoManager::UndoManager(QObject *parent)
    : QObject(parent), m_stack(new QUndoStack(this))
{
    connect(m_stack, &QUndoStack::canUndoChanged, this, &UndoManager::canUndoChanged);
    connect(m_stack, &QUndoStack::canRedoChanged, this, &UndoManager::canRedoChanged);
    connect(m_stack, &QUndoStack::cleanChanged, this, &UndoManager::cleanChanged);
}

UndoManager::~UndoManager() = default;

void UndoManager::push(QUndoCommand *cmd)
{
    if (m_stack && cmd) {
        m_stack->push(cmd);
    }
}

void UndoManager::undo()
{
    if (m_stack) {
        m_stack->undo();
    }
}

void UndoManager::redo()
{
    if (m_stack) {
        m_stack->redo();
    }
}

void UndoManager::clear()
{
    if (m_stack) {
        m_stack->clear();
    }
}
