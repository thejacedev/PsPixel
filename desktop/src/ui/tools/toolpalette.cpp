#include "toolpalette.h"
#include "toolmanager.h"
#include "basetool.h"
#include <QIcon>
#include <QSplitter>

using namespace PixelPaint;

ToolPalette::ToolPalette(ToolManager *toolManager, QWidget *parent)
    : QDockWidget("Tools", parent)
    , m_toolManager(toolManager)
    , m_buttonGroup(new QButtonGroup(this))
    , m_currentColor(Qt::black)
    , m_currentBrushSize(DEFAULT_BRUSH_SIZE)
    , m_currentToolType(ToolType::Brush)
{
    setObjectName("ToolPalette");
    
    // Allow the dock widget to be docked in multiple areas
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    
    // Set features to allow floating, moving, and closing
    setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
    
    // Set size hints for better docking behavior
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    
    // Prevent this dock widget from being tabbed over
    setAttribute(Qt::WA_DeleteOnClose, false);
    
    m_buttonGroup->setExclusive(true);
    setupUI();
}

QSize ToolPalette::sizeHint() const
{
    // Return a reasonable size hint for dock splitting
    return QSize(200, 400);
}

void ToolPalette::setupUI()
{
    // Create main content widget
    m_contentWidget = new QWidget();
    setWidget(m_contentWidget);
    
    m_mainLayout = new QVBoxLayout(m_contentWidget);
    m_mainLayout->setSpacing(SPACING_MD);
    m_mainLayout->setContentsMargins(SPACING_SM, SPACING_SM, SPACING_SM, SPACING_SM);
    
    // Create sections
    createToolButtons();
    createBrushSizeControls();
    createColorControls();
    
    // Add stretch at the bottom
    m_mainLayout->addStretch();
    
    // Set initial size
    setMinimumWidth(120);
    setMaximumWidth(200);
    resize(150, 400);
}

void ToolPalette::createToolButtons()
{
    // Tools section
    m_toolsFrame = new QFrame();
    m_toolsFrame->setFrameStyle(QFrame::StyledPanel);
    m_toolsFrame->setStyleSheet(
        QString("QFrame {"
        "    background-color: palette(window);"
        "    border: 1px solid palette(mid);"
        "    border-radius: %1px;"
        "    padding: %2px;"
        "}").arg(RADIUS_PANEL).arg(SPACING_XS)
    );

    QVBoxLayout *toolsFrameLayout = new QVBoxLayout(m_toolsFrame);
    toolsFrameLayout->setSpacing(SPACING_XS);
    toolsFrameLayout->setContentsMargins(SPACING_XS, SPACING_XS, SPACING_XS, SPACING_XS);

    // Tools title
    QLabel *toolsTitle = new QLabel("Tools");
    toolsTitle->setStyleSheet(QString("font-weight: bold; font-size: %1px; color: palette(text); border: none;").arg(FONT_SIZE_BODY));
    toolsTitle->setAlignment(Qt::AlignCenter);
    toolsFrameLayout->addWidget(toolsTitle);
    
    // Tools grid (2 columns)
    QWidget *toolsGridWidget = new QWidget();
    m_toolsLayout = new QGridLayout(toolsGridWidget);
    m_toolsLayout->setSpacing(3);
    m_toolsLayout->setContentsMargins(0, 0, 0, 0);
    
    // Create tool buttons
    if (m_toolManager) {
        createToolButton(ToolType::Select);
        createToolButton(ToolType::MagicWand);
        createToolButton(ToolType::Lasso);
        createToolButton(ToolType::Brush);
        createToolButton(ToolType::Eraser);
        createToolButton(ToolType::Eyedropper);
        createToolButton(ToolType::PaintBucket);
        createToolButton(ToolType::Line);
        createToolButton(ToolType::Rectangle);
        createToolButton(ToolType::Circle);
        
        // Select default tool
        if (m_toolButtons.contains(ToolType::Brush)) {
            m_toolButtons[ToolType::Brush]->setChecked(true);
        }
    }
    
    toolsFrameLayout->addWidget(toolsGridWidget);
    m_mainLayout->addWidget(m_toolsFrame);
}

void ToolPalette::createBrushSizeControls()
{
    // Brush section
    m_brushFrame = new QFrame();
    m_brushFrame->setFrameStyle(QFrame::StyledPanel);
    m_brushFrame->setStyleSheet(
        QString("QFrame {"
        "    background-color: palette(window);"
        "    border: 1px solid palette(mid);"
        "    border-radius: %1px;"
        "    padding: %2px;"
        "}").arg(RADIUS_PANEL).arg(SPACING_XS)
    );

    m_brushLayout = new QVBoxLayout(m_brushFrame);
    m_brushLayout->setSpacing(SPACING_XS);
    m_brushLayout->setContentsMargins(SPACING_XS, SPACING_XS, SPACING_XS, SPACING_XS);

    // Brush title
    QLabel *brushTitle = new QLabel("Brush");
    brushTitle->setStyleSheet(QString("font-weight: bold; font-size: %1px; color: palette(text); border: none;").arg(FONT_SIZE_BODY));
    brushTitle->setAlignment(Qt::AlignCenter);
    m_brushLayout->addWidget(brushTitle);
    
    // Brush size
    m_brushSizeLabel = new QLabel("Size:");
    m_brushSizeLabel->setStyleSheet(QString("border: none; color: palette(text); font-size: %1px;").arg(FONT_SIZE_BODY));
    m_brushLayout->addWidget(m_brushSizeLabel);
    
    m_brushSizeSpinBox = new BrushSizeSpinBox();
    m_brushSizeSpinBox->setRange(MIN_BRUSH_SIZE, MAX_BRUSH_SIZE);
    m_brushSizeSpinBox->setValue(DEFAULT_BRUSH_SIZE);
    m_brushSizeSpinBox->setToolTip(QString("Brush Size: %1 pixels\n\nAffects: Brush, Eraser, and Line tools\nRange: %2 - %3 pixels\n\n• Click arrow buttons to change\n• Scroll wheel to adjust quickly\n• Click number to type directly")
                                   .arg(DEFAULT_BRUSH_SIZE).arg(MIN_BRUSH_SIZE).arg(MAX_BRUSH_SIZE));
    
    // Make the spinbox more responsive to arrow buttons and less prone to text editing issues
    m_brushSizeSpinBox->setButtonSymbols(QAbstractSpinBox::UpDownArrows);
    m_brushSizeSpinBox->setAccelerated(true); // Faster increment when holding buttons
    m_brushSizeSpinBox->setKeyboardTracking(false); // Only emit signal when editing is finished
    m_brushSizeSpinBox->setFocusPolicy(Qt::StrongFocus);
    m_brushSizeSpinBox->setStyleSheet(
        QString("QSpinBox {"
        "    border: 1px solid palette(mid);"
        "    border-radius: %1px;"
        "    padding: 2px;").arg(RADIUS_CONTROL) +
        "    selection-background-color: palette(highlight);"
        "}"
        "QSpinBox:focus {"
        "    border: 2px solid palette(highlight);"
        "}"
        "QSpinBox::up-button {"
        "    subcontrol-origin: border;"
        "    subcontrol-position: top right;"
        "    width: 16px;"
        "    border-left: 1px solid palette(mid);"
        "    border-bottom: 1px solid palette(mid);"
        "    border-top-right-radius: 3px;"
        "    background-color: palette(button);"
        "}"
        "QSpinBox::up-button:hover {"
        "    background-color: palette(light);"
        "}"
        "QSpinBox::up-button:pressed {"
        "    background-color: palette(dark);"
        "}"
        "QSpinBox::down-button {"
        "    subcontrol-origin: border;"
        "    subcontrol-position: bottom right;"
        "    width: 16px;"
        "    border-left: 1px solid palette(mid);"
        "    border-top: 1px solid palette(mid);"
        "    border-bottom-right-radius: 3px;"
        "    background-color: palette(button);"
        "}"
        "QSpinBox::down-button:hover {"
        "    background-color: palette(light);"
        "}"
        "QSpinBox::down-button:pressed {"
        "    background-color: palette(dark);"
        "}"
        "QSpinBox::up-arrow {"
        "    image: none;"
        "    width: 7px;"
        "    height: 7px;"
        "    border: 2px solid palette(text);"
        "    border-bottom: none;"
        "    border-left: none;"
        "}"
        "QSpinBox::down-arrow {"
        "    image: none;"
        "    width: 7px;"
        "    height: 7px;"
        "    border: 2px solid palette(text);"
        "    border-top: none;"
        "    border-right: none;"
        "}"
    );
    
    connect(m_brushSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ToolPalette::onBrushSizeChanged);
    m_brushLayout->addWidget(m_brushSizeSpinBox);
    
    m_mainLayout->addWidget(m_brushFrame);
}

void ToolPalette::createColorControls()
{
    // Color section
    m_colorFrame = new QFrame();
    m_colorFrame->setFrameStyle(QFrame::StyledPanel);
    m_colorFrame->setStyleSheet(
        QString("QFrame {"
        "    background-color: palette(window);"
        "    border: 1px solid palette(mid);"
        "    border-radius: %1px;"
        "    padding: %2px;"
        "}").arg(RADIUS_PANEL).arg(SPACING_XS)
    );

    m_colorLayout = new QVBoxLayout(m_colorFrame);
    m_colorLayout->setSpacing(SPACING_XS);
    m_colorLayout->setContentsMargins(SPACING_XS, SPACING_XS, SPACING_XS, SPACING_XS);

    // Color title
    QLabel *colorTitle = new QLabel("Color");
    colorTitle->setStyleSheet(QString("font-weight: bold; font-size: %1px; color: palette(text); border: none;").arg(FONT_SIZE_BODY));
    colorTitle->setAlignment(Qt::AlignCenter);
    m_colorLayout->addWidget(colorTitle);
    
    // Color button — shows current color as a swatch with hex code
    m_colorButton = new QPushButton();
    m_colorButton->setFixedHeight(TOOL_BUTTON_SIZE);
    m_colorButton->setCursor(Qt::PointingHandCursor);
    updateColorButtonAppearance(m_currentColor);
    connect(m_colorButton, &QPushButton::clicked, this, &ToolPalette::onColorButtonClicked);
    m_colorLayout->addWidget(m_colorButton);
    
    m_mainLayout->addWidget(m_colorFrame);
}

void ToolPalette::createToolButton(ToolType toolType)
{
    if (!m_toolManager || !m_toolManager->getCurrentTool()) {
        return;
    }
    
    // Find the tool to get its info
    BaseTool *tool = nullptr;
    for (auto it = m_toolManager->m_tools.constBegin(); it != m_toolManager->m_tools.constEnd(); ++it) {
        if (it.key() == toolType) {
            tool = it.value();
            break;
        }
    }
    
    if (!tool) return;
    
    QPushButton *button = new QPushButton();
    button->setFixedSize(TOOL_BUTTON_SIZE, TOOL_BUTTON_SIZE);
    button->setCheckable(true);

    // Set icon and tooltip based on tool type
    QString toolName = tool->getName();
    QString tooltip = QString("%1\n\n%2").arg(toolName, tool->getTooltip());

    QString iconPath = tool->getIconPath();
    if (!iconPath.isEmpty()) {
        button->setIcon(QIcon(iconPath));
        button->setIconSize(QSize(20, 20));
    } else {
        button->setText(toolName.left(1));
    }
    button->setToolTip(tooltip);
    button->setStyleSheet(
        QString("QPushButton {"
        "    border: 1px solid palette(mid);"
        "    background-color: palette(button);"
        "    border-radius: %1px;"
        "    font-size: %2px;"
        "    font-weight: normal;"
        "}"
        "QPushButton:checked {"
        "    background-color: palette(highlight);"
        "    color: palette(highlighted-text);"
        "    border: 2px solid palette(highlight);"
        "}"
        "QPushButton:hover {"
        "    background-color: palette(light);"
        "    border: 2px solid palette(highlight);"
        "}"
        "QPushButton:pressed {"
        "    background-color: palette(dark);"
        "}").arg(RADIUS_CONTROL).arg(FONT_SIZE_TITLE)
    );
    
    // Store tool type as property
    button->setProperty("toolType", static_cast<int>(toolType));
    
    connect(button, &QPushButton::clicked, this, &ToolPalette::onToolButtonClicked);
    
    // Add to button group and map
    m_buttonGroup->addButton(button);
    m_toolButtons[toolType] = button;
    
    // Add to grid layout (2 columns)
    int count = m_toolButtons.size() - 1;
    int row = count / 2;
    int col = count % 2;
    m_toolsLayout->addWidget(button, row, col);
}

void ToolPalette::updateCurrentTool()
{
    if (!m_toolManager) return;
    
    ToolType currentType = m_toolManager->getCurrentToolType();
    m_currentToolType = currentType;
    
    // Update button selection
    if (m_toolButtons.contains(currentType)) {
        m_toolButtons[currentType]->setChecked(true);
    }
}

void ToolPalette::updateBrushSize(int size)
{
    m_currentBrushSize = size;
    if (m_brushSizeSpinBox) {
        m_brushSizeSpinBox->blockSignals(true);
        m_brushSizeSpinBox->setValue(size);
        m_brushSizeSpinBox->setToolTip(QString("Brush Size: %1 pixels\n\nAffects: Brush, Eraser, and Line tools\nRange: %2 - %3 pixels\n\n• Click arrow buttons to change\n• Scroll wheel to adjust quickly\n• Click number to type directly")
                                       .arg(size).arg(MIN_BRUSH_SIZE).arg(MAX_BRUSH_SIZE));
        m_brushSizeSpinBox->blockSignals(false);
    }
}

void ToolPalette::updateCurrentColor(const QColor &color)
{
    m_currentColor = color;
    if (m_colorButton) {
        updateColorButtonAppearance(color);
    }
}

void ToolPalette::updateColorButtonAppearance(const QColor &color)
{
    // Choose text color for readability: white on dark colors, dark on light
    double luminance = 0.299 * color.redF() + 0.587 * color.greenF() + 0.114 * color.blueF();
    QString textColor = luminance > 0.5 ? "#222222" : "#ffffff";
    QString shadowColor = luminance > 0.5 ? "rgba(255,255,255,0.4)" : "rgba(0,0,0,0.5)";

    m_colorButton->setText(color.name().toUpper());
    m_colorButton->setToolTip(QString("Current Color: %1\nRGB(%2, %3, %4)\n\nClick to open color picker")
        .arg(color.name().toUpper())
        .arg(color.red()).arg(color.green()).arg(color.blue()));
    m_colorButton->setStyleSheet(
        QString("QPushButton {"
                "    background-color: %1;"
                "    border: 2px solid palette(mid);"
                "    border-radius: %2px;"
                "    color: %3;"
                "    font-size: %4px;"
                "    font-weight: bold;"
                "    font-family: monospace;"
                "    text-shadow: 1px 1px 2px %5;"
                "}"
                "QPushButton:hover {"
                "    border: 2px solid palette(highlight);"
                "    background-color: %1;"
                "}"
                "QPushButton:pressed {"
                "    border: 3px solid palette(highlight);"
                "}")
        .arg(color.name())
        .arg(RADIUS_CONTROL)
        .arg(textColor)
        .arg(FONT_SIZE_BODY)
        .arg(shadowColor)
    );
}

void ToolPalette::onToolButtonClicked()
{
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if (!button) return;
    
    bool ok;
    int toolTypeInt = button->property("toolType").toInt(&ok);
    if (ok) {
        ToolType toolType = static_cast<ToolType>(toolTypeInt);
        m_currentToolType = toolType;
        emit toolSelected(toolType);
    }
}

void ToolPalette::onBrushSizeChanged(int size)
{
    m_currentBrushSize = size;
    if (m_brushSizeSpinBox) {
        m_brushSizeSpinBox->setToolTip(QString("Brush Size: %1 pixels\n\nAffects: Brush, Eraser, and Line tools\nRange: %2 - %3 pixels\n\n• Click arrow buttons to change\n• Scroll wheel to adjust quickly\n• Click number to type directly")
                                       .arg(size).arg(MIN_BRUSH_SIZE).arg(MAX_BRUSH_SIZE));
    }
    emit brushSizeChanged(size);
}

void ToolPalette::onColorButtonClicked()
{
    emit colorButtonClicked();
} 