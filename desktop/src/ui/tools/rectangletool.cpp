#include "rectangletool.h"
#include "pixelcanvas.h"
#include <QApplication>

using namespace PixelPaint;

RectangleTool::RectangleTool(QObject *parent)
    : BaseTool(parent)
    , m_drawing(false)
    , m_filled(false)
{
}

void RectangleTool::onMousePress(const QPoint &canvasPos, Qt::MouseButton button)
{
    if (button == Qt::LeftButton && m_canvas) {
        startHistoryOperation();
        m_drawing = true;
        m_startPoint = canvasPos;
        m_endPoint = canvasPos;
        m_filled = QApplication::keyboardModifiers() & Qt::ShiftModifier;
        m_originalCanvas = m_canvas->canvasImage();
    }
}

void RectangleTool::onMouseMove(const QPoint &canvasPos, Qt::MouseButtons buttons)
{
    if (m_drawing && (buttons & Qt::LeftButton)) {
        m_endPoint = canvasPos;
        m_filled = QApplication::keyboardModifiers() & Qt::ShiftModifier;
        drawPreview(m_filled);
    }
}

void RectangleTool::onMouseRelease(const QPoint &canvasPos, Qt::MouseButton button)
{
    if (button == Qt::LeftButton && m_drawing) {
        m_endPoint = canvasPos;
        m_filled = QApplication::keyboardModifiers() & Qt::ShiftModifier;
        commit(m_filled);
        m_drawing = false;
        commitHistoryOperation("Draw Rectangle");
        emit canvasModified();
    }
}

void RectangleTool::drawPreview(bool filled)
{
    if (!m_canvas) return;

    QImage preview = m_originalCanvas;
    QRgb colorRgb = m_color.rgba();

    int left = qMin(m_startPoint.x(), m_endPoint.x());
    int right = qMax(m_startPoint.x(), m_endPoint.x());
    int top = qMin(m_startPoint.y(), m_endPoint.y());
    int bottom = qMax(m_startPoint.y(), m_endPoint.y());

    if (filled) {
        // Fill entire rectangle
        for (int y = top; y <= bottom; ++y) {
            for (int x = left; x <= right; ++x) {
                if (isValidPosition(x, y)) {
                    preview.setPixel(x, y, colorRgb);
                }
            }
        }
    } else {
        // Outline only
        QVector<QPoint> points = getRectanglePoints(m_startPoint, m_endPoint);
        for (const QPoint &point : points) {
            if (isValidPosition(point.x(), point.y())) {
                preview.setPixel(point.x(), point.y(), colorRgb);
            }
        }
    }

    m_canvas->canvasImageRef() = preview;
    m_canvas->update();
}

void RectangleTool::commit(bool filled)
{
    if (!m_canvas) return;

    m_canvas->canvasImageRef() = m_originalCanvas;

    int left = qMin(m_startPoint.x(), m_endPoint.x());
    int right = qMax(m_startPoint.x(), m_endPoint.x());
    int top = qMin(m_startPoint.y(), m_endPoint.y());
    int bottom = qMax(m_startPoint.y(), m_endPoint.y());

    if (filled) {
        for (int y = top; y <= bottom; ++y) {
            for (int x = left; x <= right; ++x) {
                m_canvas->setPixel(x, y, m_color);
            }
        }
        m_canvas->update();
    } else {
        QVector<QPoint> points = getRectanglePoints(m_startPoint, m_endPoint);
        for (const QPoint &point : points) {
            drawPixel(point.x(), point.y(), m_color);
        }
    }
}

void RectangleTool::onDeactivate()
{
    if (m_drawing) {
        cancelHistoryOperation();
        m_drawing = false;
    }
}
