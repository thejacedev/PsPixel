#include "historypalette.h"
#include "historymanager.h"
#include "constants.h"
#include <QDateTime>
#include <QApplication>
#include <QMessageBox>

using namespace PixelPaint;

// HistoryListItem implementation
HistoryListItem::HistoryListItem(const QString &actionName, int historyIndex, QListWidget *parent)
    : QListWidgetItem(actionName, parent)
    , m_historyIndex(historyIndex)
    , m_isCurrent(false)
{
    // Set item properties
    setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
}

void HistoryListItem::setIsCurrent(bool current)
{
    m_isCurrent = current;
    
    if (current) {
        // Highlight current item
        setBackground(QBrush(HISTORY_CURRENT_BG));
        setForeground(QBrush(QColor(255, 255, 255)));
        QFont font = this->font();
        font.setBold(true);
        setFont(font);
    } else {
        // Reset to normal appearance
        setBackground(QBrush(QColor(0, 0, 0, 0))); // Transparent
        setForeground(QBrush(QApplication::palette().color(QPalette::Text)));
        QFont font = this->font();
        font.setBold(false);
        setFont(font);
    }
}

// HistoryPalette implementation
HistoryPalette::HistoryPalette(HistoryManager *historyManager, QWidget *parent)
    : QDockWidget("History", parent)
    , m_historyManager(historyManager)
    , m_contentWidget(nullptr)
    , m_mainLayout(nullptr)
    , m_headerFrame(nullptr)
    , m_headerLayout(nullptr)
    , m_titleLabel(nullptr)
    , m_clearButton(nullptr)
    , m_historyList(nullptr)
    , m_currentHistoryIndex(-1)
{
    setupUI();
    
    // Connect to history manager signals
    if (m_historyManager) {
        connect(m_historyManager, &HistoryManager::historyChanged, this, &HistoryPalette::updateHistory);
        connect(m_historyManager, &HistoryManager::undoAvailabilityChanged, this, &HistoryPalette::updateCurrentIndex);
        connect(m_historyManager, &HistoryManager::redoAvailabilityChanged, this, &HistoryPalette::updateCurrentIndex);
    }
    
    // Initial update
    updateHistory();
}

void HistoryPalette::setupUI()
{
    // Set dock widget properties
    setObjectName("HistoryPalette");
    setWindowTitle("History");
    setMinimumWidth(150);
    setMaximumWidth(500);
    setMinimumHeight(200);
    
    // Set size hints for better docking behavior
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    
    // Allow the dock widget to be docked in multiple areas
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    
    // Set features to allow floating, moving, and closing
    setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
    
    // Prevent this dock widget from being tabbed over
    setAttribute(Qt::WA_DeleteOnClose, false);
    
    // Create content widget
    m_contentWidget = new QWidget();
    setWidget(m_contentWidget);
    
    // Main layout
    m_mainLayout = new QVBoxLayout(m_contentWidget);
    m_mainLayout->setContentsMargins(8, 8, 8, 8);
    m_mainLayout->setSpacing(8);
    
    // Header section
    m_headerFrame = new QFrame();
    m_headerFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    m_headerFrame->setLineWidth(1);
    
    m_headerLayout = new QVBoxLayout(m_headerFrame);
    m_headerLayout->setContentsMargins(6, 6, 6, 6);
    m_headerLayout->setSpacing(4);
    
    // Title
    m_titleLabel = new QLabel("History");
    m_titleLabel->setStyleSheet("font-weight: bold; font-size: 12px; color: palette(text);");
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_headerLayout->addWidget(m_titleLabel);
    
    // Clear button
    m_clearButton = new QPushButton("Clear All");
    m_clearButton->setToolTip("Clear all history (Ctrl+Shift+Z)");
    m_clearButton->setMaximumHeight(24);
    m_clearButton->setStyleSheet(
        "QPushButton {"
        "    background-color: palette(button);"
        "    border: 1px solid palette(mid);"
        "    border-radius: 3px;"
        "    padding: 2px 8px;"
        "    font-size: 10px;"
        "}"
        "QPushButton:hover {"
        "    background-color: palette(light);"
        "}"
        "QPushButton:pressed {"
        "    background-color: palette(dark);"
        "}"
    );
    connect(m_clearButton, &QPushButton::clicked, this, &HistoryPalette::onClearHistoryClicked);
    m_headerLayout->addWidget(m_clearButton);
    
    m_mainLayout->addWidget(m_headerFrame);
    
    // History list
    m_historyList = new QListWidget();
    m_historyList->setAlternatingRowColors(true);
    m_historyList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_historyList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_historyList->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_historyList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    // Style the list
    m_historyList->setStyleSheet(
        "QListWidget {"
        "    background-color: palette(base);"
        "    border: 1px solid palette(mid);"
        "    border-radius: 3px;"
        "    selection-background-color: palette(highlight);"
        "    selection-color: palette(highlighted-text);"
        "    outline: none;"
        "}"
        "QListWidget::item {"
        "    padding: 4px 8px;"
        "    border-bottom: 1px solid palette(midlight);"
        "    min-height: 20px;"
        "}"
        "QListWidget::item:hover {"
        "    background-color: palette(light);"
        "}"
        "QListWidget::item:selected {"
        "    background-color: palette(highlight);"
        "    color: palette(highlighted-text);"
        "}"
    );
    
    // Connect list signals
    connect(m_historyList, &QListWidget::itemClicked, this, &HistoryPalette::onHistoryItemClicked);
    connect(m_historyList, &QListWidget::itemDoubleClicked, this, &HistoryPalette::onHistoryItemDoubleClicked);
    
    m_mainLayout->addWidget(m_historyList, 1); // Give list all remaining space
}

void HistoryPalette::updateHistory()
{
    refreshHistoryList();
    updateCurrentIndex();
}

void HistoryPalette::updateCurrentIndex()
{
    if (!m_historyManager) return;
    
    m_currentHistoryIndex = m_historyManager->getCurrentIndex();
    updateItemStyles();
    
    // Scroll to current item
    if (m_currentHistoryIndex >= 0 && m_currentHistoryIndex < m_historyList->count()) {
        m_historyList->setCurrentRow(m_currentHistoryIndex);
        m_historyList->scrollToItem(m_historyList->item(m_currentHistoryIndex));
    }
}

void HistoryPalette::refreshHistoryList()
{
    if (!m_historyManager) return;
    
    // Clear existing items
    m_historyList->clear();
    
    // Add history items
    int historySize = m_historyManager->getHistorySize();
    for (int i = 0; i < historySize; ++i) {
        // Get actual action name from history manager
        QString actionName = m_historyManager->getActionName(i);
        if (actionName.isEmpty()) {
            if (i == 0) {
                actionName = "Initial State";
            } else {
                actionName = QString("Action %1").arg(i);
            }
        }
        
        HistoryListItem *item = new HistoryListItem(actionName, i, m_historyList);
        
        // Add timestamp info as tooltip
        QDateTime timestamp = m_historyManager->getActionTimestamp(i);
        QString tooltipText = QString("History state %1: %2").arg(i + 1).arg(actionName);
        if (timestamp.isValid()) {
            tooltipText += QString("\nTimestamp: %1").arg(timestamp.toString("hh:mm:ss"));
        }
        tooltipText += "\nClick to go to this state";
        item->setToolTip(tooltipText);
        
        // Set icon based on action type - we can expand this later with specific icons
        if (actionName.contains("Brush", Qt::CaseInsensitive)) {
            item->setIcon(QIcon(":/assets/icons/brush.png"));
        } else if (actionName.contains("Erase", Qt::CaseInsensitive)) {
            item->setIcon(QIcon(":/assets/icons/eraser.png"));
        } else if (actionName.contains("Line", Qt::CaseInsensitive)) {
            item->setIcon(QIcon(":/assets/icons/line.png"));
        } else if (actionName.contains("Paint Bucket", Qt::CaseInsensitive) || actionName.contains("Fill", Qt::CaseInsensitive)) {
            item->setIcon(QIcon(":/assets/icons/paintbucket.png"));
        } else if (actionName.contains("Clear", Qt::CaseInsensitive)) {
            item->setIcon(QIcon(":/assets/icons/clear.png"));
        } else if (actionName.contains("Initial", Qt::CaseInsensitive)) {
            item->setIcon(QIcon(":/assets/icons/new.png"));
        } else {
            // Default history icon
            item->setIcon(QIcon(":/assets/icons/history.png"));
        }
    }
    
}

void HistoryPalette::updateItemStyles()
{
    for (int i = 0; i < m_historyList->count(); ++i) {
        HistoryListItem *item = dynamic_cast<HistoryListItem*>(m_historyList->item(i));
        if (item) {
            bool isCurrent = (i == m_currentHistoryIndex);
            item->setIsCurrent(isCurrent);
            
            // Gray out future states (items after current index)
            if (i > m_currentHistoryIndex) {
                item->setForeground(QBrush(HISTORY_FUTURE_TEXT));
                QFont font = item->font();
                font.setItalic(true);
                item->setFont(font);
            } else {
                item->setForeground(QBrush(QApplication::palette().color(QPalette::Text)));
                QFont font = item->font();
                font.setItalic(false);
                item->setFont(font);
            }
        }
    }
}

void HistoryPalette::onHistoryItemClicked(QListWidgetItem *item)
{
    HistoryListItem *historyItem = dynamic_cast<HistoryListItem*>(item);
    if (!historyItem || !m_historyManager) return;
    
    int targetIndex = historyItem->getHistoryIndex();
    int currentIndex = m_historyManager->getCurrentIndex();
    
    // Navigate to the clicked state
    if (targetIndex < currentIndex) {
        // Undo to reach target
        while (m_historyManager->getCurrentIndex() > targetIndex && m_historyManager->canUndo()) {
            m_historyManager->undo();
        }
    } else if (targetIndex > currentIndex) {
        // Redo to reach target
        while (m_historyManager->getCurrentIndex() < targetIndex && m_historyManager->canRedo()) {
            m_historyManager->redo();
        }
    }
    
    // Update display
    updateCurrentIndex();
}

void HistoryPalette::onHistoryItemDoubleClicked(QListWidgetItem *item)
{
    // Same as single click for now
    onHistoryItemClicked(item);
}

void HistoryPalette::onClearHistoryClicked()
{
    if (!m_historyManager) return;
    
    // Confirm with user
    QMessageBox::StandardButton reply = QMessageBox::question(this, 
        "Clear History", 
        "Are you sure you want to clear all history? This cannot be undone.",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        m_historyManager->clearHistory();
    }
}

QSize HistoryPalette::sizeHint() const
{
    // Return a reasonable size hint for dock splitting
    return QSize(250, 400);
} 