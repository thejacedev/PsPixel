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
    QString getTooltip() const override { return "Select Tool (V) - Select and move regions"; }

    void onMousePress(const QPoint &canvasPos, Qt::MouseButton button) override;
    void onMouseMove(const QPoint &canvasPos, Qt::MouseButtons buttons) override;
    void onMouseRelease(const QPoint &canvasPos, Qt::MouseButton button) override;
    void onDeactivate() override;

    // For overlay rendering
    bool hasSelection() const { return m_hasSelection; }
    QRect selectionRect() const { return m_selectionRect; }

private:
    enum class Phase { None, Selecting, Moving };

    void commitSelection();
    void clearSelection();

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
