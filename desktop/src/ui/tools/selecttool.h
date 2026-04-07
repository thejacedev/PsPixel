#ifndef SELECTTOOL_H
#define SELECTTOOL_H

#include "basetool.h"
#include <QImage>
#include <QRect>

namespace PixelPaint {

class SelectTool : public BaseTool
{
    Q_OBJECT

public:
    explicit SelectTool(QObject *parent = nullptr);

    ToolType getType() const override { return ToolType::Select; }
    QString getName() const override { return "Select"; }
    QString getIconPath() const override { return ":/assets/icons/select.png"; }
    QString getTooltip() const override { return "Rectangular Select (V)\nDrag to select, then draw inside selection\nClick outside to deselect"; }

    void onMousePress(const QPoint &canvasPos, Qt::MouseButton button) override;
    void onMouseMove(const QPoint &canvasPos, Qt::MouseButtons buttons) override;
    void onMouseRelease(const QPoint &canvasPos, Qt::MouseButton button) override;
    void onDeactivate() override;

private:
    enum class Phase { None, Selecting, Moving };

    void applySelectionToCanvas();
    void commitMove();

    Phase m_phase;
    bool m_hasSelection;

    // Selection phase
    QPoint m_selectStart;
    QPoint m_selectEnd;

    // Move phase
    QPoint m_moveStart;
    QRect m_selectionRect;
    QImage m_selectionPixels;    // The copied pixel region
    QImage m_canvasBeforeMove;   // Canvas state before move started
};

} // namespace PixelPaint

#endif // SELECTTOOL_H
