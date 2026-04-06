#ifndef SAVEEXPORTDIALOG_H
#define SAVEEXPORTDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

namespace PixelPaint {

class SaveExportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SaveExportDialog(QWidget *parent = nullptr);
    
    enum SaveType {
        SaveProject,   // Save as .pspx
        ExportImage    // Export as PNG/JPG/etc
    };
    
    static QString saveProject(QWidget *parent, const QString &suggestedName = "");
    static QString saveProject(QWidget *parent, const QString &suggestedName, const QString &existingProjectPath);
    static QString exportImage(QWidget *parent, const QString &suggestedName = "");

private slots:
    void onSaveProject();
    void onExportImage();

private:
    void setupUI();
    QString getLastSaveLocation();
    void saveLastSaveLocation(const QString &path);
    
    QPushButton *m_saveProjectButton;
    QPushButton *m_exportImageButton;
    QPushButton *m_cancelButton;
    QLineEdit *m_projectNameEdit;
    QLabel *m_infoLabel;
    
    static QString s_lastProjectPath;
    static QString s_lastExportPath;
};

} // namespace PixelPaint

#endif // SAVEEXPORTDIALOG_H 