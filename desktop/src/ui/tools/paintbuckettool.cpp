#include "paintbuckettool.h"
#include "pixelcanvas.h"
#include <QStack>

using namespace PixelPaint;

PaintBucketTool::PaintBucketTool(QObject *parent)
    : BaseTool(parent)
{
}

void PaintBucketTool::onMousePress(const QPoint &canvasPos, Qt::MouseButton button)
{
    if (button == Qt::LeftButton && m_canvas) {
        int x = canvasPos.x();
        int y = canvasPos.y();

        if (isValidPosition(x, y)) {
            QColor targetColor = m_canvas->pixelAt(x, y);

            if (!colorsEqual(targetColor, m_color)) {
                startHistoryOperation();
                floodFill(x, y, targetColor, m_color);
                commitHistoryOperation("Paint Bucket");
                emit canvasModified();
            }
        }
    }
}

void PaintBucketTool::onMouseMove(const QPoint &canvasPos, Qt::MouseButtons buttons)
{
    Q_UNUSED(canvasPos)
    Q_UNUSED(buttons)
}

void PaintBucketTool::onMouseRelease(const QPoint &canvasPos, Qt::MouseButton button)
{
    Q_UNUSED(canvasPos)
    Q_UNUSED(button)
}

void PaintBucketTool::floodFill(int x, int y, const QColor &targetColor, const QColor &fillColor)
{
    if (!m_canvas || !isValidPosition(x, y)) return;

    QImage &img = m_canvas->canvasImageRef();
    QRgb targetRgb = targetColor.rgba();
    QRgb fillRgb = fillColor.rgba();

    int w = m_canvas->getCanvasWidth();
    int h = m_canvas->getCanvasHeight();

    QStack<QPoint> stack;
    stack.push(QPoint(x, y));

    while (!stack.isEmpty()) {
        QPoint point = stack.pop();
        int px = point.x();
        int py = point.y();

        if (px < 0 || px >= w || py < 0 || py >= h) continue;
        if (img.pixel(px, py) != targetRgb) continue;

        img.setPixel(px, py, fillRgb);

        stack.push(QPoint(px + 1, py));
        stack.push(QPoint(px - 1, py));
        stack.push(QPoint(px, py + 1));
        stack.push(QPoint(px, py - 1));
    }

    m_canvas->update();
}

bool PaintBucketTool::colorsEqual(const QColor &c1, const QColor &c2) const
{
    return c1.rgba() == c2.rgba();
}
