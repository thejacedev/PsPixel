#include "saveexportdialog.h"
#include "constants.h"
#include <QIcon>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>

using namespace PixelPaint;

// Static members
QString SaveExportDialog::s_lastProjectPath;
QString SaveExportDialog::s_lastExportPath;

SaveExportDialog::SaveExportDialog(QWidget *parent)
    : QDialog(parent)
    , m_saveProjectButton(nullptr)
    , m_exportImageButton(nullptr)
    , m_cancelButton(nullptr)
    , m_projectNameEdit(nullptr)
    , m_infoLabel(nullptr)
{
    setupUI();
    setWindowTitle("Save or Export");
    setModal(true);
    resize(400, 200);
}

void SaveExportDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Info label
    m_infoLabel = new QLabel("Choose how to save your work:");
    m_infoLabel->setWordWrap(true);
    mainLayout->addWidget(m_infoLabel);
    
    // Project name input
    QHBoxLayout *nameLayout = new QHBoxLayout();
    nameLayout->addWidget(new QLabel("Project name:"));
    m_projectNameEdit = new QLineEdit();
    m_projectNameEdit->setText(QString("Untitled_%1").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")));
    nameLayout->addWidget(m_projectNameEdit);
    mainLayout->addLayout(nameLayout);
    
    mainLayout->addSpacing(SPACING_LG);

    // Save project button
    m_saveProjectButton = new QPushButton(QIcon(":/assets/icons/save.png"), "Save Project (.pspx)");
    m_saveProjectButton->setToolTip("Save as .pspx project file to continue editing later");
    m_saveProjectButton->setStyleSheet(QString("QPushButton { text-align: left; padding: %1px; font-size: %2px; }").arg(SPACING_MD).arg(FONT_SIZE_BODY));
    connect(m_saveProjectButton, &QPushButton::clicked, this, &SaveExportDialog::onSaveProject);
    mainLayout->addWidget(m_saveProjectButton);

    // Export button
    m_exportImageButton = new QPushButton("Export Image (PNG/JPG/...)");
    m_exportImageButton->setToolTip("Export as image file for sharing");
    m_exportImageButton->setStyleSheet(QString("QPushButton { text-align: left; padding: %1px; font-size: %2px; }").arg(SPACING_MD).arg(FONT_SIZE_BODY));
    connect(m_exportImageButton, &QPushButton::clicked, this, &SaveExportDialog::onExportImage);
    mainLayout->addWidget(m_exportImageButton);

    mainLayout->addSpacing(SPACING_LG);
    
    // Cancel button
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    m_cancelButton = new QPushButton("Cancel");
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(m_cancelButton);
    mainLayout->addLayout(buttonLayout);
    
    // Add info text
    QLabel *infoText = new QLabel(
        "<b>Save Project</b>: Keeps all your work editable in .pspx format<br>"
        "<b>Export Image</b>: Creates final image files for sharing"
    );
    infoText->setWordWrap(true);
    infoText->setStyleSheet(QString("color: palette(mid); font-size: %1px; margin-top: %2px;").arg(FONT_SIZE_CAPTION).arg(SPACING_MD));
    mainLayout->addWidget(infoText);
}

QString SaveExportDialog::saveProject(QWidget *parent, const QString &suggestedName)
{
    return saveProject(parent, suggestedName, QString());
}

QString SaveExportDialog::saveProject(QWidget *parent, const QString &suggestedName, const QString &existingProjectPath)
{
    // If we have an existing project path, we're re-saving an existing project
    if (!existingProjectPath.isEmpty()) {
        // Delete the old file first
        QFile oldFile(existingProjectPath);
        if (oldFile.exists()) {
            if (!oldFile.remove()) {
                QMessageBox::warning(parent, "Warning", 
                    QString("Could not delete old project file: %1").arg(existingProjectPath));
                // Continue anyway - maybe the file is locked temporarily
            }
        }
        
        // Save to the same path
        QFileInfo fileInfo(existingProjectPath);
        s_lastProjectPath = fileInfo.absolutePath();
        
        // Save to settings
        QSettings settings;
        settings.setValue("lastProjectPath", s_lastProjectPath);
        
        return existingProjectPath;
    }
    
    // Original logic for new projects
    // Get or create AppData projects folder
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString projectsPath = appDataPath + "/Projects";
    QDir dir;
    if (!dir.exists(projectsPath)) {
        dir.mkpath(projectsPath);
    }
    
    // Use last saved location or default to projects folder
    QString startPath = s_lastProjectPath.isEmpty() ? projectsPath : s_lastProjectPath;
    
    QString fileName = suggestedName.isEmpty() ? 
        QString("Untitled_%1").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")) : 
        suggestedName;
    
    QString fullPath = QFileDialog::getSaveFileName(
        parent,
        "Save Project",
        startPath + "/" + fileName + ".pspx",
        "PixelPaint Project Files (*.pspx);;All Files (*)"
    );
    
    if (!fullPath.isEmpty()) {
        // Remember the directory for next time
        QFileInfo fileInfo(fullPath);
        s_lastProjectPath = fileInfo.absolutePath();
        
        // Save to settings
        QSettings settings;
        settings.setValue("lastProjectPath", s_lastProjectPath);
    }
    
    return fullPath;
}

QString SaveExportDialog::exportImage(QWidget *parent, const QString &suggestedName)
{
    // Use last export location or default to Pictures folder
    QString startPath = s_lastExportPath.isEmpty() ? 
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) : 
        s_lastExportPath;
    
    QString fileName = suggestedName.isEmpty() ? 
        QString("PixelArt_%1").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")) : 
        suggestedName;
    
    QString fullPath = QFileDialog::getSaveFileName(
        parent,
        "Export Image",
        startPath + "/" + fileName + ".png",
        "PNG Files (*.png);;JPEG Files (*.jpg *.jpeg);;BMP Files (*.bmp);;All Files (*)"
    );
    
    if (!fullPath.isEmpty()) {
        // Remember the directory for next time
        QFileInfo fileInfo(fullPath);
        s_lastExportPath = fileInfo.absolutePath();
        
        // Save to settings
        QSettings settings;
        settings.setValue("lastExportPath", s_lastExportPath);
    }
    
    return fullPath;
}

void SaveExportDialog::onSaveProject()
{
    QString projectName = m_projectNameEdit->text().trimmed();
    if (projectName.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please enter a project name.");
        return;
    }
    
    QString filePath = saveProject(this, projectName);
    if (!filePath.isEmpty()) {
        accept();
    }
}

void SaveExportDialog::onExportImage()
{
    QString projectName = m_projectNameEdit->text().trimmed();
    QString filePath = exportImage(this, projectName);
    if (!filePath.isEmpty()) {
        accept();
    }
}

QString SaveExportDialog::getLastSaveLocation()
{
    QSettings settings;
    return settings.value("lastProjectPath", 
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/Projects").toString();
}

void SaveExportDialog::saveLastSaveLocation(const QString &path)
{
    QSettings settings;
    settings.setValue("lastProjectPath", path);
    s_lastProjectPath = path;
} 