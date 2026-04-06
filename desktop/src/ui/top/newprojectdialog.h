#ifndef NEWPROJECTDIALOG_H
#define NEWPROJECTDIALOG_H

#include <QDialog>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

namespace PixelPaint {

class NewProjectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewProjectDialog(QWidget *parent = nullptr);
    
    // Static method for showing the dialog
    static bool showNewProjectDialog(QWidget *parent = nullptr);

private slots:
    void onCreateNewProject();
    void onShowStartScreen();

private:
    void setupUI();
    
    QPushButton *m_createNewButton;
    QPushButton *m_showStartScreenButton;
    QPushButton *m_cancelButton;
};

} // namespace PixelPaint

#endif // NEWPROJECTDIALOG_H 