#include "lassotool.h"
#include "pixelcanvas.h"
#include <QPainter>
#include <QPainterPath>
#include <QApplication>

using namespace PixelPaint;

LassoTool::LassoTool(QObject *parent)
    : BaseTool(parent)
    , m_drawing(false)
{
}

void LassoTool::onMousePress(const QPoint &canvasPos, Qt::MouseButton button)
{
    if (button != Qt::LeftButton || !m_canvas) return;

    m_drawing = true;
    m_polygon.clear();
    m_polygon.append(canvasPos);
}

void LassoTool::onMouseMove(const QPoint &canvasPos, Qt::MouseButtons buttons)
{
    if (!m_drawing || !(buttons & Qt::LeftButton) || !m_canvas) return;

    // Only add point if it differs from the last one
    if (m_polygon.isEmpty() || m_polygon.last() != canvasPos) {
        m_polygon.append(canvasPos);
        m_canvas->update();
    }
}

void LassoTool::onMouseRelease(const QPoint &canvasPos, Qt::MouseButton button)
{
    if (button != Qt::LeftButton || !m_canvas || !m_drawing) return;

    m_drawing = false;

    // Need at least 3 points to form a polygon
    if (m_polygon.size() < 3) {
        m_polygon.clear();
        return;
    }

    // Close the polygon
    m_polygon.append(m_polygon.first());

    QImage newMask = polygonToMask(m_polygon);

    // Shift+click adds to existing selection
    bool additive = QApplication::keyboardModifiers() & Qt::ShiftModifier;

    if (additive && m_canvas->hasSelection()) {
        QImage existing = m_canvas->selectionMask();
        for (int y = 0; y < newMask.height(); ++y) {
            for (int x = 0; x < newMask.width(); ++x) {
                if (qGray(newMask.pixel(x, y)) > 127) {
                    existing.setPixel(x, y, qRgb(255, 255, 255));
                }
            }
        }
        m_canvas->setSelectionMask(existing);
    } else {
        m_canvas->setSelectionMask(newMask);
    }

    m_polygon.clear();
}

QImage LassoTool::polygonToMask(const QPolygon &polygon)
{
    int w = m_canvas->getCanvasWidth();
    int h = m_canvas->getCanvasHeight();

    QImage mask(w, h, QImage::Format_Grayscale8);
    mask.fill(0);

    // Use QPainter to fill the polygon on the mask
    QPainter painter(&mask);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 255, 255));

    QPainterPath path;
    path.addPolygon(QPolygonF(polygon));
    path.closeSubpath();
    painter.drawPath(path);
    painter.end();

    return mask;
}
