#include "historymanager.h"
#include <QDateTime>
#include <cstring>

using namespace PixelPaint;

HistoryManager::HistoryManager(QObject *parent)
    : QObject(parent)
    , m_currentIndex(-1)
    , m_maxHistorySize(50)
    , m_lastCanUndo(false)
    , m_lastCanRedo(false)
    , m_operationInProgress(false)
{
}

void HistoryManager::saveState(const QImage &canvasData, const QString &actionName)
{
    // If we're not at the end of history, remove everything after current position
    if (m_currentIndex < m_history.size() - 1) {
        m_history.erase(m_history.begin() + m_currentIndex + 1, m_history.end());
    }

    // Don't save if the state is identical to the current state
    if (!m_history.isEmpty() && m_currentIndex >= 0 &&
        statesAreEqual(canvasData, m_history[m_currentIndex].canvasData)) {
        return;
    }

    HistoryState newState(canvasData, actionName, QDateTime::currentMSecsSinceEpoch());
    m_history.append(newState);
    m_currentIndex = m_history.size() - 1;

    trimHistoryIfNeeded();

    updateAvailability();
    emit historyChanged();
}

void HistoryManager::startOperation(const QImage &canvasData)
{
    m_operationInProgress = true;
    m_operationStartState = canvasData;
}

void HistoryManager::commitOperation(const QImage &canvasData, const QString &actionName)
{
    if (!m_operationInProgress) {
        saveState(canvasData, actionName);
        return;
    }

    m_operationInProgress = false;

    if (!statesAreEqual(m_operationStartState, canvasData)) {
        saveState(canvasData, actionName);
    }

    m_operationStartState = QImage();
}

void HistoryManager::cancelOperation()
{
    if (m_operationInProgress) {
        m_operationInProgress = false;
        m_operationStartState = QImage();
    }
}

bool HistoryManager::statesAreEqual(const QImage &state1, const QImage &state2) const
{
    if (state1.size() != state2.size()) return false;
    if (state1.isNull() && state2.isNull()) return true;
    if (state1.isNull() || state2.isNull()) return false;
    return std::memcmp(state1.constBits(), state2.constBits(), state1.sizeInBytes()) == 0;
}

bool HistoryManager::canUndo() const
{
    return m_currentIndex > 0;
}

bool HistoryManager::canRedo() const
{
    return m_currentIndex < m_history.size() - 1;
}

QString HistoryManager::getUndoActionName() const
{
    if (canUndo()) {
        return m_history[m_currentIndex].actionName;
    }
    return QString();
}

QString HistoryManager::getRedoActionName() const
{
    if (canRedo()) {
        return m_history[m_currentIndex + 1].actionName;
    }
    return QString();
}

QString HistoryManager::getActionName(int index) const
{
    if (index >= 0 && index < m_history.size()) {
        return m_history[index].actionName;
    }
    return QString();
}

QDateTime HistoryManager::getActionTimestamp(int index) const
{
    if (index >= 0 && index < m_history.size()) {
        return QDateTime::fromMSecsSinceEpoch(m_history[index].timestamp);
    }
    return QDateTime();
}

void HistoryManager::clearHistory()
{
    m_history.clear();
    m_currentIndex = -1;
    m_operationInProgress = false;
    m_operationStartState = QImage();
    updateAvailability();
    emit historyChanged();
}

void HistoryManager::setMaxHistorySize(int maxSize)
{
    m_maxHistorySize = qMax(1, maxSize);
    trimHistoryIfNeeded();
}

void HistoryManager::undo()
{
    if (!canUndo()) return;

    m_currentIndex--;
    emit stateChanged(m_history[m_currentIndex].canvasData);
    updateAvailability();
}

void HistoryManager::redo()
{
    if (!canRedo()) return;

    m_currentIndex++;
    emit stateChanged(m_history[m_currentIndex].canvasData);
    updateAvailability();
}

void HistoryManager::updateAvailability()
{
    bool currentCanUndo = canUndo();
    bool currentCanRedo = canRedo();

    if (currentCanUndo != m_lastCanUndo) {
        m_lastCanUndo = currentCanUndo;
        emit undoAvailabilityChanged(currentCanUndo);
    }

    if (currentCanRedo != m_lastCanRedo) {
        m_lastCanRedo = currentCanRedo;
        emit redoAvailabilityChanged(currentCanRedo);
    }
}

void HistoryManager::trimHistoryIfNeeded()
{
    while (m_history.size() > m_maxHistorySize) {
        m_history.removeFirst();
        if (m_currentIndex > 0) {
            m_currentIndex--;
        }
    }

    if (m_currentIndex >= m_history.size()) {
        m_currentIndex = m_history.size() - 1;
    }
}
