#include "selecttool.h"
#include "pixelcanvas.h"
#include <QPainter>

using namespace PixelPaint;

SelectTool::SelectTool(QObject *parent)
    : BaseTool(parent)
    , m_phase(Phase::None)
    , m_hasSelection(false)
{
}

void SelectTool::onMousePress(const QPoint &canvasPos, Qt::MouseButton button)
{
    if (button != Qt::LeftButton || !m_canvas) return;

    if (m_hasSelection && m_selectionRect.contains(canvasPos)) {
        // Clicked inside selection — start moving
        m_phase = Phase::Moving;
        m_moveStart = canvasPos;
        startHistoryOperation();

        // Save canvas state and clear the selection area on canvas
        m_canvasBeforeMove = m_canvas->canvasImage();
        QImage &img = m_canvas->canvasImageRef();
        for (int y = m_selectionRect.top(); y <= m_selectionRect.bottom(); ++y) {
            for (int x = m_selectionRect.left(); x <= m_selectionRect.right(); ++x) {
                if (isValidPosition(x, y)) {
                    img.setPixelColor(x, y, QColor(0, 0, 0, 0));
                }
            }
        }
        m_canvas->update();
    } else {
        // Commit any existing selection first
        if (m_hasSelection) {
            commitSelection();
        }

        // Start new selection
        m_phase = Phase::Selecting;
        m_selectStart = canvasPos;
        m_selectEnd = canvasPos;
        m_hasSelection = false;
    }
}

void SelectTool::onMouseMove(const QPoint &canvasPos, Qt::MouseButtons buttons)
{
    if (!(buttons & Qt::LeftButton) || !m_canvas) return;

    if (m_phase == Phase::Selecting) {
        m_selectEnd = canvasPos;
        // Update visual selection rect
        m_selectionRect = QRect(
            QPoint(qMin(m_selectStart.x(), m_selectEnd.x()), qMin(m_selectStart.y(), m_selectEnd.y())),
            QPoint(qMax(m_selectStart.x(), m_selectEnd.x()), qMax(m_selectStart.y(), m_selectEnd.y()))
        );
        m_hasSelection = true;
        m_canvas->update();

    } else if (m_phase == Phase::Moving) {
        // Move the selection
        QPoint delta = canvasPos - m_moveStart;
        m_selectionRect.translate(delta);
        m_moveStart = canvasPos;

        // Redraw: start from canvas-before-move with selection area cleared, then paste snippet
        QImage preview = m_canvasBeforeMove;

        // Clear original selection area
        QRect origRect(
            QPoint(qMin(m_selectStart.x(), m_selectEnd.x()), qMin(m_selectStart.y(), m_selectEnd.y())),
            QPoint(qMax(m_selectStart.x(), m_selectEnd.x()), qMax(m_selectStart.y(), m_selectEnd.y()))
        );
        for (int y = origRect.top(); y <= origRect.bottom(); ++y) {
            for (int x = origRect.left(); x <= origRect.right(); ++x) {
                if (x >= 0 && x < m_canvas->getCanvasWidth() && y >= 0 && y < m_canvas->getCanvasHeight()) {
                    preview.setPixelColor(x, y, QColor(0, 0, 0, 0));
                }
            }
        }

        // Paste selection at new position
        QPainter p(&preview);
        p.setCompositionMode(QPainter::CompositionMode_SourceOver);
        p.drawImage(m_selectionRect.topLeft(), m_selectionPixels);
        p.end();

        m_canvas->canvasImageRef() = preview;
        m_canvas->update();
    }
}

void SelectTool::onMouseRelease(const QPoint &canvasPos, Qt::MouseButton button)
{
    if (button != Qt::LeftButton || !m_canvas) return;

    if (m_phase == Phase::Selecting) {
        m_selectEnd = canvasPos;
        m_selectionRect = QRect(
            QPoint(qMin(m_selectStart.x(), m_selectEnd.x()), qMin(m_selectStart.y(), m_selectEnd.y())),
            QPoint(qMax(m_selectStart.x(), m_selectEnd.x()), qMax(m_selectStart.y(), m_selectEnd.y()))
        );

        // Clamp to canvas bounds
        m_selectionRect = m_selectionRect.intersected(
            QRect(0, 0, m_canvas->getCanvasWidth(), m_canvas->getCanvasHeight()));

        if (m_selectionRect.width() > 1 && m_selectionRect.height() > 1) {
            m_hasSelection = true;
            // Copy the selected pixels
            m_selectionPixels = m_canvas->canvasImage().copy(m_selectionRect);
        } else {
            m_hasSelection = false;
        }
        m_phase = Phase::None;
        m_canvas->update();

    } else if (m_phase == Phase::Moving) {
        commitSelection();
        commitHistoryOperation("Move Selection");
        emit canvasModified();
        m_phase = Phase::None;
    }
}

void SelectTool::commitSelection()
{
    // Selection is already painted on the canvas from the move preview
    m_hasSelection = false;
    m_selectionPixels = QImage();
    m_canvasBeforeMove = QImage();
    if (m_canvas) m_canvas->update();
}

void SelectTool::clearSelection()
{
    m_hasSelection = false;
    m_selectionPixels = QImage();
    m_canvasBeforeMove = QImage();
    m_phase = Phase::None;
}

void SelectTool::onDeactivate()
{
    if (m_phase == Phase::Moving) {
        cancelHistoryOperation();
    }
    clearSelection();
    if (m_canvas) m_canvas->update();
}
