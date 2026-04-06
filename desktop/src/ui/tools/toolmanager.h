#ifndef TOOLMANAGER_H
#define TOOLMANAGER_H

#include <QObject>
#include <QMap>
#include <QButtonGroup>
#include <QHBoxLayout>
#include <QPushButton>
#include "constants.h"

namespace PixelPaint {

class BaseTool;
class PixelCanvas;
class HistoryManager;

class ToolManager : public QObject
{
    Q_OBJECT
    friend class ToolPalette;

public:
    explicit ToolManager(QObject *parent = nullptr);
    ~ToolManager();

    // Tool management
    void initializeTools();
    void setCanvas(PixelCanvas *canvas);
    void setHistoryManager(HistoryManager *historyManager);
    void setCurrentColor(const QColor &color);
    void setBrushSize(int size);
    
    // Current tool
    BaseTool* getCurrentTool() const { return m_currentTool; }
    ToolType getCurrentToolType() const;
    
    // UI creation
    QHBoxLayout* createToolbar();
    
    // Tool selection
    void selectTool(ToolType toolType);
    
    // History manager access
    HistoryManager* getHistoryManager() const { return m_historyManager; }

signals:
    void toolChanged(ToolType toolType);
    void colorPicked(const QColor &color);
    void canvasModified();

private slots:
    void onToolButtonClicked();
    void onColorPicked(const QColor &color);
    void onCanvasModified();

private:
    void createToolButton(ToolType toolType);
    QPushButton* getToolButton(ToolType toolType) const;
    
    QMap<ToolType, BaseTool*> m_tools;
    QMap<ToolType, QPushButton*> m_toolButtons;
    QButtonGroup *m_buttonGroup;
    QHBoxLayout *m_toolbarLayout;
    
    BaseTool *m_currentTool;
    PixelCanvas *m_canvas;
    HistoryManager *m_historyManager;
    QColor m_currentColor;
    int m_brushSize;
};

} // namespace PixelPaint

#endif // TOOLMANAGER_H 