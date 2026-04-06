#include "newprojectdialog.h"
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
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: palette(windowText); margin-bottom: 10px;");
    mainLayout->addWidget(titleLabel);
    
    // Info label
    QLabel *infoLabel = new QLabel("Choose how to create your new project:");
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("color: palette(text); margin-bottom: 20px;");
    mainLayout->addWidget(infoLabel);
    
    // Start screen button
    m_showStartScreenButton = new QPushButton(QIcon(":/assets/icons/home.png"), "Go to Start Screen");
    m_showStartScreenButton->setToolTip("Return to the start screen to create a new project with preset options");
    m_showStartScreenButton->setStyleSheet(
        "QPushButton { text-align: left; padding: 15px; font-size: 12px; "
        "background-color: #0078d4; color: white; border: none; border-radius: 3px; font-weight: bold; }"
        "QPushButton:hover { background-color: #106ebe; }"
        "QPushButton:pressed { background-color: #005a9e; }"
    );
    connect(m_showStartScreenButton, &QPushButton::clicked, this, &NewProjectDialog::onShowStartScreen);
    mainLayout->addWidget(m_showStartScreenButton);
    
    // Quick create button
    m_createNewButton = new QPushButton(QIcon(":/assets/icons/bolt.png"), "Quick Create (64x64)");
    m_createNewButton->setToolTip("Quickly create a new 64x64 project with default settings");
    m_createNewButton->setStyleSheet("QPushButton { text-align: left; padding: 15px; font-size: 12px; }");
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
    infoText->setStyleSheet("color: #666; font-size: 10px; margin-top: 10px;");
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