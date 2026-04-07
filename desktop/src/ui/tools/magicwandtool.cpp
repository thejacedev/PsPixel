#include "magicwandtool.h"
#include "pixelcanvas.h"
#include <QStack>
#include <QApplication>

using namespace PixelPaint;

MagicWandTool::MagicWandTool(QObject *parent)
    : BaseTool(parent)
    , m_tolerance(0)
{
}

void MagicWandTool::onMousePress(const QPoint &canvasPos, Qt::MouseButton button)
{
    if (button != Qt::LeftButton || !m_canvas) return;

    int x = canvasPos.x();
    int y = canvasPos.y();
    if (!isValidPosition(x, y)) return;

    QImage newMask = floodSelect(x, y, m_tolerance);

    // Shift+click adds to existing selection
    bool additive = QApplication::keyboardModifiers() & Qt::ShiftModifier;

    if (additive && m_canvas->hasSelection()) {
        // Merge with existing mask
        QImage existing = m_canvas->selectionMask();
        for (int py = 0; py < newMask.height(); ++py) {
            for (int px = 0; px < newMask.width(); ++px) {
                if (qGray(newMask.pixel(px, py)) > 127) {
                    existing.setPixel(px, py, qRgb(255, 255, 255));
                }
            }
        }
        m_canvas->setSelectionMask(existing);
    } else {
        m_canvas->setSelectionMask(newMask);
    }
}

void MagicWandTool::onMouseMove(const QPoint &canvasPos, Qt::MouseButtons buttons)
{
    Q_UNUSED(canvasPos)
    Q_UNUSED(buttons)
}

void MagicWandTool::onMouseRelease(const QPoint &canvasPos, Qt::MouseButton button)
{
    Q_UNUSED(canvasPos)
    Q_UNUSED(button)
}

QImage MagicWandTool::floodSelect(int x, int y, int tolerance)
{
    int w = m_canvas->getCanvasWidth();
    int h = m_canvas->getCanvasHeight();

    QImage mask(w, h, QImage::Format_Grayscale8);
    mask.fill(0);

    const QImage &img = m_canvas->canvasImage();
    QRgb targetRgb = img.pixel(x, y);

    QStack<QPoint> stack;
    stack.push(QPoint(x, y));

    while (!stack.isEmpty()) {
        QPoint pt = stack.pop();
        int px = pt.x();
        int py = pt.y();

        if (px < 0 || px >= w || py < 0 || py >= h) continue;
        if (qGray(mask.pixel(px, py)) > 127) continue; // Already visited
        if (!colorsMatch(img.pixel(px, py), targetRgb, tolerance)) continue;

        mask.setPixel(px, py, qRgb(255, 255, 255));

        stack.push(QPoint(px + 1, py));
        stack.push(QPoint(px - 1, py));
        stack.push(QPoint(px, py + 1));
        stack.push(QPoint(px, py - 1));
    }

    return mask;
}

bool MagicWandTool::colorsMatch(QRgb a, QRgb b, int tolerance) const
{
    int dr = qAbs(qRed(a) - qRed(b));
    int dg = qAbs(qGreen(a) - qGreen(b));
    int db = qAbs(qBlue(a) - qBlue(b));
    int da = qAbs(qAlpha(a) - qAlpha(b));
    return (dr + dg + db + da) <= tolerance * 4;
}
