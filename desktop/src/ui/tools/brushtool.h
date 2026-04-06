#ifndef BRUSHTOOL_H
#define BRUSHTOOL_H

#include "basetool.h"

namespace PixelPaint {

class BrushTool : public BaseTool
{
    Q_OBJECT

public:
    explicit BrushTool(QObject *parent = nullptr);

    // Tool information
    ToolType getType() const override { return ToolType::Brush; }
    QString getName() const override { return "Brush"; }
    QString getIconPath() const override { return ":/assets/icons/brush.png"; }
    QString getTooltip() const override { return "Brush Tool (B) - Paint with the selected color"; }

    // Mouse event handling
    void onMousePress(const QPoint &canvasPos, Qt::MouseButton button) override;
    void onMouseMove(const QPoint &canvasPos, Qt::MouseButtons buttons) override;
    void onMouseRelease(const QPoint &canvasPos, Qt::MouseButton button) override;

private:
    void drawLine(const QPoint &from, const QPoint &to);
};

} // namespace PixelPaint

#endif // BRUSHTOOL_H 