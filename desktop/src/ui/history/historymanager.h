#ifndef HISTORYMANAGER_H
#define HISTORYMANAGER_H

#include <QObject>
#include <QImage>
#include <QString>
#include <QDateTime>
#include <QVector>

namespace PixelPaint {

struct HistoryState {
    QImage canvasData;
    QString actionName;
    qint64 timestamp;

    HistoryState() : timestamp(0) {}
    HistoryState(const QImage &data, const QString &name, qint64 time)
        : canvasData(data), actionName(name), timestamp(time) {}
};

class HistoryManager : public QObject
{
    Q_OBJECT

public:
    explicit HistoryManager(QObject *parent = nullptr);
    ~HistoryManager() = default;

    // History operations
    void saveState(const QImage &canvasData, const QString &actionName);
    bool canUndo() const;
    bool canRedo() const;
    QString getUndoActionName() const;
    QString getRedoActionName() const;

    // Operation-based history methods (Photoshop-style)
    void startOperation(const QImage &canvasData);
    void commitOperation(const QImage &canvasData, const QString &actionName);
    void cancelOperation();
    bool isOperationInProgress() const { return m_operationInProgress; }

    // Clear history
    void clearHistory();

    // History limits
    void setMaxHistorySize(int maxSize);
    int getMaxHistorySize() const { return m_maxHistorySize; }

    // Get current state info
    int getCurrentIndex() const { return m_currentIndex; }
    int getHistorySize() const { return m_history.size(); }
    QString getActionName(int index) const;
    QDateTime getActionTimestamp(int index) const;

public slots:
    void undo();
    void redo();

signals:
    void stateChanged(const QImage &canvasData);
    void undoAvailabilityChanged(bool canUndo);
    void redoAvailabilityChanged(bool canRedo);
    void historyChanged();

private:
    void updateAvailability();
    void trimHistoryIfNeeded();
    bool statesAreEqual(const QImage &state1, const QImage &state2) const;

    QVector<HistoryState> m_history;
    int m_currentIndex;
    int m_maxHistorySize;
    bool m_lastCanUndo;
    bool m_lastCanRedo;

    // Operation tracking
    bool m_operationInProgress;
    QImage m_operationStartState;
};

} // namespace PixelPaint

#endif // HISTORYMANAGER_H
