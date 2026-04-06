#ifndef HISTORYPALETTE_H
#define HISTORYPALETTE_H

#include <QDockWidget>
#include <QVBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QScrollArea>
#include <QPixmap>
#include <QIcon>
#include "constants.h"

namespace PixelPaint {

class HistoryManager;

class HistoryListItem : public QListWidgetItem
{
public:
    explicit HistoryListItem(const QString &actionName, int historyIndex, QListWidget *parent = nullptr);
    
    int getHistoryIndex() const { return m_historyIndex; }
    void setHistoryIndex(int index) { m_historyIndex = index; }
    void setIsCurrent(bool current);
    
private:
    int m_historyIndex;
    bool m_isCurrent;
};

class HistoryPalette : public QDockWidget
{
    Q_OBJECT

public:
    explicit HistoryPalette(HistoryManager *historyManager, QWidget *parent = nullptr);
    ~HistoryPalette() = default;
    
    // Override for better dock widget behavior
    QSize sizeHint() const override;

public slots:
    void updateHistory();
    void updateCurrentIndex();

signals:
    void historyItemClicked(int index);

private slots:
    void onHistoryItemClicked(QListWidgetItem *item);
    void onHistoryItemDoubleClicked(QListWidgetItem *item);
    void onClearHistoryClicked();

private:
    void setupUI();
    void refreshHistoryList();
    void updateItemStyles();
    
    HistoryManager *m_historyManager;
    
    // UI elements
    QWidget *m_contentWidget;
    QVBoxLayout *m_mainLayout;
    
    // Header
    QFrame *m_headerFrame;
    QVBoxLayout *m_headerLayout;
    QLabel *m_titleLabel;
    QPushButton *m_clearButton;
    
    // History list
    QListWidget *m_historyList;
    
    // Current state
    int m_currentHistoryIndex;
};

} // namespace PixelPaint

#endif // HISTORYPALETTE_H 