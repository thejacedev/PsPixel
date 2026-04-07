#ifndef MAGICWANDTOOL_H
#define MAGICWANDTOOL_H

#include "basetool.h"
#include <QImage>

namespace PixelPaint {

class MagicWandTool : public BaseTool
{
    Q_OBJECT

public:
    explicit MagicWandTool(QObject *parent = nullptr);

    ToolType getType() const override { return ToolType::MagicWand; }
    QString getName() const override { return "Magic Wand"; }
    QString getIconPath() const override { return ":/assets/icons/magicwand.png"; }
    QString getTooltip() const override { return "Magic Wand (W)\nClick to select contiguous pixels of similar color\nShift+click to add to selection"; }

    void onMousePress(const QPoint &canvasPos, Qt::MouseButton button) override;
    void onMouseMove(const QPoint &canvasPos, Qt::MouseButtons buttons) override;
    void onMouseRelease(const QPoint &canvasPos, Qt::MouseButton button) override;

private:
    QImage floodSelect(int x, int y, int tolerance);
    bool colorsMatch(QRgb a, QRgb b, int tolerance) const;

    int m_tolerance;
};

} // namespace PixelPaint

#endif // MAGICWANDTOOL_H
