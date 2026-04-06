#ifndef EYEDROPPERTOOL_H
#define EYEDROPPERTOOL_H

#include "basetool.h"

namespace PixelPaint {

class EyedropperTool : public BaseTool
{
    Q_OBJECT

public:
    explicit EyedropperTool(QObject *parent = nullptr);

    // Tool information
    ToolType getType() const override { return ToolType::Eyedropper; }
    QString getName() const override { return "Eyedropper"; }
    QString getIconPath() const override { return ":/assets/icons/eyedropper.png"; }
    QString getTooltip() const override { return "Eyedropper Tool (I) - Pick color from canvas"; }

    // Mouse event handling
    void onMousePress(const QPoint &canvasPos, Qt::MouseButton button) override;
    void onMouseMove(const QPoint &canvasPos, Qt::MouseButtons buttons) override;
    void onMouseRelease(const QPoint &canvasPos, Qt::MouseButton button) override;

signals:
    void colorPicked(const QColor &color);

private:
    QColor getPixelColor(int x, int y) const;
};

} // namespace PixelPaint

#endif // EYEDROPPERTOOL_H 