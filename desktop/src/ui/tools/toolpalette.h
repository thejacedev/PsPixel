#ifndef TOOLPALETTE_H
#define TOOLPALETTE_H

#include <QDockWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
#include <QButtonGroup>
#include <QFrame>
#include <QScrollArea>
#include <QWheelEvent>
#include <QFocusEvent>
#include "constants.h"

namespace PixelPaint {

class ToolManager;
class BaseTool;

// Custom spinbox that's more responsive to wheel events and arrow buttons
class BrushSizeSpinBox : public QSpinBox
{
    Q_OBJECT
public:
    explicit BrushSizeSpinBox(QWidget *parent = nullptr) : QSpinBox(parent) {}

protected:
    void wheelEvent(QWheelEvent *event) override {
        // Make wheel events more responsive
        if (event->angleDelta().y() > 0) {
            stepUp();
        } else if (event->angleDelta().y() < 0) {
            stepDown();
        }
        event->accept();
    }
    
    void focusInEvent(QFocusEvent *event) override {
        QSpinBox::focusInEvent(event);
        // Select all text when focused to make typing easier
        selectAll();
    }
};

class ToolPalette : public QDockWidget
{
    Q_OBJECT

public:
    explicit ToolPalette(ToolManager *toolManager, QWidget *parent = nullptr);
    ~ToolPalette() = default;

    // Override for better dock widget behavior
    QSize sizeHint() const override;

    void updateCurrentTool();
    void updateBrushSize(int size);
    void updateCurrentColor(const QColor &color);

signals:
    void toolSelected(ToolType toolType);
    void brushSizeChanged(int size);
    void colorButtonClicked();

private slots:
    void onToolButtonClicked();
    void onBrushSizeChanged(int size);
    void onColorButtonClicked();

private:
    void setupUI();
    void createToolButtons();
    void createBrushSizeControls();
    void createColorControls();
    void createToolButton(ToolType toolType);
    void updateColorButtonAppearance(const QColor &color);

    ToolManager *m_toolManager;
    
    // UI elements
    QWidget *m_contentWidget;
    QVBoxLayout *m_mainLayout;
    
    // Tool section
    QFrame *m_toolsFrame;
    QGridLayout *m_toolsLayout;
    QButtonGroup *m_buttonGroup;
    QMap<ToolType, QPushButton*> m_toolButtons;
    
    // Brush section
    QFrame *m_brushFrame;
    QVBoxLayout *m_brushLayout;
    BrushSizeSpinBox *m_brushSizeSpinBox;
    QLabel *m_brushSizeLabel;
    
    // Color section
    QFrame *m_colorFrame;
    QVBoxLayout *m_colorLayout;
    QPushButton *m_colorButton;
    
    // Current state
    QColor m_currentColor;
    int m_currentBrushSize;
    ToolType m_currentToolType;
};

} // namespace PixelPaint

#endif // TOOLPALETTE_H 