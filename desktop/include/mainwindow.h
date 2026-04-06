#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QColorDialog>
#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QDockWidget>
#include <QImage>
#include <QDragEnterEvent>
#include <QDropEvent>
#include "constants.h"
#include "../src/fileformat/pspxformat.h"

namespace PixelPaint {

class PixelCanvas;
class ZoomBar;
class StartScreen;
class ToolManager;
class ToolPalette;
class HistoryManager;
class HistoryPalette;
class LayerManager;
class LayerPalette;
class AutoUpdater;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void selectColor();
    void clearCanvas();
    void newProject();
    void saveProject();
    void exportImage();
    void exportAllLayers();
    void loadProject();
    void resizeCanvas();
    void onColorSelected(const QColor& color);
    
    // Zoom and Pan slots
    void zoomIn();
    void zoomOut();
    void resetZoom();
    void resetPan();
    void fitToWindow();
    void onZoomBarChanged(int value);
    void onStartScreenProjectSelected(const QString &filePath);
    void onStartScreenNewProject(int width, int height, int pixelSize, const QColor &backgroundColor);
    
    // History slots
    void undo();
    void redo();

    // Canvas transform slots
    void flipHorizontal();
    void flipVertical();
    void rotateCW();
    void rotateCCW();

    // Clipboard slots
    void copySelection();
    void pasteSelection();

protected:
    // Override to control dock widget behavior
    bool event(QEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    // UI setup methods
    void setupWindow();
    void setupUI();
    void setupMenuBar();
    void setupConnections();
    void createStatusBar();
    void updateZoomBar();
    void showStartScreen();
    void showMainInterface();
    bool shouldShowStartScreen();
    void addToRecentProjects(const QString &filePath);
    void loadCustomColors();
    void saveCustomColors();
    void setupColorDialog(QColorDialog &dialog);
    
    // Core components
    PixelCanvas *m_canvas;
    StartScreen *m_startScreen;
    ToolManager *m_toolManager;
    ToolPalette *m_toolPalette;
    HistoryManager *m_historyManager;
    HistoryPalette *m_historyPalette;
    LayerManager *m_layerManager2;
    LayerPalette *m_layerPalette;
    
    // Menu actions
    QAction *m_undoAction;
    QAction *m_redoAction;
    QAction *m_mirrorHAction;
    QAction *m_mirrorVAction;
    
    // UI elements
    QPushButton *m_colorButton;
    QSpinBox *m_brushSizeSpinBox;
    QLabel *m_brushSizeLabel;
    QWidget *m_centralWidget;
    QVBoxLayout *m_mainLayout;
    QHBoxLayout *m_statusBarLayout;
    
    // Zoom controls
    ZoomBar *m_zoomBar;
    QPushButton *m_resetZoomButton;
    
    // State
    QColor m_currentColor;
    ProjectData m_currentProject;
    bool m_projectModified;
    QString m_currentProjectPath;
    QVector<QColor> m_customColors;
    QImage m_clipboard;
    AutoUpdater *m_updater;
};

} // namespace PixelPaint

#endif // MAINWINDOW_H 