#ifndef LINETOOL_H
#define LINETOOL_H

#include "basetool.h"
#include <QImage>

namespace PixelPaint {

class LineTool : public BaseTool
{
    Q_OBJECT

public:
    explicit LineTool(QObject *parent = nullptr);

    // Tool information
    ToolType getType() const override { return ToolType::Line; }
    QString getName() const override { return "Line"; }
    QString getIconPath() const override { return ":/assets/icons/line.png"; }
    QString getTooltip() const override { return "Line Tool (L) - Draw straight lines"; }

    // Mouse event handling
    void onMousePress(const QPoint &canvasPos, Qt::MouseButton button) override;
    void onMouseMove(const QPoint &canvasPos, Qt::MouseButtons buttons) override;
    void onMouseRelease(const QPoint &canvasPos, Qt::MouseButton button) override;
    
    // Tool activation/deactivation
    void onDeactivate() override;

private:
    void drawPreviewLine();
    void commitLine();
    
    QPoint m_startPoint;
    QPoint m_endPoint;
    bool m_drawing;
    QImage m_originalCanvas;
};

} // namespace PixelPaint

#endif // LINETOOL_H 