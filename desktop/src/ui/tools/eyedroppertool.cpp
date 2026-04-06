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
    if (!m_canvas || !isValidPosition(x, y)) {
        return QColor();
    }
    return m_canvas->pixelAt(x, y);
}
