#ifndef LASSOTOOL_H
#define LASSOTOOL_H

#include "basetool.h"
#include <QImage>
#include <QPolygon>

namespace PixelPaint {

class LassoTool : public BaseTool
{
    Q_OBJECT

public:
    explicit LassoTool(QObject *parent = nullptr);

    ToolType getType() const override { return ToolType::Lasso; }
    QString getName() const override { return "Lasso"; }
    QString getIconPath() const override { return ":/assets/icons/lasso.png"; }
    QString getTooltip() const override { return "Lasso Select (L)\nDraw a freehand shape to select an area\nShift+draw to add to selection"; }

    void onMousePress(const QPoint &canvasPos, Qt::MouseButton button) override;
    void onMouseMove(const QPoint &canvasPos, Qt::MouseButtons buttons) override;
    void onMouseRelease(const QPoint &canvasPos, Qt::MouseButton button) override;

    // For overlay rendering
    bool isDrawing() const { return m_drawing; }
    const QPolygon& currentPolygon() const { return m_polygon; }

private:
    QImage polygonToMask(const QPolygon &polygon);

    bool m_drawing;
    QPolygon m_polygon;
};

} // namespace PixelPaint

#endif // LASSOTOOL_H
