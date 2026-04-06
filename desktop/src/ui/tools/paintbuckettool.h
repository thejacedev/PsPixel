#ifndef PAINTBUCKETTOOL_H
#define PAINTBUCKETTOOL_H

#include "basetool.h"
#include <QQueue>

namespace PixelPaint {

class PaintBucketTool : public BaseTool
{
    Q_OBJECT

public:
    explicit PaintBucketTool(QObject *parent = nullptr);

    // Tool information
    ToolType getType() const override { return ToolType::PaintBucket; }
    QString getName() const override { return "Paint Bucket"; }
    QString getIconPath() const override { return ":/assets/icons/paintbucket.png"; }
    QString getTooltip() const override { return "Paint Bucket Tool (G) - Fill connected area with color"; }

    // Mouse event handling
    void onMousePress(const QPoint &canvasPos, Qt::MouseButton button) override;
    void onMouseMove(const QPoint &canvasPos, Qt::MouseButtons buttons) override;
    void onMouseRelease(const QPoint &canvasPos, Qt::MouseButton button) override;

private:
    void floodFill(int x, int y, const QColor &targetColor, const QColor &fillColor);
    bool colorsEqual(const QColor &c1, const QColor &c2) const;
};

} // namespace PixelPaint

#endif // PAINTBUCKETTOOL_H 