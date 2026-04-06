#ifndef RECTANGLETOOL_H
#define RECTANGLETOOL_H

#include "basetool.h"
#include <QImage>

namespace PixelPaint {

class RectangleTool : public BaseTool
{
    Q_OBJECT

public:
    explicit RectangleTool(QObject *parent = nullptr);

    ToolType getType() const override { return ToolType::Rectangle; }
    QString getName() const override { return "Rectangle"; }
    QString getIconPath() const override { return ":/assets/icons/rectangle.png"; }
    QString getTooltip() const override { return "Rectangle Tool (R) - Draw rectangles. Hold Shift for filled."; }

    void onMousePress(const QPoint &canvasPos, Qt::MouseButton button) override;
    void onMouseMove(const QPoint &canvasPos, Qt::MouseButtons buttons) override;
    void onMouseRelease(const QPoint &canvasPos, Qt::MouseButton button) override;
    void onDeactivate() override;

private:
    void drawPreview(bool filled);
    void commit(bool filled);

    QPoint m_startPoint;
    QPoint m_endPoint;
    bool m_drawing;
    bool m_filled;
    QImage m_originalCanvas;
};

} // namespace PixelPaint

#endif // RECTANGLETOOL_H
