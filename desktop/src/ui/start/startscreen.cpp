#include "startscreen.h"
#include <QPainter>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QDir>
#include <QStandardPaths>
#include <QFileInfo>
#include <QApplication>
#include <QStyleOption>
#include <QIcon>
#include <QtMath>

using namespace PixelPaint;

// ProjectThumbnail Implementation
ProjectThumbnail::ProjectThumbnail(const RecentProject &project, QWidget *parent)
    : QFrame(parent)
    , m_project(project)
    , m_hovered(false)
{
    setFixedSize(StartScreen::THUMBNAIL_WIDTH, StartScreen::THUMBNAIL_HEIGHT);
    setFrameStyle(QFrame::Box);
    setStyleSheet("QFrame { border: 1px solid palette(mid); background-color: palette(base); }");
    setCursor(Qt::PointingHandCursor);
}

void ProjectThumbnail::paintEvent(QPaintEvent *event)
{
    QFrame::paintEvent(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw hover effect
    if (m_hovered) {
        painter.fillRect(rect(), QColor(0, 120, 215, 20));
    }
    
    // Draw thumbnail
    QRect thumbnailRect = rect().adjusted(10, 10, -10, -40);
    if (!m_project.thumbnail.isNull()) {
        // Draw a subtle border around the thumbnail
        painter.setPen(QPen(QColor(200, 200, 200), 1));
        painter.drawRect(thumbnailRect);
        
        // Draw the thumbnail using nearest neighbor scaling to keep pixels crisp
        QRect contentRect = thumbnailRect.adjusted(1, 1, -1, -1);
        QPixmap scaledThumbnail = m_project.thumbnail.scaled(
            contentRect.size(), Qt::KeepAspectRatio, Qt::FastTransformation);
        
        // Center the scaled thumbnail
        int offsetX = (contentRect.width() - scaledThumbnail.width()) / 2;
        int offsetY = (contentRect.height() - scaledThumbnail.height()) / 2;
        QRect drawRect = contentRect.adjusted(offsetX, offsetY, 
                                            offsetX - contentRect.width() + scaledThumbnail.width(),
                                            offsetY - contentRect.height() + scaledThumbnail.height());
        
        painter.drawPixmap(drawRect, scaledThumbnail);
    } else {
        painter.fillRect(thumbnailRect, QColor(240, 240, 240));
        painter.setPen(QColor(150, 150, 150));
        painter.drawText(thumbnailRect, Qt::AlignCenter, "No Preview");
    }
    
    // Draw project info
    QRect infoRect = rect().adjusted(10, rect().height() - 35, -10, -5);
    painter.setPen(palette().color(QPalette::WindowText));
    painter.drawText(infoRect, Qt::AlignLeft | Qt::AlignTop, m_project.name);
    
    QString infoText = QString("%1x%2 • %3px")
        .arg(m_project.canvasWidth)
        .arg(m_project.canvasHeight)
        .arg(m_project.pixelSize);
    painter.setPen(palette().color(QPalette::Text));
    painter.drawText(infoRect.adjusted(0, 12, 0, 0), Qt::AlignLeft | Qt::AlignTop, infoText);
}

void ProjectThumbnail::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit projectSelected(m_project.filePath);
    }
}

void ProjectThumbnail::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event)
    m_hovered = true;
    update();
}

void ProjectThumbnail::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    m_hovered = false;
    update();
}

// NewProjectPanel Implementation
NewProjectPanel::NewProjectPanel(QWidget *parent)
    : QFrame(parent)
    , m_backgroundColor(255, 255, 255, 255) // White by default instead of transparent
    , m_updatingFromPreset(false)
{
    setupUI();
    loadCustomColors();
    loadPresets();
    
    // Set initial background color button
    updateBackgroundColorButton();
}

void NewProjectPanel::setupUI()
{
    setFrameStyle(QFrame::Box);
    setStyleSheet("QFrame { border: 1px solid palette(mid); background-color: palette(window); }");
    
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(15);
    layout->setContentsMargins(20, 20, 20, 20);
    
    // Title
    QLabel *titleLabel = new QLabel("Create New Project");
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: palette(windowText);");
    layout->addWidget(titleLabel);
    
    // Presets
    QHBoxLayout *presetsLayout = new QHBoxLayout();
    presetsLayout->addWidget(new QLabel("Preset:"));
    m_presetsCombo = new QComboBox();
    m_presetsCombo->setMinimumWidth(150);
    presetsLayout->addWidget(m_presetsCombo);
    
    // Preset management buttons
    m_savePresetButton = new QPushButton(QIcon(":/assets/icons/save.png"), "");
    m_savePresetButton->setToolTip("Save current settings as preset");
    m_savePresetButton->setMaximumWidth(30);
    connect(m_savePresetButton, &QPushButton::clicked, this, &NewProjectPanel::onSavePreset);
    presetsLayout->addWidget(m_savePresetButton);
    
    m_deletePresetButton = new QPushButton(QIcon(":/assets/icons/trash.png"), "");
    m_deletePresetButton->setToolTip("Delete selected preset");
    m_deletePresetButton->setMaximumWidth(30);
    connect(m_deletePresetButton, &QPushButton::clicked, this, &NewProjectPanel::onDeletePreset);
    presetsLayout->addWidget(m_deletePresetButton);
    
    layout->addLayout(presetsLayout);
    
    // Canvas size
    QGridLayout *sizeLayout = new QGridLayout();
    
    sizeLayout->addWidget(new QLabel("Width:"), 0, 0);
    m_widthSpinBox = new QSpinBox();
    m_widthSpinBox->setRange(MIN_CANVAS_SIZE, MAX_CANVAS_SIZE);
    m_widthSpinBox->setValue(64);
    m_widthSpinBox->setSuffix(" px");
    sizeLayout->addWidget(m_widthSpinBox, 0, 1);
    
    sizeLayout->addWidget(new QLabel("Height:"), 1, 0);
    m_heightSpinBox = new QSpinBox();
    m_heightSpinBox->setRange(MIN_CANVAS_SIZE, MAX_CANVAS_SIZE);
    m_heightSpinBox->setValue(64);
    m_heightSpinBox->setSuffix(" px");
    sizeLayout->addWidget(m_heightSpinBox, 1, 1);
    
    sizeLayout->addWidget(new QLabel("Pixel Size:"), 2, 0);
    m_pixelSizeSpinBox = new QSpinBox();
    m_pixelSizeSpinBox->setRange(1, 20);
    m_pixelSizeSpinBox->setValue(8);
    m_pixelSizeSpinBox->setSuffix(" px");
    sizeLayout->addWidget(m_pixelSizeSpinBox, 2, 1);
    
    layout->addLayout(sizeLayout);
    
    // Background color
    QHBoxLayout *colorLayout = new QHBoxLayout();
    colorLayout->addWidget(new QLabel("Background:"));
    m_backgroundColorButton = new QPushButton("White");
    m_backgroundColorButton->setStyleSheet("QPushButton { text-align: left; padding: 5px; }");
    connect(m_backgroundColorButton, &QPushButton::clicked, this, &NewProjectPanel::onSelectBackgroundColor);
    colorLayout->addWidget(m_backgroundColorButton);
    
    // Add transparent button
    QPushButton *transparentButton = new QPushButton("Transparent");
    transparentButton->setToolTip("Make background transparent");
    transparentButton->setMaximumWidth(80);
    connect(transparentButton, &QPushButton::clicked, [this]() {
        m_backgroundColor = QColor(0, 0, 0, 0);
        updateBackgroundColorButton();
    });
    colorLayout->addWidget(transparentButton);
    
    layout->addLayout(colorLayout);
    
    layout->addStretch();
    
    // Create button
    m_createButton = new QPushButton("Create Project");
    m_createButton->setStyleSheet(
        "QPushButton { background-color: #0078d4; color: white; padding: 10px; "
        "border: none; border-radius: 3px; font-weight: bold; }"
        "QPushButton:hover { background-color: #106ebe; }"
        "QPushButton:pressed { background-color: #005a9e; }"
    );
    connect(m_createButton, &QPushButton::clicked, this, &NewProjectPanel::onCreateProject);
    layout->addWidget(m_createButton);
}

void NewProjectPanel::loadPresets()
{
    m_presets.clear();
    
    // Add built-in presets
    m_presets.append(ProjectPreset("Custom", 64, 64, 8, QColor(0, 0, 0, 0), true));
    m_presets.append(ProjectPreset("Small Icon (16x16)", 16, 16, 16, QColor(0, 0, 0, 0), true));
    m_presets.append(ProjectPreset("Icon (32x32)", 32, 32, 8, QColor(0, 0, 0, 0), true));
    m_presets.append(ProjectPreset("Large Icon (64x64)", 64, 64, 4, QColor(0, 0, 0, 0), true));
    m_presets.append(ProjectPreset("Sprite (128x128)", 128, 128, 2, QColor(0, 0, 0, 0), true));
    m_presets.append(ProjectPreset("Tile (16x16)", 16, 16, 12, QColor(0, 0, 0, 0), true));
    m_presets.append(ProjectPreset("Character (24x32)", 24, 32, 8, QColor(0, 0, 0, 0), true));
    
    // Add some presets with colored backgrounds
    m_presets.append(ProjectPreset("White Canvas (64x64)", 64, 64, 8, QColor(255, 255, 255), true));
    m_presets.append(ProjectPreset("Black Canvas (64x64)", 64, 64, 8, QColor(0, 0, 0), true));
    m_presets.append(ProjectPreset("Game Sprite (32x32)", 32, 32, 8, QColor(255, 255, 255), true));
    
    // Load custom presets from settings
    QSettings settings;
    int customPresetCount = settings.beginReadArray("customPresets");
    for (int i = 0; i < customPresetCount; ++i) {
        settings.setArrayIndex(i);
        ProjectPreset preset;
        preset.name = settings.value("name").toString();
        preset.width = settings.value("width", 64).toInt();
        preset.height = settings.value("height", 64).toInt();
        preset.pixelSize = settings.value("pixelSize", 8).toInt();
        preset.backgroundColor = settings.value("backgroundColor", QColor(0, 0, 0, 0)).value<QColor>();
        preset.isBuiltIn = false;
        m_presets.append(preset);
    }
    settings.endArray();
    
    updatePresetCombo();
    
    // Connect preset selection
    connect(m_presetsCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &NewProjectPanel::onPresetChanged);
}

void NewProjectPanel::onSelectBackgroundColor()
{
    QColorDialog dialog(this);
    setupColorDialog(dialog);
    
    // If current background is transparent, start with white for color selection
    QColor initialColor = m_backgroundColor;
    if (initialColor.alpha() == 0) {
        initialColor = QColor(255, 255, 255, 255); // White
    }
    dialog.setCurrentColor(initialColor);
    
    if (dialog.exec() == QDialog::Accepted) {
        m_backgroundColor = dialog.currentColor();
        
        // Add to custom colors if not already there and not transparent
        if (!m_customColors.contains(m_backgroundColor) && m_backgroundColor.alpha() > 0) {
            m_customColors.prepend(m_backgroundColor);
            if (m_customColors.size() > 16) { // Limit to 16 custom colors
                m_customColors.removeLast();
            }
            saveCustomColors();
        }
        
        // Also get custom colors from the dialog itself in case user picked from the custom area
        for (int i = 0; i < 16; ++i) {
            QColor customColor = dialog.customColor(i);
            if (customColor.isValid() && customColor.alpha() > 0 && !m_customColors.contains(customColor)) {
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
        
        updateBackgroundColorButton();
    }
}

void NewProjectPanel::onCreateProject()
{
    emit createNewProject(
        m_widthSpinBox->value(),
        m_heightSpinBox->value(),
        m_pixelSizeSpinBox->value(),
        m_backgroundColor
    );
}

void NewProjectPanel::onPresetChanged(int index)
{
    if (index >= 0 && index < m_presets.size()) {
        const ProjectPreset &preset = m_presets[index];
        
        m_updatingFromPreset = true;
        
        // Always update dimensions and pixel size
        m_widthSpinBox->setValue(preset.width);
        m_heightSpinBox->setValue(preset.height);
        m_pixelSizeSpinBox->setValue(preset.pixelSize);
        
        // For non-Custom presets, update the background color
        if (preset.name != "Custom") {
            m_backgroundColor = preset.backgroundColor;
            updateBackgroundColorButton();
        }
        // For "Custom" preset, keep whatever background color the user has selected
        
        m_updatingFromPreset = false;
        
        // Update delete button state
        m_deletePresetButton->setEnabled(!preset.isBuiltIn);
    }
}

void NewProjectPanel::onSavePreset()
{
    bool ok;
    QString name = QInputDialog::getText(this, "Save Preset", 
                                       "Preset name:", QLineEdit::Normal, "", &ok);
    
    if (ok && !name.trimmed().isEmpty()) {
        name = name.trimmed();
        
        // Check if preset with this name already exists
        bool exists = false;
        for (int i = 0; i < m_presets.size(); ++i) {
            if (m_presets[i].name == name) {
                if (m_presets[i].isBuiltIn) {
                    QMessageBox::warning(this, "Cannot Overwrite", 
                        "Cannot overwrite built-in presets. Please choose a different name.");
                    return;
                }
                
                int ret = QMessageBox::question(this, "Preset Exists", 
                    QString("Preset '%1' already exists. Overwrite?").arg(name),
                    QMessageBox::Yes | QMessageBox::No);
                
                if (ret == QMessageBox::Yes) {
                    // Update existing preset
                    m_presets[i].width = m_widthSpinBox->value();
                    m_presets[i].height = m_heightSpinBox->value();
                    m_presets[i].pixelSize = m_pixelSizeSpinBox->value();
                    m_presets[i].backgroundColor = m_backgroundColor;
                    exists = true;
                } else {
                    return;
                }
                break;
            }
        }
        
        if (!exists) {
            // Add new preset
            ProjectPreset newPreset(name, 
                                  m_widthSpinBox->value(),
                                  m_heightSpinBox->value(),
                                  m_pixelSizeSpinBox->value(),
                                  m_backgroundColor,
                                  false);
            m_presets.append(newPreset);
        }
        
        savePresets();
        updatePresetCombo();
        
        // Select the newly saved preset
        for (int i = 0; i < m_presets.size(); ++i) {
            if (m_presets[i].name == name) {
                m_presetsCombo->setCurrentIndex(i);
                break;
            }
        }
    }
}

void NewProjectPanel::onDeletePreset()
{
    int currentIndex = m_presetsCombo->currentIndex();
    if (currentIndex >= 0 && currentIndex < m_presets.size()) {
        const ProjectPreset &preset = m_presets[currentIndex];
        
        if (preset.isBuiltIn) {
            QMessageBox::information(this, "Cannot Delete", "Cannot delete built-in presets.");
            return;
        }
        
        int ret = QMessageBox::question(this, "Delete Preset", 
            QString("Delete preset '%1'?").arg(preset.name),
            QMessageBox::Yes | QMessageBox::No);
        
        if (ret == QMessageBox::Yes) {
            m_presets.removeAt(currentIndex);
            savePresets();
            updatePresetCombo();
            m_presetsCombo->setCurrentIndex(0); // Select "Custom"
        }
    }
}

void NewProjectPanel::updatePresetCombo()
{
    m_presetsCombo->clear();
    
    for (const ProjectPreset &preset : m_presets) {
        QString displayName = preset.name;
        if (!preset.isBuiltIn) {
            displayName += " *"; // Custom preset indicator
        }
        m_presetsCombo->addItem(displayName);
    }
    
    // Update delete button state
    if (m_presetsCombo->currentIndex() >= 0 && m_presetsCombo->currentIndex() < m_presets.size()) {
        m_deletePresetButton->setEnabled(!m_presets[m_presetsCombo->currentIndex()].isBuiltIn);
    }
}

void NewProjectPanel::savePresets()
{
    QSettings settings;
    
    // Save only custom presets
    QVector<ProjectPreset> customPresets;
    for (const ProjectPreset &preset : m_presets) {
        if (!preset.isBuiltIn) {
            customPresets.append(preset);
        }
    }
    
    settings.beginWriteArray("customPresets");
    for (int i = 0; i < customPresets.size(); ++i) {
        settings.setArrayIndex(i);
        const ProjectPreset &preset = customPresets[i];
        settings.setValue("name", preset.name);
        settings.setValue("width", preset.width);
        settings.setValue("height", preset.height);
        settings.setValue("pixelSize", preset.pixelSize);
        settings.setValue("backgroundColor", preset.backgroundColor);
    }
    settings.endArray();
}

void NewProjectPanel::loadCustomColors()
{
    QSettings settings;
    int colorCount = settings.beginReadArray("customColors");
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

void NewProjectPanel::saveCustomColors()
{
    QSettings settings;
    settings.beginWriteArray("customColors");
    
    for (int i = 0; i < m_customColors.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("color", m_customColors[i]);
    }
    settings.endArray();
}

void NewProjectPanel::setupColorDialog(QColorDialog &dialog)
{
    dialog.setOption(QColorDialog::ShowAlphaChannel, true);
    
    // Set custom colors - but only valid ones
    for (int i = 0; i < qMin(m_customColors.size(), 16); ++i) {
        if (m_customColors[i].isValid()) {
            dialog.setCustomColor(i, m_customColors[i]);
        }
    }
}

void NewProjectPanel::updateBackgroundColorButton()
{
    QString buttonText;
    if (m_backgroundColor.alpha() == 0) {
        buttonText = "Transparent";
        m_backgroundColorButton->setStyleSheet("QPushButton { text-align: left; padding: 5px; }");
    } else {
        buttonText = m_backgroundColor.name();
        m_backgroundColorButton->setStyleSheet(QString(
            "QPushButton { text-align: left; padding: 5px; background-color: %1; }")
            .arg(m_backgroundColor.name()));
    }
    m_backgroundColorButton->setText(buttonText);
}

// StartScreen Implementation
StartScreen::StartScreen(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(600, 400);
    setupUI();
    loadRecentProjects();
}

void StartScreen::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setSpacing(20);
    m_mainLayout->setContentsMargins(40, 40, 40, 40);

    // Header
    QHBoxLayout *headerLayout = new QHBoxLayout();
    m_titleLabel = new QLabel("PixelPaint");
    m_titleLabel->setStyleSheet("font-size: 28px; font-weight: bold; color: palette(windowText);");
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch();
    m_mainLayout->addLayout(headerLayout);

    // Content layout
    QHBoxLayout *contentLayout = new QHBoxLayout();

    // Recent projects section
    QVBoxLayout *recentSection = new QVBoxLayout();
    m_recentLabel = new QLabel("Recent Projects");
    m_recentLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: palette(windowText); margin-bottom: 10px;");
    recentSection->addWidget(m_recentLabel);

    m_recentProjectsArea = new QScrollArea();
    m_recentProjectsArea->setWidgetResizable(true);
    m_recentProjectsArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_recentProjectsArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_recentProjectsArea->setMinimumHeight(300);

    QWidget *projectsWidget = new QWidget();
    m_projectsGrid = new QGridLayout(projectsWidget);
    m_projectsGrid->setSpacing(15);
    m_recentProjectsArea->setWidget(projectsWidget);

    recentSection->addWidget(m_recentProjectsArea);
    contentLayout->addLayout(recentSection, 2);

    // New project panel
    m_newProjectPanel = new NewProjectPanel();
    m_newProjectPanel->setFixedWidth(300);
    connect(m_newProjectPanel, &NewProjectPanel::createNewProject,
            this, &StartScreen::onNewProjectClicked);
    contentLayout->addWidget(m_newProjectPanel, 0);

    m_mainLayout->addLayout(contentLayout);
}

void StartScreen::loadRecentProjects()
{
    QSettings settings;
    QStringList recentFiles = settings.value("recentProjects").toStringList();

    m_recentProjects.clear();

    for (const QString &filePath : recentFiles) {
        if (!QFile::exists(filePath)) continue;

        ProjectData projectData;
        if (!PSPXFormat::loadProject(filePath, projectData)) continue;

        if (projectData.canvasWidth <= 0 || projectData.canvasHeight <= 0 ||
            projectData.pixelSize <= 0 || projectData.pixelData.isNull()) {
            continue;
        }

        RecentProject recent;
        recent.name = QFileInfo(filePath).baseName();
        recent.filePath = filePath;
        recent.lastModified = QFileInfo(filePath).lastModified();
        recent.canvasWidth = projectData.canvasWidth;
        recent.canvasHeight = projectData.canvasHeight;
        recent.pixelSize = projectData.pixelSize;
        recent.thumbnail = generateThumbnail(projectData, QSize(THUMBNAIL_WIDTH - 20, THUMBNAIL_HEIGHT - 50));

        m_recentProjects.append(recent);
    }

    refreshRecentProjects();
}

void StartScreen::refreshRecentProjects()
{
    // Clear existing thumbnails
    QLayoutItem *item;
    while ((item = m_projectsGrid->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    
    // Add recent project thumbnails
    int row = 0, col = 0;
    const int maxCols = 3;
    
    for (const RecentProject &project : m_recentProjects) {
        ProjectThumbnail *thumbnail = new ProjectThumbnail(project);
        connect(thumbnail, &ProjectThumbnail::projectSelected,
                this, &StartScreen::onProjectThumbnailClicked);
        
        m_projectsGrid->addWidget(thumbnail, row, col);
        
        col++;
        if (col >= maxCols) {
            col = 0;
            row++;
        }
    }
    
    // Show message if no recent projects
    if (m_recentProjects.isEmpty()) {
        QLabel *noProjectsLabel = new QLabel("No recent projects.\nCreate a new project to get started!");
        noProjectsLabel->setAlignment(Qt::AlignCenter);
        noProjectsLabel->setStyleSheet("color: palette(windowText); font-size: 14px;");
        m_projectsGrid->addWidget(noProjectsLabel, 0, 0, 1, maxCols);
    }
}

QPixmap StartScreen::generateThumbnail(const ProjectData &projectData, const QSize &size)
{
    if (projectData.pixelData.isNull() ||
        projectData.canvasWidth <= 0 || projectData.canvasHeight <= 0 ||
        size.width() <= 0 || size.height() <= 0) {
        return QPixmap();
    }

    // Create checkerboard + canvas composite
    QImage composite(projectData.canvasWidth, projectData.canvasHeight, QImage::Format_ARGB32);

    // Draw checkerboard
    const int checkSize = qMax(1, projectData.canvasWidth / 32);
    for (int y = 0; y < projectData.canvasHeight; ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(composite.scanLine(y));
        for (int x = 0; x < projectData.canvasWidth; ++x) {
            bool even = ((x / checkSize) + (y / checkSize)) % 2 == 0;
            line[x] = even ? CHECKERBOARD_LIGHT.rgba() : CHECKERBOARD_DARK.rgba();
        }
    }

    // Draw canvas pixels on top
    QPainter painter(&composite);
    painter.drawImage(0, 0, projectData.pixelData);
    painter.end();

    // Scale to thumbnail size with nearest-neighbor
    QPixmap result = QPixmap::fromImage(composite.scaled(size, Qt::KeepAspectRatio, Qt::FastTransformation));
    return result;
}

void StartScreen::onProjectThumbnailClicked(const QString &filePath)
{
    emit projectSelected(filePath);
}

void StartScreen::onNewProjectClicked(int width, int height, int pixelSize, const QColor &backgroundColor)
{
    emit newProjectRequested(width, height, pixelSize, backgroundColor);
}