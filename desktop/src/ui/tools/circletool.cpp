#include "circletool.h"
#include "pixelcanvas.h"
#include <QApplication>
#include <QtMath>

using namespace PixelPaint;

CircleTool::CircleTool(QObject *parent)
    : BaseTool(parent)
    , m_drawing(false)
    , m_filled(false)
{
}

int CircleTool::currentRadius() const
{
    int dx = m_endPoint.x() - m_center.x();
    int dy = m_endPoint.y() - m_center.y();
    return qMax(1, static_cast<int>(qSqrt(dx * dx + dy * dy)));
}

void CircleTool::onMousePress(const QPoint &canvasPos, Qt::MouseButton button)
{
    if (button == Qt::LeftButton && m_canvas) {
        startHistoryOperation();
        m_drawing = true;
        m_center = canvasPos;
        m_endPoint = canvasPos;
        m_filled = QApplication::keyboardModifiers() & Qt::ShiftModifier;
        m_originalCanvas = m_canvas->canvasImage();
    }
}

void CircleTool::onMouseMove(const QPoint &canvasPos, Qt::MouseButtons buttons)
{
    if (m_drawing && (buttons & Qt::LeftButton)) {
        m_endPoint = canvasPos;
        m_filled = QApplication::keyboardModifiers() & Qt::ShiftModifier;
        drawPreview(m_filled);
    }
}

void CircleTool::onMouseRelease(const QPoint &canvasPos, Qt::MouseButton button)
{
    if (button == Qt::LeftButton && m_drawing) {
        m_endPoint = canvasPos;
        m_filled = QApplication::keyboardModifiers() & Qt::ShiftModifier;
        commit(m_filled);
        m_drawing = false;
        commitHistoryOperation("Draw Circle");
        emit canvasModified();
    }
}

void CircleTool::drawPreview(bool filled)
{
    if (!m_canvas) return;

    QImage preview = m_originalCanvas;
    QRgb colorRgb = m_color.rgba();
    int radius = currentRadius();

    if (filled) {
        // Filled circle — scan all pixels in bounding box
        for (int y = m_center.y() - radius; y <= m_center.y() + radius; ++y) {
            for (int x = m_center.x() - radius; x <= m_center.x() + radius; ++x) {
                int dx = x - m_center.x();
                int dy = y - m_center.y();
                if (dx * dx + dy * dy <= radius * radius && isValidPosition(x, y)) {
                    preview.setPixel(x, y, colorRgb);
                }
            }
        }
    } else {
        // Outline — use Midpoint Circle Algorithm points
        QVector<QPoint> points = getCirclePoints(m_center, radius);
        for (const QPoint &point : points) {
            if (isValidPosition(point.x(), point.y())) {
                preview.setPixel(point.x(), point.y(), colorRgb);
            }
        }
    }

    m_canvas->canvasImageRef() = preview;
    m_canvas->update();
}

void CircleTool::commit(bool filled)
{
    if (!m_canvas) return;

    m_canvas->canvasImageRef() = m_originalCanvas;
    int radius = currentRadius();

    if (filled) {
        for (int y = m_center.y() - radius; y <= m_center.y() + radius; ++y) {
            for (int x = m_center.x() - radius; x <= m_center.x() + radius; ++x) {
                int dx = x - m_center.x();
                int dy = y - m_center.y();
                if (dx * dx + dy * dy <= radius * radius) {
                    m_canvas->setPixel(x, y, m_color);
                }
            }
        }
        m_canvas->update();
    } else {
        QVector<QPoint> points = getCirclePoints(m_center, radius);
        for (const QPoint &point : points) {
            drawPixel(point.x(), point.y(), m_color);
        }
    }
}

void CircleTool::onDeactivate()
{
    if (m_drawing) {
        cancelHistoryOperation();
        m_drawing = false;
    }
}
