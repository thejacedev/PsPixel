#ifndef CIRCLETOOL_H
#define CIRCLETOOL_H

#include "basetool.h"
#include <QImage>

namespace PixelPaint {

class CircleTool : public BaseTool
{
    Q_OBJECT

public:
    explicit CircleTool(QObject *parent = nullptr);

    ToolType getType() const override { return ToolType::Circle; }
    QString getName() const override { return "Circle"; }
    QString getIconPath() const override { return ":/assets/icons/circle.png"; }
    QString getTooltip() const override { return "Circle Tool (O) - Draw circles. Hold Shift for filled."; }

    void onMousePress(const QPoint &canvasPos, Qt::MouseButton button) override;
    void onMouseMove(const QPoint &canvasPos, Qt::MouseButtons buttons) override;
    void onMouseRelease(const QPoint &canvasPos, Qt::MouseButton button) override;
    void onDeactivate() override;

private:
    void drawPreview(bool filled);
    void commit(bool filled);
    int currentRadius() const;

    QPoint m_center;
    QPoint m_endPoint;
    bool m_drawing;
    bool m_filled;
    QImage m_originalCanvas;
};

} // namespace PixelPaint

#endif // CIRCLETOOL_H
