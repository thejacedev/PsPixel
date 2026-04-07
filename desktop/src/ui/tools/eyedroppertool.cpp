#include "eyedroppertool.h"
#include "pixelcanvas.h"

using namespace PixelPaint;

EyedropperTool::EyedropperTool(QObject *parent)
    : BaseTool(parent)
{
}

void EyedropperTool::onMousePress(const QPoint &canvasPos, Qt::MouseButton button)
{
    if (button == Qt::LeftButton) {
        QColor pickedColor = getPixelColor(canvasPos.x(), canvasPos.y());
        if (pickedColor.isValid()) {
            emit colorPicked(pickedColor);
        }
    }
}

void EyedropperTool::onMouseMove(const QPoint &canvasPos, Qt::MouseButtons buttons)
{
    Q_UNUSED(canvasPos)
    Q_UNUSED(buttons)
}

void EyedropperTool::onMouseRelease(const QPoint &canvasPos, Qt::MouseButton button)
{
    Q_UNUSED(canvasPos)
    Q_UNUSED(button)
}

QColor EyedropperTool::getPixelColor(int x, int y) const
{
    if (!m_canvas) return QColor();

    // If reference layer is active, always sample from reference
    if (m_canvas->referenceActive() && m_canvas->hasReferenceImage()) {
        QColor refColor = m_canvas->referencePixelAt(x, y);
        if (refColor.isValid() && refColor.alpha() > 0) {
            return refColor;
        }
        return QColor();
    }

    if (!isValidPosition(x, y)) return QColor();

    QColor canvasColor = m_canvas->pixelAt(x, y);

    // If canvas pixel is transparent, try reference image
    if (canvasColor.alpha() == 0 && m_canvas->hasReferenceImage()) {
        QColor refColor = m_canvas->referencePixelAt(x, y);
        if (refColor.isValid() && refColor.alpha() > 0) {
            return refColor;
        }
    }

    return canvasColor;
}
