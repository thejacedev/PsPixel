#include "basetool.h"
#include "pixelcanvas.h"
#include "toolmanager.h"
#include "../history/historymanager.h"
#include <QtMath>

using namespace PixelPaint;

BaseTool::BaseTool(QObject *parent)
    : QObject(parent)
    , m_color(Qt::black)
    , m_brushSize(1)
    , m_canvas(nullptr)
    , m_isDrawing(false)
    , m_operationInProgress(false)
{
}

void BaseTool::setColor(const QColor &color)
{
    m_color = color;
}

void BaseTool::setBrushSize(int size)
{
    m_brushSize = qBound(MIN_BRUSH_SIZE, size, MAX_BRUSH_SIZE);
}

void BaseTool::setCanvas(PixelCanvas *canvas)
{
    m_canvas = canvas;
}

void BaseTool::drawPixel(int x, int y, const QColor &color)
{
    if (!m_canvas) return;

    const int halfSize = m_brushSize / 2;

    for (int dy = -halfSize; dy <= halfSize; ++dy) {
        for (int dx = -halfSize; dx <= halfSize; ++dx) {
            int px = x + dx;
            int py = y + dy;

            if (isValidPosition(px, py)) {
                if (m_brushSize > 1) {
                    const double distance = qSqrt(dx * dx + dy * dy);
                    if (distance > halfSize + 0.5) continue;
                }

                m_canvas->setPixel(px, py, color);
            }
        }
    }

    m_canvas->update();
    emit canvasModified();
}

bool BaseTool::isValidPosition(int x, int y) const
{
    if (!m_canvas) return false;
    return x >= 0 && x < m_canvas->getCanvasWidth() &&
           y >= 0 && y < m_canvas->getCanvasHeight();
}

QVector<QPoint> BaseTool::getLinePoints(const QPoint &start, const QPoint &end)
{
    QVector<QPoint> points;

    int x0 = start.x();
    int y0 = start.y();
    int x1 = end.x();
    int y1 = end.y();

    // Bresenham's line algorithm
    int dx = qAbs(x1 - x0);
    int dy = qAbs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    int x = x0;
    int y = y0;

    while (true) {
        points.append(QPoint(x, y));

        if (x == x1 && y == y1) break;

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }

    return points;
}

QVector<QPoint> BaseTool::getCirclePoints(const QPoint &center, int radius)
{
    QVector<QPoint> points;

    int x = 0;
    int y = radius;
    int d = 3 - 2 * radius;

    auto addCirclePoints = [&](int cx, int cy, int x, int y) {
        points.append(QPoint(cx + x, cy + y));
        points.append(QPoint(cx - x, cy + y));
        points.append(QPoint(cx + x, cy - y));
        points.append(QPoint(cx - x, cy - y));
        points.append(QPoint(cx + y, cy + x));
        points.append(QPoint(cx - y, cy + x));
        points.append(QPoint(cx + y, cy - x));
        points.append(QPoint(cx - y, cy - x));
    };

    addCirclePoints(center.x(), center.y(), x, y);

    while (y >= x) {
        x++;

        if (d > 0) {
            y--;
            d = d + 4 * (x - y) + 10;
        } else {
            d = d + 4 * x + 6;
        }

        addCirclePoints(center.x(), center.y(), x, y);
    }

    return points;
}

QVector<QPoint> BaseTool::getRectanglePoints(const QPoint &topLeft, const QPoint &bottomRight)
{
    QVector<QPoint> points;

    int left = qMin(topLeft.x(), bottomRight.x());
    int right = qMax(topLeft.x(), bottomRight.x());
    int top = qMin(topLeft.y(), bottomRight.y());
    int bottom = qMax(topLeft.y(), bottomRight.y());

    for (int x = left; x <= right; ++x) {
        points.append(QPoint(x, top));
        if (top != bottom) {
            points.append(QPoint(x, bottom));
        }
    }

    for (int y = top + 1; y < bottom; ++y) {
        points.append(QPoint(left, y));
        if (left != right) {
            points.append(QPoint(right, y));
        }
    }

    return points;
}

void BaseTool::saveCanvasState(const QString &actionName)
{
    if (!m_canvas) return;

    ToolManager *toolManager = qobject_cast<ToolManager*>(parent());
    if (toolManager) {
        HistoryManager *historyManager = toolManager->getHistoryManager();
        if (historyManager) {
            historyManager->saveState(m_canvas->getCanvasState(), actionName);
        }
    }
}

void BaseTool::startHistoryOperation()
{
    if (!m_canvas) return;
    if (m_operationInProgress) return;

    ToolManager *toolManager = qobject_cast<ToolManager*>(parent());
    if (toolManager) {
        HistoryManager *historyManager = toolManager->getHistoryManager();
        if (historyManager) {
            historyManager->startOperation(m_canvas->getCanvasState());
            m_operationInProgress = true;
        }
    }
}

void BaseTool::commitHistoryOperation(const QString &actionName)
{
    if (!m_canvas) return;
    if (!m_operationInProgress) return;

    ToolManager *toolManager = qobject_cast<ToolManager*>(parent());
    if (toolManager) {
        HistoryManager *historyManager = toolManager->getHistoryManager();
        if (historyManager) {
            historyManager->commitOperation(m_canvas->getCanvasState(), actionName);
            m_operationInProgress = false;
        }
    }
}

void BaseTool::cancelHistoryOperation()
{
    if (!m_operationInProgress) {
        return;
    }

    ToolManager *toolManager = qobject_cast<ToolManager*>(parent());
    if (toolManager) {
        HistoryManager *historyManager = toolManager->getHistoryManager();
        if (historyManager) {
            historyManager->cancelOperation();
            m_operationInProgress = false;
        }
    }
}
