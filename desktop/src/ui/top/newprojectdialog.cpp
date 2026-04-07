#include "newprojectdialog.h"
#include "constants.h"
#include <QIcon>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QApplication>

using namespace PixelPaint;

NewProjectDialog::NewProjectDialog(QWidget *parent)
    : QDialog(parent)
    , m_createNewButton(nullptr)
    , m_showStartScreenButton(nullptr)
    , m_cancelButton(nullptr)
{
    setupUI();
    setWindowTitle("New Project");
    setModal(true);
    resize(400, 200);
}

void NewProjectDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Title
    QLabel *titleLabel = new QLabel("Create New Project");
    titleLabel->setStyleSheet(QString("font-size: %1px; font-weight: bold; color: palette(windowText); margin-bottom: %2px;").arg(FONT_SIZE_TITLE).arg(SPACING_MD));
    mainLayout->addWidget(titleLabel);
    
    // Info label
    QLabel *infoLabel = new QLabel("Choose how to create your new project:");
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet(QString("color: palette(text); font-size: %1px; margin-bottom: 20px;").arg(FONT_SIZE_BODY));
    mainLayout->addWidget(infoLabel);
    
    // Start screen button
    m_showStartScreenButton = new QPushButton(QIcon(":/assets/icons/home.png"), "Go to Start Screen");
    m_showStartScreenButton->setToolTip("Return to the start screen to create a new project with preset options");
    m_showStartScreenButton->setStyleSheet(
        QString("QPushButton { text-align: left; padding: %4px; font-size: %5px; "
        "background-color: %1; color: white; border: none; border-radius: %6px; font-weight: bold; }"
        "QPushButton:hover { background-color: %2; }"
        "QPushButton:pressed { background-color: %3; }")
        .arg(ACCENT_HEX, ACCENT_HOVER_HEX, ACCENT_PRESSED_HEX)
        .arg(SPACING_LG).arg(FONT_SIZE_BODY).arg(RADIUS_CONTROL)
    );
    connect(m_showStartScreenButton, &QPushButton::clicked, this, &NewProjectDialog::onShowStartScreen);
    mainLayout->addWidget(m_showStartScreenButton);
    
    // Quick create button
    m_createNewButton = new QPushButton(QIcon(":/assets/icons/bolt.png"), "Quick Create (64x64)");
    m_createNewButton->setToolTip("Quickly create a new 64x64 project with default settings");
    m_createNewButton->setStyleSheet(QString("QPushButton { text-align: left; padding: %1px; font-size: %2px; }").arg(SPACING_LG).arg(FONT_SIZE_BODY));
    connect(m_createNewButton, &QPushButton::clicked, this, &NewProjectDialog::onCreateNewProject);
    mainLayout->addWidget(m_createNewButton);
    
    mainLayout->addSpacing(20);
    
    // Cancel button
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    m_cancelButton = new QPushButton("Cancel");
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(m_cancelButton);
    mainLayout->addLayout(buttonLayout);
    
    // Add info text
    QLabel *infoText = new QLabel(
        "<b>Start Screen</b>: Full project customization with presets and templates<br>"
        "<b>Quick Create</b>: Fast setup with common defaults"
    );
    infoText->setWordWrap(true);
    infoText->setStyleSheet(QString("color: palette(mid); font-size: %1px; margin-top: %2px;").arg(FONT_SIZE_CAPTION).arg(SPACING_MD));
    mainLayout->addWidget(infoText);
}

bool NewProjectDialog::showNewProjectDialog(QWidget *parent)
{
    NewProjectDialog dialog(parent);
    return dialog.exec() == QDialog::Accepted;
}

void NewProjectDialog::onCreateNewProject()
{
    // Set a custom result to indicate quick create
    setResult(2); // Custom result code for quick create
    accept();
}

void NewProjectDialog::onShowStartScreen()
{
    // Set result to 1 (accepted) to indicate start screen
    setResult(1);
    accept();
} 