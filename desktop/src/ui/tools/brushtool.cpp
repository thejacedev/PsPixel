#include "brushtool.h"

using namespace PixelPaint;

BrushTool::BrushTool(QObject *parent)
    : BaseTool(parent)
{
}

void BrushTool::onMousePress(const QPoint &canvasPos, Qt::MouseButton button)
{
    if (button == Qt::LeftButton) {
        // Start history operation before drawing
        startHistoryOperation();
        
        m_isDrawing = true;
        m_lastPoint = canvasPos;
        drawPixel(canvasPos.x(), canvasPos.y(), m_color);
    }
}

void BrushTool::onMouseMove(const QPoint &canvasPos, Qt::MouseButtons buttons)
{
    if (m_isDrawing && (buttons & Qt::LeftButton)) {
        if (canvasPos != m_lastPoint) {
            drawLine(m_lastPoint, canvasPos);
            m_lastPoint = canvasPos;
        }
    }
}

void BrushTool::onMouseRelease(const QPoint &canvasPos, Qt::MouseButton button)
{
    Q_UNUSED(canvasPos)
    if (button == Qt::LeftButton && m_isDrawing) {
        m_isDrawing = false;
        // Commit the brush stroke to history
        commitHistoryOperation("Brush Stroke");
    }
}

void BrushTool::drawLine(const QPoint &from, const QPoint &to)
{
    QVector<QPoint> points = getLinePoints(from, to);
    
    for (const QPoint &point : points) {
        drawPixel(point.x(), point.y(), m_color);
    }
} 