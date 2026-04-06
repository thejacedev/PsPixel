#include "mainwindow.h"
#include "pixelcanvas.h"
#include "constants.h"
#include "ui/bottom/zoombar.h"
#include "ui/top/saveexportdialog.h"
#include "ui/top/newprojectdialog.h"
#include "ui/top/keybindsdialog.h"
#include "ui/top/autoupdater.h"
#include "fileformat/pspxformat.h"
#include "ui/start/startscreen.h"
#include "ui/tools/toolmanager.h"
#include "ui/tools/toolpalette.h"
#include "ui/tools/selecttool.h"
#include "ui/history/historymanager.h"
#include "ui/history/historypalette.h"
#include "ui/layers/layer.h"
#include "ui/layers/layermanager.h"
#include "ui/layers/layerpalette.h"
#include <QDateTime>
#include <QFileInfo>
#include <QTimer>
#include <QTransform>
#include <QPainter>
#include <QRegularExpression>
#include <QStandardPaths>

using namespace PixelPaint;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_canvas(nullptr)
    , m_startScreen(nullptr)
    , m_toolManager(nullptr)
    , m_toolPalette(nullptr)
    , m_historyManager(nullptr)
    , m_historyPalette(nullptr)
    , m_layerManager2(nullptr)
    , m_layerPalette(nullptr)
    , m_undoAction(nullptr)
    , m_redoAction(nullptr)
    , m_mirrorHAction(nullptr)
    , m_mirrorVAction(nullptr)
    , m_colorButton(nullptr)
    , m_brushSizeSpinBox(nullptr)
    , m_brushSizeLabel(nullptr)
    , m_centralWidget(nullptr)
    , m_mainLayout(nullptr)
    , m_statusBarLayout(nullptr)
    , m_zoomBar(nullptr)
    , m_resetZoomButton(nullptr)
    , m_currentColor(Qt::black)
    , m_projectModified(false)
    , m_updater(nullptr)
{
    setupWindow();
    setupMenuBar();
    loadCustomColors();

    // Check for updates on startup (silent — no popup if already latest)
    m_updater = new AutoUpdater(this, this);
    QTimer::singleShot(2000, this, [this]() { m_updater->checkForUpdates(true); });
    
    if (shouldShowStartScreen()) {
        showStartScreen();
    } else {
        setupUI();
        setupConnections();
    }
}

MainWindow::~MainWindow() = default;

void MainWindow::setupWindow()
{
    setWindowTitle(QString("%1 - %2").arg(APP_NAME, APP_DESCRIPTION));
    setMinimumSize(MIN_WINDOW_WIDTH, MIN_WINDOW_HEIGHT);
    resize(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
    
    // Set application icon
    setWindowIcon(QIcon(":/assets/icon.png"));
}

void MainWindow::setupUI()
{
    m_centralWidget = new QWidget();
    setCentralWidget(m_centralWidget);
    m_mainLayout = new QVBoxLayout(m_centralWidget);

    createStatusBar();

    m_toolManager = new ToolManager(this);
    m_historyManager = new HistoryManager(this);

    m_canvas = new PixelCanvas();
    m_canvas->setCurrentColor(m_currentColor);
    m_canvas->setToolManager(m_toolManager);
    m_toolManager->setCanvas(m_canvas);
    m_toolManager->setHistoryManager(m_historyManager);
    m_toolManager->setCurrentColor(m_currentColor);
    m_toolManager->setBrushSize(DEFAULT_BRUSH_SIZE);

    m_toolPalette = new ToolPalette(m_toolManager, this);
    m_toolPalette->updateCurrentColor(m_currentColor);
    m_toolPalette->updateBrushSize(DEFAULT_BRUSH_SIZE);
    addDockWidget(Qt::LeftDockWidgetArea, m_toolPalette);

    m_historyPalette = new HistoryPalette(m_historyManager, this);
    addDockWidget(Qt::RightDockWidgetArea, m_historyPalette);

    // Layer system
    m_layerManager2 = new LayerManager(this);
    m_layerManager2->reset(DEFAULT_CANVAS_WIDTH, DEFAULT_CANVAS_HEIGHT);
    m_canvas->setLayerManager(m_layerManager2);

    m_layerPalette = new LayerPalette(m_layerManager2, this);
    addDockWidget(Qt::RightDockWidgetArea, m_layerPalette);

    setDockOptions(QMainWindow::AllowNestedDocks | QMainWindow::AnimatedDocks);

    // Set initial dock sizes when both are visible
    QTimer::singleShot(100, this, [this]() {
        if (m_toolPalette && m_historyPalette) {
            resizeDocks({m_toolPalette, m_historyPalette}, {200, 200}, Qt::Horizontal);
        }
    });

    m_mainLayout->addWidget(m_canvas, 1);
    m_mainLayout->addLayout(m_statusBarLayout);

    updateZoomBar();
}

void MainWindow::setupMenuBar()
{
    QMenuBar *menuBar = this->menuBar();
    
    // File menu
    QMenu *fileMenu = menuBar->addMenu("File");
    
    QAction *newAction = fileMenu->addAction("New Project");
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &MainWindow::newProject);
    
    fileMenu->addSeparator();
    
    QAction *saveAction = fileMenu->addAction("Save Project");
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveProject);
    
    QAction *exportAction = fileMenu->addAction("Export Image");
    exportAction->setShortcut(QKeySequence("Ctrl+E"));
    connect(exportAction, &QAction::triggered, this, &MainWindow::exportImage);
    
    QAction *exportLayersAction = fileMenu->addAction("Export All Layers");
    exportLayersAction->setShortcut(QKeySequence("Ctrl+Shift+E"));
    connect(exportLayersAction, &QAction::triggered, this, &MainWindow::exportAllLayers);

    fileMenu->addSeparator();

    QAction *loadAction = fileMenu->addAction("Open Project");
    loadAction->setShortcut(QKeySequence::Open);
    connect(loadAction, &QAction::triggered, this, &MainWindow::loadProject);

    fileMenu->addSeparator();
    
    QAction *exitAction = fileMenu->addAction("Exit");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    
    // Edit menu
    QMenu *editMenu = menuBar->addMenu("Edit");
    
    m_undoAction = editMenu->addAction("Undo");
    m_undoAction->setShortcut(QKeySequence::Undo);
    m_undoAction->setEnabled(false); // Initially disabled
    connect(m_undoAction, &QAction::triggered, this, &MainWindow::undo);
    
    m_redoAction = editMenu->addAction("Redo");
    m_redoAction->setShortcut(QKeySequence::Redo);
    m_redoAction->setEnabled(false); // Initially disabled
    connect(m_redoAction, &QAction::triggered, this, &MainWindow::redo);
    
    editMenu->addSeparator();
    
    QAction *clearCanvasAction = editMenu->addAction("Clear Canvas");
    clearCanvasAction->setShortcut(QKeySequence("Ctrl+N"));
    connect(clearCanvasAction, &QAction::triggered, this, &MainWindow::clearCanvas);
    
    QAction *resizeCanvasAction = editMenu->addAction("Resize Canvas");
    resizeCanvasAction->setShortcut(QKeySequence("Ctrl+R"));
    connect(resizeCanvasAction, &QAction::triggered, this, &MainWindow::resizeCanvas);

    editMenu->addSeparator();

    QAction *flipHAction = editMenu->addAction("Flip Horizontal");
    flipHAction->setShortcut(QKeySequence("Ctrl+Shift+H"));
    connect(flipHAction, &QAction::triggered, this, &MainWindow::flipHorizontal);

    QAction *flipVAction = editMenu->addAction("Flip Vertical");
    flipVAction->setShortcut(QKeySequence("Ctrl+Shift+V"));
    connect(flipVAction, &QAction::triggered, this, &MainWindow::flipVertical);

    QAction *rotateCWAction = editMenu->addAction("Rotate 90\u00b0 CW");
    rotateCWAction->setShortcut(QKeySequence("Ctrl+Shift+R"));
    connect(rotateCWAction, &QAction::triggered, this, &MainWindow::rotateCW);

    QAction *rotateCCWAction = editMenu->addAction("Rotate 90\u00b0 CCW");
    rotateCCWAction->setShortcut(QKeySequence("Ctrl+Shift+L"));
    connect(rotateCCWAction, &QAction::triggered, this, &MainWindow::rotateCCW);

    editMenu->addSeparator();

    QAction *copyAction = editMenu->addAction("Copy");
    copyAction->setShortcut(QKeySequence::Copy);
    connect(copyAction, &QAction::triggered, this, &MainWindow::copySelection);

    QAction *pasteAction = editMenu->addAction("Paste");
    pasteAction->setShortcut(QKeySequence::Paste);
    connect(pasteAction, &QAction::triggered, this, &MainWindow::pasteSelection);

    // View menu
    QMenu *viewMenu = menuBar->addMenu("View");
    
    QAction *zoomInAction = viewMenu->addAction("Zoom In");
    zoomInAction->setShortcut(QKeySequence::ZoomIn);
    connect(zoomInAction, &QAction::triggered, this, &MainWindow::zoomIn);
    
    QAction *zoomOutAction = viewMenu->addAction("Zoom Out");
    zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    connect(zoomOutAction, &QAction::triggered, this, &MainWindow::zoomOut);
    
    QAction *resetZoomAction = viewMenu->addAction("Reset Zoom");
    resetZoomAction->setShortcut(QKeySequence("Ctrl+0"));
    connect(resetZoomAction, &QAction::triggered, this, &MainWindow::resetZoom);
    
    viewMenu->addSeparator();
    
    QAction *resetPanAction = viewMenu->addAction("Reset Pan");
    resetPanAction->setShortcut(QKeySequence("Ctrl+Shift+0"));
    connect(resetPanAction, &QAction::triggered, this, &MainWindow::resetPan);
    
    QAction *fitToWindowAction = viewMenu->addAction("Fit to Window");
    fitToWindowAction->setShortcut(QKeySequence("Ctrl+F"));
    connect(fitToWindowAction, &QAction::triggered, this, &MainWindow::fitToWindow);
    
    viewMenu->addSeparator();
    
    QAction *toggleToolPaletteAction = viewMenu->addAction("Toggle Tool Palette");
    toggleToolPaletteAction->setShortcut(QKeySequence("F1"));
    connect(toggleToolPaletteAction, &QAction::triggered, this, [this]() {
        if (m_toolPalette) {
            m_toolPalette->setVisible(!m_toolPalette->isVisible());
        }
    });
    
    QAction *toggleHistoryPaletteAction = viewMenu->addAction("Toggle History Palette");
    toggleHistoryPaletteAction->setShortcut(QKeySequence("F2"));
    connect(toggleHistoryPaletteAction, &QAction::triggered, this, [this]() {
        if (m_historyPalette) {
            m_historyPalette->setVisible(!m_historyPalette->isVisible());
        }
    });
    
    viewMenu->addSeparator();

    m_mirrorHAction = viewMenu->addAction("Mirror Horizontal");
    m_mirrorHAction->setCheckable(true);
    m_mirrorHAction->setShortcut(QKeySequence("Alt+H"));
    connect(m_mirrorHAction, &QAction::toggled, this, [this](bool on) {
        if (m_canvas) m_canvas->setMirrorHorizontal(on);
    });

    m_mirrorVAction = viewMenu->addAction("Mirror Vertical");
    m_mirrorVAction->setCheckable(true);
    m_mirrorVAction->setShortcut(QKeySequence("Alt+V"));
    connect(m_mirrorVAction, &QAction::toggled, this, [this](bool on) {
        if (m_canvas) m_canvas->setMirrorVertical(on);
    });

    // Tool shortcuts (respect saved keybindings)
    auto addToolShortcut = [this](const QString &bindingKey, ToolType toolType) {
        QAction *action = new QAction(this);
        action->setShortcut(KeybindsDialog::bindingFor(bindingKey));
        connect(action, &QAction::triggered, [this, toolType]() {
            if (m_toolManager) m_toolManager->selectTool(toolType);
        });
        addAction(action);
    };

    addToolShortcut("tool.select",     ToolType::Select);
    addToolShortcut("tool.brush",      ToolType::Brush);
    addToolShortcut("tool.eraser",     ToolType::Eraser);
    addToolShortcut("tool.eyedropper", ToolType::Eyedropper);
    addToolShortcut("tool.fill",       ToolType::PaintBucket);
    addToolShortcut("tool.line",       ToolType::Line);
    addToolShortcut("tool.rectangle",  ToolType::Rectangle);
    addToolShortcut("tool.circle",     ToolType::Circle);

    // Settings menu
    QMenu *settingsMenu = menuBar->addMenu("Settings");
    QAction *keybindsAction = settingsMenu->addAction("Keyboard Shortcuts...");
    connect(keybindsAction, &QAction::triggered, this, [this]() {
        KeybindsDialog dialog(this);
        dialog.exec();
    });

    // Help menu
    QMenu *helpMenu = menuBar->addMenu("Help");

    QAction *checkUpdatesAction = helpMenu->addAction("Check for Updates...");
    connect(checkUpdatesAction, &QAction::triggered, this, [this]() {
        if (m_updater) m_updater->checkForUpdates(false);
    });

    helpMenu->addSeparator();

    QAction *aboutAction = helpMenu->addAction("About");
    connect(aboutAction, &QAction::triggered, [this]() {
        QMessageBox::about(this, "About",
            QString("%1 v%2\n\n%3\n\nControls:\n"
                   "• Ctrl+Mouse Wheel: Zoom in/out\n"
                   "• Middle Mouse Button: Pan around\n"
                   "• Left Mouse Button: Draw").arg(APP_NAME, VERSION, APP_DESCRIPTION));
    });
}

void MainWindow::createStatusBar()
{
    m_statusBarLayout = new QHBoxLayout();
    
    // Zoom controls
    m_zoomBar = new ZoomBar();
    connect(m_zoomBar, &ZoomBar::zoomChanged, this, &MainWindow::onZoomBarChanged);
    
    m_resetZoomButton = new QPushButton("Reset Zoom");
    m_resetZoomButton->setToolTip("Reset Zoom (Ctrl+0)");
    connect(m_resetZoomButton, &QPushButton::clicked, this, &MainWindow::resetZoom);
    
    // Add widgets to status bar layout
    m_statusBarLayout->addStretch(); // Push everything to the right
    m_statusBarLayout->addWidget(m_zoomBar);
    m_statusBarLayout->addWidget(m_resetZoomButton);
}

void MainWindow::setupConnections()
{
    // Connect tool palette signals
    if (m_toolPalette) {
        connect(m_toolPalette, &ToolPalette::toolSelected, m_toolManager, &ToolManager::selectTool);
        connect(m_toolPalette, &ToolPalette::brushSizeChanged, m_toolManager, &ToolManager::setBrushSize);
        connect(m_toolPalette, &ToolPalette::brushSizeChanged, m_canvas, &PixelCanvas::setBrushSize);
        connect(m_toolPalette, &ToolPalette::colorButtonClicked, this, &MainWindow::selectColor);
    }
    
    // Connect tool manager signals
    if (m_toolManager) {
        connect(m_toolManager, &ToolManager::colorPicked, this, &MainWindow::onColorSelected);
        connect(m_toolManager, &ToolManager::canvasModified, this, [this]() {
            m_projectModified = true;
        });
        connect(m_toolManager, &ToolManager::toolChanged, this, [this](ToolType toolType) {
            if (m_toolPalette) {
                m_toolPalette->updateCurrentTool();
            }
        });
    }
    
    // Connect canvas zoom changes to update zoom bar
    connect(m_canvas, &PixelCanvas::zoomFactorChanged, this, &MainWindow::updateZoomBar);
    
    // Connect history manager signals
    if (m_historyManager) {
        connect(m_historyManager, &HistoryManager::stateChanged, this, [this](const QImage &canvasData) {
            if (m_canvas) {
                m_canvas->canvasImageRef() = canvasData;
                m_canvas->update();
            }
        });
        
        connect(m_historyManager, &HistoryManager::undoAvailabilityChanged, this, [this](bool canUndo) {
            if (m_undoAction) {
                m_undoAction->setEnabled(canUndo);
                QString actionName = m_historyManager->getUndoActionName();
                if (canUndo && !actionName.isEmpty()) {
                    m_undoAction->setText(QString("Undo %1").arg(actionName));
                } else {
                    m_undoAction->setText("Undo");
                }
            }
        });
        
        connect(m_historyManager, &HistoryManager::redoAvailabilityChanged, this, [this](bool canRedo) {
            if (m_redoAction) {
                m_redoAction->setEnabled(canRedo);
                QString actionName = m_historyManager->getRedoActionName();
                if (canRedo && !actionName.isEmpty()) {
                    m_redoAction->setText(QString("Redo %1").arg(actionName));
                } else {
                    m_redoAction->setText("Redo");
                }
            }
        });
    }
}

void MainWindow::selectColor()
{
    QColorDialog dialog(this);
    setupColorDialog(dialog);
    dialog.setCurrentColor(m_currentColor);
    
    if (dialog.exec() == QDialog::Accepted) {
        QColor color = dialog.currentColor();
        onColorSelected(color);
        
        // Add to custom colors if not already there
        if (!m_customColors.contains(color)) {
            m_customColors.prepend(color);
            if (m_customColors.size() > 16) { // Limit to 16 custom colors
                m_customColors.removeLast();
            }
            saveCustomColors();
        }
        
        // Also get custom colors from the dialog itself in case user picked from the custom area
        for (int i = 0; i < 16; ++i) {
            QColor customColor = dialog.customColor(i);
            if (customColor.isValid() && !m_customColors.contains(customColor)) {
                m_customColors.append(customColor);
            }
        }
        
        // Remove duplicates and limit size
        QVector<QColor> uniqueColors;
        for (const QColor &color : m_customColors) {
            if (!uniqueColors.contains(color)) {
                uniqueColors.append(color);
            }
        }
        m_customColors = uniqueColors;
        
        while (m_customColors.size() > 16) {
            m_customColors.removeLast();
        }
        saveCustomColors();
    }
}

void MainWindow::onColorSelected(const QColor& color)
{
    m_currentColor = color;
    m_canvas->setCurrentColor(color);
    
    // Update tool manager
    if (m_toolManager) {
        m_toolManager->setCurrentColor(color);
    }
    
    // Update tool palette
    if (m_toolPalette) {
        m_toolPalette->updateCurrentColor(color);
    }
}

void MainWindow::clearCanvas()
{
    // Save state before clearing
    if (m_historyManager) {
        m_historyManager->saveState(m_canvas->getCanvasState(), "Clear Canvas");
    }
    
    m_canvas->clearCanvas();
}

void MainWindow::newProject()
{
    // Ask if user wants to save current project if modified
    if (m_projectModified) {
        QMessageBox::StandardButton result = QMessageBox::question(this, "Unsaved Changes",
            "You have unsaved changes. Do you want to save the current project before creating a new one?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        
        if (result == QMessageBox::Save) {
            saveProject();
        } else if (result == QMessageBox::Cancel) {
            return; // User cancelled
        }
        // If Discard, just continue
    }
    
    // Show the new project dialog
    NewProjectDialog dialog(this);
    int result = dialog.exec();
    
    if (result == QDialog::Rejected) {
        return; // User cancelled
    } else if (result == 1) {
        // User chose to go to start screen - use QTimer to defer the call until after dialog cleanup
        QTimer::singleShot(0, this, &MainWindow::showStartScreen);
    } else if (result == 2) {
        // User chose quick create - create a default 64x64 project
        if (m_canvas) {
            m_canvas->resizeCanvas(DEFAULT_CANVAS_WIDTH, DEFAULT_CANVAS_HEIGHT);
            m_canvas->clearCanvas();
        } else {
            // If we don't have a canvas yet, switch to main interface first
            showMainInterface();
            m_canvas->resizeCanvas(DEFAULT_CANVAS_WIDTH, DEFAULT_CANVAS_HEIGHT);
            m_canvas->clearCanvas();
        }
        
        // Reset project state
        m_currentProject = ProjectData();
        m_currentProjectPath.clear();
        m_projectModified = false;
        
        // Clear history and save initial state
        if (m_historyManager) {
            m_historyManager->clearHistory();
            m_historyManager->saveState(m_canvas->getCanvasState(), "New Project");
        }
        
        // Update window title to show we have a new project
        setWindowTitle(QString("%1 - %2 - New Project").arg(APP_NAME, APP_DESCRIPTION));
    }
}

void MainWindow::saveProject()
{
    QString fileName;
    
    // If we have a current project path, use it for re-saving
    if (!m_currentProjectPath.isEmpty()) {
        fileName = SaveExportDialog::saveProject(this, QFileInfo(m_currentProjectPath).baseName(), m_currentProjectPath);
    } else {
        // New project, use regular save dialog
        fileName = SaveExportDialog::saveProject(this);
    }
    
    if (!fileName.isEmpty()) {
        // Create project data
        ProjectData projectData;
        projectData.canvasWidth = m_canvas->getCanvasWidth();
        projectData.canvasHeight = m_canvas->getCanvasHeight();
        projectData.pixelSize = m_canvas->getPixelSize();
        projectData.pixelData = m_canvas->canvasImage();
        projectData.projectName = QFileInfo(fileName).baseName();
        projectData.lastSavedPath = fileName;
        
        // Preserve creation timestamp if this is an existing project
        if (!m_currentProjectPath.isEmpty() && !m_currentProject.projectName.isEmpty()) {
            projectData.createdTimestamp = m_currentProject.createdTimestamp;
        } else {
            projectData.createdTimestamp = QDateTime::currentSecsSinceEpoch();
        }
        projectData.modifiedTimestamp = QDateTime::currentSecsSinceEpoch();
        
        if (PSPXFormat::saveProject(fileName, projectData)) {
            QMessageBox::information(this, "Success", "Project saved successfully!");
            m_projectModified = false;
            m_currentProjectPath = fileName;
            m_currentProject = projectData; // Update current project data
            addToRecentProjects(fileName);
            
            // Update window title
            setWindowTitle(QString("%1 - %2 - %3").arg(APP_NAME, APP_DESCRIPTION, projectData.projectName));
        } else {
            QMessageBox::warning(this, "Error", "Failed to save project!");
        }
    }
}

void MainWindow::exportImage()
{
    QString fileName = SaveExportDialog::exportImage(this);
    
    if (!fileName.isEmpty()) {
        if (m_canvas->saveImage(fileName)) {
            QMessageBox::information(this, "Success", "Image exported successfully!");
        } else {
            QMessageBox::warning(this, "Error", "Failed to export image!");
        }
    }
}

void MainWindow::exportAllLayers()
{
    if (!m_layerManager2 || m_layerManager2->layerCount() == 0) {
        QMessageBox::information(this, "Export Layers", "No layers to export.");
        return;
    }

    QString dir = QFileDialog::getExistingDirectory(this, "Export All Layers To Folder",
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));

    if (dir.isEmpty()) return;

    int exported = 0;
    for (int i = 0; i < m_layerManager2->layerCount(); ++i) {
        const Layer &layer = m_layerManager2->layerAt(i);
        QString safeName = layer.name;
        safeName.replace(QRegularExpression("[^a-zA-Z0-9_\\- ]"), "_");
        QString filePath = QString("%1/%2_%3.png").arg(dir).arg(i, 2, 10, QChar('0')).arg(safeName);
        if (layer.image.save(filePath)) {
            exported++;
        }
    }

    QMessageBox::information(this, "Export Layers",
        QString("Exported %1 of %2 layers to:\n%3").arg(exported).arg(m_layerManager2->layerCount()).arg(dir));
}

void MainWindow::loadProject()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        "Open Project", "AppData/Projects", "PixelPaint Project Files (*.pspx);;All Files (*)");
    
    if (!fileName.isEmpty()) {
        ProjectData projectData;
        if (PSPXFormat::loadProject(fileName, projectData)) {
            // Apply project data to canvas
            m_canvas->resizeCanvas(projectData.canvasWidth, projectData.canvasHeight);
            m_canvas->canvasImageRef() = projectData.pixelData;
            m_canvas->update();
            
            // Update project state
            m_currentProject = projectData;
            m_currentProjectPath = fileName;
            m_projectModified = false;
            
            // Update window title
            setWindowTitle(QString("%1 - %2 - %3").arg(APP_NAME, APP_DESCRIPTION, projectData.projectName));
            
            // Clear history and save initial state
            if (m_historyManager) {
                m_historyManager->clearHistory();
                m_historyManager->saveState(m_canvas->getCanvasState(), "Project Loaded");
            }
            
            QMessageBox::information(this, "Success", "Project loaded successfully!");
        } else {
            QMessageBox::warning(this, "Error", "Failed to load project!");
        }
    }
}

void MainWindow::resizeCanvas()
{
    bool ok;
    int width = QInputDialog::getInt(this, "Resize Canvas", "Canvas Width:", 
                                   m_canvas->getCanvasWidth(), MIN_CANVAS_SIZE, MAX_CANVAS_SIZE, 1, &ok);
    if (!ok) return;
    
    int height = QInputDialog::getInt(this, "Resize Canvas", "Canvas Height:", 
                                    m_canvas->getCanvasHeight(), MIN_CANVAS_SIZE, MAX_CANVAS_SIZE, 1, &ok);
    if (ok) {
        // Save state before resizing
        if (m_historyManager) {
            m_historyManager->saveState(m_canvas->getCanvasState(), "Resize Canvas");
        }
        
        m_canvas->resizeCanvas(width, height);
    }
}

// Zoom and Pan slot implementations
void MainWindow::zoomIn()
{
    if (m_canvas) {
        m_canvas->zoomIn();
        updateZoomBar();
    }
}

void MainWindow::zoomOut()
{
    if (m_canvas) {
        m_canvas->zoomOut();
        updateZoomBar();
    }
}

void MainWindow::resetZoom()
{
    if (m_canvas) {
        m_canvas->resetZoom();
        updateZoomBar();
    }
}

void MainWindow::resetPan()
{
    if (m_canvas) {
        m_canvas->resetPan();
    }
}

void MainWindow::fitToWindow()
{
    if (m_canvas) {
        // Calculate zoom factor to fit canvas in current window
        const int availableWidth = centralWidget()->width() - 40; // Some margin
        const int availableHeight = centralWidget()->height() - 150; // Account for toolbars
        
        const int canvasPixelWidth = m_canvas->getCanvasWidth() * m_canvas->getPixelSize();
        const int canvasPixelHeight = m_canvas->getCanvasHeight() * m_canvas->getPixelSize();
        
        const double zoomX = static_cast<double>(availableWidth) / canvasPixelWidth;
        const double zoomY = static_cast<double>(availableHeight) / canvasPixelHeight;
        
        const double fitZoom = qMin(zoomX, zoomY);
        
        m_canvas->setZoomFactor(fitZoom);
        m_canvas->resetPan();
        updateZoomBar();
    }
}

void MainWindow::updateZoomBar()
{
    if (m_canvas && m_zoomBar) {
        const double zoomFactor = m_canvas->getZoomFactor();
        m_zoomBar->setZoomLevel(static_cast<int>(zoomFactor * 100));
    }
}

void MainWindow::onZoomBarChanged(int value)
{
    double zoomFactor = static_cast<double>(value) / 100.0;
    if (m_canvas) {
        m_canvas->setZoomFactor(zoomFactor);
    }
}

bool MainWindow::shouldShowStartScreen()
{
    // Always show start screen on app startup
    return true;
}

void MainWindow::showStartScreen()
{
    if (!m_startScreen) {
        m_startScreen = new StartScreen();
        connect(m_startScreen, &StartScreen::projectSelected,
                this, &MainWindow::onStartScreenProjectSelected);
        connect(m_startScreen, &StartScreen::newProjectRequested,
                this, &MainWindow::onStartScreenNewProject);
    }

    setCentralWidget(m_startScreen);
}

void MainWindow::showMainInterface()
{
    if (!m_canvas) {
        setupUI();
        setupConnections();
    } else if (!m_centralWidget) {
        // Central widget was taken by setCentralWidget(startScreen), recreate UI
        m_canvas = nullptr;
        m_mainLayout = nullptr;
        m_statusBarLayout = nullptr;
        m_zoomBar = nullptr;
        m_toolPalette = nullptr;
        setupUI();
        setupConnections();
    }

    setCentralWidget(m_centralWidget);
}

void MainWindow::onStartScreenProjectSelected(const QString &filePath)
{
    ProjectData projectData;
    if (PSPXFormat::loadProject(filePath, projectData)) {
        showMainInterface();

        m_canvas->resizeCanvas(projectData.canvasWidth, projectData.canvasHeight);
        m_canvas->canvasImageRef() = projectData.pixelData;
        m_canvas->update();

        m_currentProject = projectData;
        m_currentProjectPath = filePath;
        m_projectModified = false;

        setWindowTitle(QString("%1 - %2 - %3").arg(APP_NAME, APP_DESCRIPTION, projectData.projectName));
        addToRecentProjects(filePath);

        if (m_historyManager) {
            m_historyManager->clearHistory();
            m_historyManager->saveState(m_canvas->getCanvasState(), "Project Loaded");
        }

        QMessageBox::information(this, "Success", "Project loaded successfully!");
    } else {
        QMessageBox::warning(this, "Error", "Failed to load project!");
    }
}

void MainWindow::onStartScreenNewProject(int width, int height, int pixelSize, const QColor &backgroundColor)
{
    Q_UNUSED(pixelSize)

    showMainInterface();

    m_canvas->resizeCanvas(width, height);

    // Fill canvas with the specified background color
    QImage bgImage(width, height, QImage::Format_ARGB32);
    bgImage.fill(backgroundColor);
    m_canvas->canvasImageRef() = bgImage;
    m_canvas->update();

    m_currentProject = ProjectData();
    m_currentProjectPath.clear();
    m_projectModified = false;

    if (m_historyManager) {
        m_historyManager->clearHistory();
        m_historyManager->saveState(m_canvas->getCanvasState(), "New Project");
    }
}

void MainWindow::addToRecentProjects(const QString &filePath)
{
    QSettings settings;
    QStringList recentFiles = settings.value("recentProjects").toStringList();
    
    // Remove if already exists
    recentFiles.removeAll(filePath);
    
    // Add to front
    recentFiles.prepend(filePath);
    
    // Limit to max recent projects
    const int maxRecent = 12;
    while (recentFiles.size() > maxRecent) {
        recentFiles.removeLast();
    }
    
    // Save back to settings
    settings.setValue("recentProjects", recentFiles);
}

void MainWindow::loadCustomColors()
{
    QSettings settings;
    int colorCount = settings.beginReadArray("mainWindowCustomColors");
    m_customColors.clear();
    
    for (int i = 0; i < colorCount; ++i) {
        settings.setArrayIndex(i);
        QColor color = settings.value("color").value<QColor>();
        if (color.isValid()) {
            m_customColors.append(color);
        }
    }
    settings.endArray();
}

void MainWindow::saveCustomColors()
{
    QSettings settings;
    settings.beginWriteArray("mainWindowCustomColors");
    
    for (int i = 0; i < m_customColors.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("color", m_customColors[i]);
    }
    settings.endArray();
}

void MainWindow::setupColorDialog(QColorDialog &dialog)
{
    dialog.setWindowTitle("Select Color");
    
    // Set custom colors - but only valid ones
    for (int i = 0; i < qMin(m_customColors.size(), 16); ++i) {
        if (m_customColors[i].isValid()) {
            dialog.setCustomColor(i, m_customColors[i]);
        }
    }
}

void MainWindow::undo()
{
    if (m_historyManager) {
        m_historyManager->undo();
    }
}

void MainWindow::redo()
{
    if (m_historyManager) {
        m_historyManager->redo();
    }
}

void MainWindow::flipHorizontal()
{
    if (!m_canvas) return;
    if (m_historyManager)
        m_historyManager->saveState(m_canvas->getCanvasState(), "Flip Horizontal");
    m_canvas->canvasImageRef() = m_canvas->canvasImage().mirrored(true, false);
    m_canvas->update();
}

void MainWindow::flipVertical()
{
    if (!m_canvas) return;
    if (m_historyManager)
        m_historyManager->saveState(m_canvas->getCanvasState(), "Flip Vertical");
    m_canvas->canvasImageRef() = m_canvas->canvasImage().mirrored(false, true);
    m_canvas->update();
}

void MainWindow::rotateCW()
{
    if (!m_canvas) return;
    if (m_historyManager)
        m_historyManager->saveState(m_canvas->getCanvasState(), "Rotate 90° CW");
    QTransform t;
    t.rotate(90);
    QImage rotated = m_canvas->canvasImage().transformed(t);
    m_canvas->resizeCanvas(rotated.width(), rotated.height());
    m_canvas->canvasImageRef() = rotated;
    m_canvas->update();
}

void MainWindow::rotateCCW()
{
    if (!m_canvas) return;
    if (m_historyManager)
        m_historyManager->saveState(m_canvas->getCanvasState(), "Rotate 90° CCW");
    QTransform t;
    t.rotate(-90);
    QImage rotated = m_canvas->canvasImage().transformed(t);
    m_canvas->resizeCanvas(rotated.width(), rotated.height());
    m_canvas->canvasImageRef() = rotated;
    m_canvas->update();
}

void MainWindow::copySelection()
{
    if (!m_canvas || !m_toolManager) return;
    BaseTool *tool = m_toolManager->getCurrentTool();
    if (!tool || tool->getType() != ToolType::Select) return;

    auto *selectTool = static_cast<SelectTool*>(tool);
    if (!selectTool->hasSelection()) return;

    QRect sel = selectTool->selectionRect();
    m_clipboard = m_canvas->canvasImage().copy(sel);
}

void MainWindow::pasteSelection()
{
    if (!m_canvas || m_clipboard.isNull()) return;

    // Switch to select tool
    if (m_toolManager) {
        m_toolManager->selectTool(ToolType::Select);
    }

    // Save state and paste at top-left
    if (m_historyManager)
        m_historyManager->saveState(m_canvas->getCanvasState(), "Paste");

    QPainter p(&m_canvas->canvasImageRef());
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);
    p.drawImage(0, 0, m_clipboard);
    p.end();
    m_canvas->update();
}

bool MainWindow::event(QEvent *event)
{
    // Handle dock widget events to prevent replacement
    if (event->type() == QEvent::ChildAdded || event->type() == QEvent::ChildRemoved) {
        // Ensure our dock widgets maintain proper spacing
        QTimer::singleShot(100, this, [this]() {
            if (m_toolPalette && m_historyPalette) {
                // Check if both widgets are in the same dock area
                Qt::DockWidgetArea toolArea = dockWidgetArea(m_toolPalette);
                Qt::DockWidgetArea historyArea = dockWidgetArea(m_historyPalette);
                
                if (toolArea == historyArea && toolArea != Qt::NoDockWidgetArea) {
                    // They're in the same area, ensure equal split
                    resizeDocks({m_toolPalette, m_historyPalette}, {200, 200}, Qt::Horizontal);
                }
            }
        });
    }
    
    return QMainWindow::event(event);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    // Accept drag events to allow custom handling
    event->acceptProposedAction();
    QMainWindow::dragEnterEvent(event);
}

void MainWindow::dropEvent(QDropEvent *event)
{
    // Let the base class handle the drop, then fix the layout
    QMainWindow::dropEvent(event);
    
    // After drop, ensure proper layout
    QTimer::singleShot(100, this, [this]() {
        if (m_toolPalette && m_historyPalette) {
            Qt::DockWidgetArea toolArea = dockWidgetArea(m_toolPalette);
            Qt::DockWidgetArea historyArea = dockWidgetArea(m_historyPalette);
            
            if (toolArea == historyArea && toolArea != Qt::NoDockWidgetArea) {
                // They're in the same area, ensure equal split
                resizeDocks({m_toolPalette, m_historyPalette}, {200, 200}, Qt::Horizontal);
            }
        }
    });
}



 