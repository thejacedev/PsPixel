#include "erasertool.h"
#include "constants.h"

using namespace PixelPaint;

EraserTool::EraserTool(QObject *parent)
    : BaseTool(parent)
{
}

void EraserTool::onMousePress(const QPoint &canvasPos, Qt::MouseButton button)
{
    if (button == Qt::LeftButton) {
        // Start history operation before erasing
        startHistoryOperation();
        
        m_isDrawing = true;
        m_lastPoint = canvasPos;
        drawPixel(canvasPos.x(), canvasPos.y(), Qt::transparent);
    }
}

void EraserTool::onMouseMove(const QPoint &canvasPos, Qt::MouseButtons buttons)
{
    if (m_isDrawing && (buttons & Qt::LeftButton)) {
        if (canvasPos != m_lastPoint) {
            drawLine(m_lastPoint, canvasPos);
            m_lastPoint = canvasPos;
        }
    }
}

void EraserTool::onMouseRelease(const QPoint &canvasPos, Qt::MouseButton button)
{
    Q_UNUSED(canvasPos)
    if (button == Qt::LeftButton && m_isDrawing) {
        m_isDrawing = false;
        // Commit the erase operation to history
        commitHistoryOperation("Erase");
    }
}

void EraserTool::drawLine(const QPoint &from, const QPoint &to)
{
    QVector<QPoint> points = getLinePoints(from, to);
    
    for (const QPoint &point : points) {
        drawPixel(point.x(), point.y(), Qt::transparent);
    }
} 
