#include "linetool.h"
#include "pixelcanvas.h"
#include <QtMath>

using namespace PixelPaint;

LineTool::LineTool(QObject *parent)
    : BaseTool(parent)
    , m_drawing(false)
{
}

void LineTool::onMousePress(const QPoint &canvasPos, Qt::MouseButton button)
{
    if (button == Qt::LeftButton && m_canvas) {
        startHistoryOperation();

        m_drawing = true;
        m_startPoint = canvasPos;
        m_endPoint = canvasPos;

        m_canvas->setLinePreviewStart(m_startPoint);

        // Save original canvas for preview restoration (COW — nearly free)
        m_originalCanvas = m_canvas->canvasImage();
    }
}

void LineTool::onMouseMove(const QPoint &canvasPos, Qt::MouseButtons buttons)
{
    if (m_drawing && (buttons & Qt::LeftButton)) {
        m_endPoint = canvasPos;
        drawPreviewLine();
    }
}

void LineTool::onMouseRelease(const QPoint &canvasPos, Qt::MouseButton button)
{
    if (button == Qt::LeftButton && m_drawing) {
        m_endPoint = canvasPos;
        commitLine();
        m_drawing = false;

        m_canvas->clearLinePreviewStart();
        commitHistoryOperation("Draw Line");
        emit canvasModified();
    }
}

void LineTool::drawPreviewLine()
{
    if (!m_canvas) return;

    // Start from a fresh copy of the original (COW — deep copy only on first write)
    QImage preview = m_originalCanvas;

    QVector<QPoint> points = getLinePoints(m_startPoint, m_endPoint);
    QRgb colorRgb = m_color.rgba();

    for (const QPoint &point : points) {
        if (isValidPosition(point.x(), point.y())) {
            const int halfSize = m_brushSize / 2;

            for (int dy = -halfSize; dy <= halfSize; ++dy) {
                for (int dx = -halfSize; dx <= halfSize; ++dx) {
                    int px = point.x() + dx;
                    int py = point.y() + dy;

                    if (isValidPosition(px, py)) {
                        if (m_brushSize > 1) {
                            const double distance = qSqrt(dx * dx + dy * dy);
                            if (distance > halfSize + 0.5) continue;
                        }

                        preview.setPixel(px, py, colorRgb);
                    }
                }
            }
        }
    }

    m_canvas->canvasImageRef() = preview;
    m_canvas->update();
}

void LineTool::commitLine()
{
    if (!m_canvas) return;

    // Restore original canvas, then draw final line with direct pixel access
    m_canvas->canvasImageRef() = m_originalCanvas;

    QVector<QPoint> points = getLinePoints(m_startPoint, m_endPoint);

    for (const QPoint &point : points) {
        // Use drawPixel which handles brush size
        drawPixel(point.x(), point.y(), m_color);
    }
}

void LineTool::onDeactivate()
{
    if (m_canvas) {
        m_canvas->clearLinePreviewStart();

        if (m_drawing) {
            cancelHistoryOperation();
            m_drawing = false;
        }
    }
}
