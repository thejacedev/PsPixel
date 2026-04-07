#include "toolmanager.h"
#include "basetool.h"
#include "brushtool.h"
#include "erasertool.h"
#include "eyedroppertool.h"
#include "paintbuckettool.h"
#include "linetool.h"
#include "rectangletool.h"
#include "circletool.h"
#include "selecttool.h"
#include "magicwandtool.h"
#include "lassotool.h"
#include "pixelcanvas.h"
#include <QLabel>

using namespace PixelPaint;

ToolManager::ToolManager(QObject *parent)
    : QObject(parent)
    , m_buttonGroup(new QButtonGroup(this))
    , m_toolbarLayout(nullptr)
    , m_currentTool(nullptr)
    , m_canvas(nullptr)
    , m_historyManager(nullptr)
    , m_currentColor(Qt::black)
    , m_brushSize(DEFAULT_BRUSH_SIZE)
{
    m_buttonGroup->setExclusive(true);
    initializeTools();
}

ToolManager::~ToolManager()
{
    // Clean up tools
    for (auto tool : m_tools) {
        delete tool;
    }
    m_tools.clear();
}

void ToolManager::initializeTools()
{
    // Create all tools
    m_tools[ToolType::Brush] = new BrushTool(this);
    m_tools[ToolType::Eraser] = new EraserTool(this);
    m_tools[ToolType::Eyedropper] = new EyedropperTool(this);
    m_tools[ToolType::PaintBucket] = new PaintBucketTool(this);
    m_tools[ToolType::Line] = new LineTool(this);
    m_tools[ToolType::Rectangle] = new RectangleTool(this);
    m_tools[ToolType::Circle] = new CircleTool(this);
    m_tools[ToolType::Select] = new SelectTool(this);
    m_tools[ToolType::MagicWand] = new MagicWandTool(this);
    m_tools[ToolType::Lasso] = new LassoTool(this);
    
    // Connect tool signals
    for (auto tool : m_tools) {
        connect(tool, &BaseTool::canvasModified, this, &ToolManager::onCanvasModified);
        
        // Special handling for eyedropper
        if (auto eyedropper = qobject_cast<EyedropperTool*>(tool)) {
            connect(eyedropper, &EyedropperTool::colorPicked, this, &ToolManager::onColorPicked);
        }
    }
    
    // Set default tool
    m_currentTool = m_tools[ToolType::Brush];
}

void ToolManager::setCanvas(PixelCanvas *canvas)
{
    m_canvas = canvas;
    
    // Update all tools with the new canvas
    for (auto tool : m_tools) {
        tool->setCanvas(canvas);
    }
}

void ToolManager::setHistoryManager(HistoryManager *historyManager)
{
    m_historyManager = historyManager;
}

void ToolManager::setCurrentColor(const QColor &color)
{
    m_currentColor = color;
    
    // Update all tools with the new color
    for (auto tool : m_tools) {
        tool->setColor(color);
    }
}

void ToolManager::setBrushSize(int size)
{
    m_brushSize = size;
    
    // Update all tools with the new brush size
    for (auto tool : m_tools) {
        tool->setBrushSize(size);
    }
}

ToolType ToolManager::getCurrentToolType() const
{
    return m_currentTool ? m_currentTool->getType() : ToolType::Brush;
}

QHBoxLayout* ToolManager::createToolbar()
{
    if (m_toolbarLayout) {
        return m_toolbarLayout;
    }
    
    m_toolbarLayout = new QHBoxLayout();
    m_toolbarLayout->setSpacing(TOOLBAR_SPACING);
    
    // Add title label
    QLabel *titleLabel = new QLabel("Tools:");
    titleLabel->setStyleSheet("font-weight: bold; color: palette(text);");
    m_toolbarLayout->addWidget(titleLabel);
    
    // Create tool buttons
    createToolButton(ToolType::Brush);
    createToolButton(ToolType::Eraser);
    createToolButton(ToolType::Eyedropper);
    createToolButton(ToolType::PaintBucket);
    createToolButton(ToolType::Line);
    
    // Add separator
    QLabel *separator = new QLabel("|");
    separator->setStyleSheet("color: palette(mid);");
    m_toolbarLayout->addWidget(separator);
    
    // Select default tool
    selectTool(ToolType::Brush);
    
    return m_toolbarLayout;
}

void ToolManager::selectTool(ToolType toolType)
{
    if (!m_tools.contains(toolType)) {
        return;
    }
    
    // Deactivate current tool
    if (m_currentTool) {
        // Clean up any incomplete history operations
        if (m_currentTool->isHistoryOperationInProgress()) {
            m_currentTool->cancelHistoryOperation();
        }
        m_currentTool->onDeactivate();
    }
    
    // Set new tool
    m_currentTool = m_tools[toolType];
    m_currentTool->onActivate();
    
    // Update tool properties
    m_currentTool->setCanvas(m_canvas);
    m_currentTool->setColor(m_currentColor);
    m_currentTool->setBrushSize(m_brushSize);
    
    // Update button state
    QPushButton *button = getToolButton(toolType);
    if (button) {
        button->setChecked(true);
    }
    
    emit toolChanged(toolType);
}

void ToolManager::onToolButtonClicked()
{
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if (!button) return;
    
    // Find which tool this button represents
    for (auto it = m_toolButtons.constBegin(); it != m_toolButtons.constEnd(); ++it) {
        if (it.value() == button) {
            selectTool(it.key());
            break;
        }
    }
}

void ToolManager::onColorPicked(const QColor &color)
{
    emit colorPicked(color);
}

void ToolManager::onCanvasModified()
{
    emit canvasModified();
}

void ToolManager::createToolButton(ToolType toolType)
{
    if (!m_tools.contains(toolType)) {
        return;
    }
    
    BaseTool *tool = m_tools[toolType];
    QPushButton *button = new QPushButton();
    
    // Set button properties
    button->setFixedSize(TOOL_BUTTON_SIZE, TOOL_BUTTON_SIZE);
    button->setCheckable(true);
    button->setToolTip(tool->getTooltip());
    button->setText(tool->getName().left(1)); // Use first letter as icon for now
    button->setStyleSheet(
        QString("QPushButton {"
        "    border: 1px solid palette(mid);"
        "    background-color: palette(button);"
        "    border-radius: %1px;"
        "}"
        "QPushButton:checked {"
        "    background-color: palette(highlight);"
        "    color: palette(highlighted-text);"
        "}"
        "QPushButton:hover {"
        "    background-color: palette(light);"
        "}").arg(RADIUS_CONTROL)
    );
    
    // Connect button
    connect(button, &QPushButton::clicked, this, &ToolManager::onToolButtonClicked);
    
    // Add to button group and layout
    m_buttonGroup->addButton(button);
    m_toolButtons[toolType] = button;
    m_toolbarLayout->addWidget(button);
}

QPushButton* ToolManager::getToolButton(ToolType toolType) const
{
    return m_toolButtons.value(toolType, nullptr);
} 