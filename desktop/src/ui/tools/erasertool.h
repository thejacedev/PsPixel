#ifndef ERASERTOOL_H
#define ERASERTOOL_H

#include "basetool.h"

namespace PixelPaint {

class EraserTool : public BaseTool
{
    Q_OBJECT

public:
    explicit EraserTool(QObject *parent = nullptr);

    // Tool information
    ToolType getType() const override { return ToolType::Eraser; }
    QString getName() const override { return "Eraser"; }
    QString getIconPath() const override { return ":/assets/icons/eraser.png"; }
    QString getTooltip() const override { return "Eraser Tool (E) - Erase pixels to transparent"; }

    // Mouse event handling
    void onMousePress(const QPoint &canvasPos, Qt::MouseButton button) override;
    void onMouseMove(const QPoint &canvasPos, Qt::MouseButtons buttons) override;
    void onMouseRelease(const QPoint &canvasPos, Qt::MouseButton button) override;

private:
    void drawLine(const QPoint &from, const QPoint &to);
};

} // namespace PixelPaint

#endif // ERASERTOOL_H 