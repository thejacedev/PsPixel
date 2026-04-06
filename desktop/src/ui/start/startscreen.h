#ifndef STARTSCREEN_H
#define STARTSCREEN_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>
#include <QSpinBox>
#include <QComboBox>
#include <QColorDialog>
#include <QPixmap>
#include <QSettings>
#include <QDateTime>
#include <QInputDialog>
#include <QMessageBox>
#include <QMenu>
#include <QAction>
#include "constants.h"
#include "../../fileformat/pspxformat.h"

namespace PixelPaint {

struct RecentProject {
    QString name;
    QString filePath;
    QPixmap thumbnail;
    QDateTime lastModified;
    int canvasWidth;
    int canvasHeight;
    int pixelSize;
};

struct ProjectPreset {
    QString name;
    int width;
    int height;
    int pixelSize;
    QColor backgroundColor;
    bool isBuiltIn;
    
    ProjectPreset() : width(64), height(64), pixelSize(8), backgroundColor(0, 0, 0, 0), isBuiltIn(false) {}
    ProjectPreset(const QString &n, int w, int h, int ps, const QColor &bg, bool builtin = false)
        : name(n), width(w), height(h), pixelSize(ps), backgroundColor(bg), isBuiltIn(builtin) {}
};

class ProjectThumbnail : public QFrame
{
    Q_OBJECT

public:
    explicit ProjectThumbnail(const RecentProject &project, QWidget *parent = nullptr);

signals:
    void projectSelected(const QString &filePath);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    RecentProject m_project;
    bool m_hovered;
};

class NewProjectPanel : public QFrame
{
    Q_OBJECT

public:
    explicit NewProjectPanel(QWidget *parent = nullptr);

signals:
    void createNewProject(int width, int height, int pixelSize, const QColor &backgroundColor);

private slots:
    void onCreateProject();
    void onSelectBackgroundColor();
    void onPresetChanged(int index);
    void onSavePreset();
    void onDeletePreset();

private:
    void setupUI();
    void loadPresets();
    void savePresets();
    void updatePresetCombo();
    void loadCustomColors();
    void saveCustomColors();
    void setupColorDialog(QColorDialog &dialog);
    void updateBackgroundColorButton();
    
    QSpinBox *m_widthSpinBox;
    QSpinBox *m_heightSpinBox;
    QSpinBox *m_pixelSizeSpinBox;
    QPushButton *m_backgroundColorButton;
    QComboBox *m_presetsCombo;
    QPushButton *m_createButton;
    QPushButton *m_savePresetButton;
    QPushButton *m_deletePresetButton;
    QColor m_backgroundColor;
    
    QVector<ProjectPreset> m_presets;
    QVector<QColor> m_customColors;
    bool m_updatingFromPreset;
};

class StartScreen : public QWidget
{
    Q_OBJECT

public:
    explicit StartScreen(QWidget *parent = nullptr);
    
    // Public constants for thumbnail sizes
    static constexpr int THUMBNAIL_WIDTH = 180;
    static constexpr int THUMBNAIL_HEIGHT = 140;
    static constexpr int MAX_RECENT_PROJECTS = 12;

signals:
    void projectSelected(const QString &filePath);
    void newProjectRequested(int width, int height, int pixelSize, const QColor &backgroundColor);

public slots:
    void refreshRecentProjects();

private slots:
    void onProjectThumbnailClicked(const QString &filePath);
    void onNewProjectClicked(int width, int height, int pixelSize, const QColor &backgroundColor);

private:
    void setupUI();
    void loadRecentProjects();
    QPixmap generateThumbnail(const ProjectData &projectData, const QSize &size);
    
    QVBoxLayout *m_mainLayout;
    QScrollArea *m_recentProjectsArea;
    QGridLayout *m_projectsGrid;
    NewProjectPanel *m_newProjectPanel;
    QLabel *m_titleLabel;
    QLabel *m_recentLabel;
    
    QVector<RecentProject> m_recentProjects;
};

} // namespace PixelPaint

#endif // STARTSCREEN_H 