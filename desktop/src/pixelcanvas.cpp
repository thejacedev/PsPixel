#include "pixelcanvas.h"
#include "ui/tools/toolmanager.h"
#include "ui/tools/basetool.h"
#include "ui/tools/selecttool.h"
#include "ui/tools/lassotool.h"
#include "ui/layers/layermanager.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QEnterEvent>
#include <QPixmap>
#include <QImage>
#include <QtMath>
#include <QCursor>
#include <QPainterPath>

using namespace PixelPaint;

PixelCanvas::PixelCanvas(QWidget *parent)
    : QWidget(parent)
    , m_canvasWidth(DEFAULT_CANVAS_WIDTH)
    , m_canvasHeight(DEFAULT_CANVAS_HEIGHT)
    , m_pixelSize(DEFAULT_PIXEL_SIZE)
    , m_brushSize(DEFAULT_BRUSH_SIZE)
    , m_zoomFactor(DEFAULT_ZOOM_FACTOR)
    , m_panOffset(0, 0)
    , m_panning(false)
    , m_currentColor(Qt::black)
    , m_painting(false)
    , m_toolManager(nullptr)
    , m_layerManager(nullptr)
    , m_mousePosition(-1, -1)
    , m_mouseOnCanvas(false)
    , m_previewStartPoint(-1, -1)
    , m_mirrorH(false)
    , m_mirrorV(false)
    , m_hasSelection(false)
    , m_refOffset(0, 0)
    , m_refOpacity(1.0)
    , m_refScale(1.0)
    , m_refLocked(false)
    , m_refActive(false)
    , m_refDragging(false)
    , m_refResizing(false)
    , m_refResizeStartScale(1.0)
    , m_refResizeCorner(3)
{
    initializeCanvas();
    updateCanvasSize();
    setMouseTracking(true);
}

void PixelCanvas::initializeCanvas()
{
    m_canvas = QImage(m_canvasWidth, m_canvasHeight, QImage::Format_ARGB32);
    m_canvas.fill(Qt::transparent);
    regenerateCheckerboard();
}

void PixelCanvas::regenerateCheckerboard()
{
    m_checkerboard = QImage(m_canvasWidth, m_canvasHeight, QImage::Format_ARGB32);
    const int checkSize = qMax(1, 4); // 4-pixel checkerboard squares
    for (int y = 0; y < m_canvasHeight; ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(m_checkerboard.scanLine(y));
        for (int x = 0; x < m_canvasWidth; ++x) {
            const bool even = ((x / checkSize) + (y / checkSize)) % 2 == 0;
            line[x] = even ? CHECKERBOARD_LIGHT.rgba() : CHECKERBOARD_DARK.rgba();
        }
    }
}

void PixelCanvas::updateCanvasSize()
{
    // Don't resize the widget - let it fill the available space in the layout
    // The canvas will be drawn centered within whatever space is available
}

void PixelCanvas::setToolManager(ToolManager *toolManager)
{
    m_toolManager = toolManager;
}

void PixelCanvas::setLayerManager(LayerManager *layerManager)
{
    m_layerManager = layerManager;
    if (m_layerManager) {
        connect(m_layerManager, &LayerManager::layersChanged, this, &PixelCanvas::updateComposite);
    }
}

void PixelCanvas::updateComposite()
{
    if (m_layerManager) {
        m_canvas = m_layerManager->compositeAll(m_canvasWidth, m_canvasHeight);
    }
    update();
}

void PixelCanvas::setCurrentColor(const QColor &color)
{
    m_currentColor = color;
}

void PixelCanvas::setBrushSize(int size)
{
    m_brushSize = qBound(MIN_BRUSH_SIZE, size, MAX_BRUSH_SIZE);
}

void PixelCanvas::clearCanvas()
{
    m_canvas.fill(Qt::transparent);
    clearSelection();
    update();
}

bool PixelCanvas::saveImage(const QString &fileName)
{
    return m_canvas.save(fileName);
}

bool PixelCanvas::loadImage(const QString &fileName)
{
    QImage image(fileName);
    if (image.isNull()) {
        return false;
    }

    m_canvas = image.scaled(m_canvasWidth, m_canvasHeight, Qt::IgnoreAspectRatio)
                     .convertToFormat(QImage::Format_ARGB32);
    regenerateCheckerboard();
    update();
    return true;
}

void PixelCanvas::setPixel(int x, int y, const QColor &color)
{
    if (x < 0 || x >= m_canvasWidth || y < 0 || y >= m_canvasHeight) return;

    // If a selection exists, only allow writes inside it
    if (m_hasSelection && !isPixelSelected(x, y)) return;

    // Write to active layer if layer manager exists, otherwise to m_canvas
    QImage &target = m_layerManager ? m_layerManager->activeLayer().image : m_canvas;

    target.setPixelColor(x, y, color);

    if (m_mirrorH) {
        int mx = m_canvasWidth - 1 - x;
        if (mx >= 0 && mx < m_canvasWidth)
            target.setPixelColor(mx, y, color);
    }
    if (m_mirrorV) {
        int my = m_canvasHeight - 1 - y;
        if (my >= 0 && my < m_canvasHeight)
            target.setPixelColor(x, my, color);
    }
    if (m_mirrorH && m_mirrorV) {
        int mx = m_canvasWidth - 1 - x;
        int my = m_canvasHeight - 1 - y;
        if (mx >= 0 && mx < m_canvasWidth && my >= 0 && my < m_canvasHeight)
            target.setPixelColor(mx, my, color);
    }
}

QColor PixelCanvas::pixelAt(int x, int y) const
{
    if (x >= 0 && x < m_canvasWidth && y >= 0 && y < m_canvasHeight) {
        if (m_layerManager) {
            return m_layerManager->activeLayer().image.pixelColor(x, y);
        }
        return m_canvas.pixelColor(x, y);
    }
    return QColor();
}

// Selection mask methods
void PixelCanvas::setSelectionMask(const QImage &mask)
{
    m_selectionMask = mask;
    m_hasSelection = !mask.isNull();
    update();
}

void PixelCanvas::clearSelection()
{
    m_selectionMask = QImage();
    m_hasSelection = false;
    update();
}

bool PixelCanvas::isPixelSelected(int x, int y) const
{
    if (!m_hasSelection || m_selectionMask.isNull()) return true;
    if (x < 0 || x >= m_selectionMask.width() || y < 0 || y >= m_selectionMask.height()) return false;
    return qGray(m_selectionMask.pixel(x, y)) > 127;
}

QRect PixelCanvas::selectionBoundingRect() const
{
    if (!m_hasSelection || m_selectionMask.isNull()) return QRect();

    int minX = m_selectionMask.width(), minY = m_selectionMask.height();
    int maxX = -1, maxY = -1;

    for (int y = 0; y < m_selectionMask.height(); ++y) {
        for (int x = 0; x < m_selectionMask.width(); ++x) {
            if (qGray(m_selectionMask.pixel(x, y)) > 127) {
                minX = qMin(minX, x);
                minY = qMin(minY, y);
                maxX = qMax(maxX, x);
                maxY = qMax(maxY, y);
            }
        }
    }

    if (maxX < 0) return QRect();
    return QRect(minX, minY, maxX - minX + 1, maxY - minY + 1);
}

void PixelCanvas::loadReferenceImage(const QString &filePath)
{
    QImage img(filePath);
    if (img.isNull()) return;
    m_refImage = img.convertToFormat(QImage::Format_ARGB32);
    m_refOffset = QPointF(0, 0);
    m_refScale = 1.0;
    update();
    emit referenceImageChanged();
}

void PixelCanvas::clearReferenceImage()
{
    m_refImage = QImage();
    m_refActive = false;
    update();
    emit referenceImageChanged();
}

QColor PixelCanvas::referencePixelAt(int x, int y) const
{
    if (m_refImage.isNull() || m_refScale <= 0) return QColor();

    // Canvas pixel (x,y) → reference image pixel, accounting for offset and scale
    // The ref is drawn at screen pos (offset * pixelSize) with size (imgSize * scale)
    // In canvas-pixel-size space: refX = offset.x * pixelSize, so in canvas-pixel coords
    // the ref starts at offset.x and each ref pixel covers (scale / pixelSize) canvas pixels
    // But the ref is drawn outside the pixelSize scale, so 1 ref pixel = (scale) canvas-pixel-size units
    // In canvas pixel coords: ref pixel = (canvasPixel - offset) * pixelSize / scale

    double rx = (x - m_refOffset.x()) * m_pixelSize / m_refScale;
    double ry = (y - m_refOffset.y()) * m_pixelSize / m_refScale;

    int ix = static_cast<int>(rx);
    int iy = static_cast<int>(ry);

    if (ix < 0 || iy < 0 || ix >= m_refImage.width() || iy >= m_refImage.height()) {
        return QColor();
    }
    return m_refImage.pixelColor(ix, iy);
}

void PixelCanvas::resizeCanvas(int width, int height)
{
    QImage oldCanvas = m_canvas;

    m_canvasWidth = qBound(MIN_CANVAS_SIZE, width, MAX_CANVAS_SIZE);
    m_canvasHeight = qBound(MIN_CANVAS_SIZE, height, MAX_CANVAS_SIZE);

    m_canvas = QImage(m_canvasWidth, m_canvasHeight, QImage::Format_ARGB32);
    m_canvas.fill(Qt::transparent);

    // Blit old content into new canvas (preserves top-left)
    QPainter p(&m_canvas);
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.drawImage(0, 0, oldCanvas);
    p.end();

    clearSelection();
    regenerateCheckerboard();
    updateCanvasSize();
    update();
    updateGeometry();
}

void PixelCanvas::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    // Fill background with neutral gray
    painter.fillRect(rect(), CANVAS_BACKGROUND);

    // Calculate canvas positioning
    const int canvasPixelWidth = m_canvasWidth * m_pixelSize;
    const int canvasPixelHeight = m_canvasHeight * m_pixelSize;
    const int scaledCanvasWidth = static_cast<int>(canvasPixelWidth * m_zoomFactor);
    const int scaledCanvasHeight = static_cast<int>(canvasPixelHeight * m_zoomFactor);

    const int centerX = width() / 2;
    const int centerY = height() / 2;
    const int canvasStartX = centerX - scaledCanvasWidth / 2;
    const int canvasStartY = centerY - scaledCanvasHeight / 2;

    // Apply transformations: center + pan + zoom
    painter.translate(canvasStartX + m_panOffset.x(), canvasStartY + m_panOffset.y());
    painter.scale(m_zoomFactor, m_zoomFactor);

    // Scale by pixelSize so each canvas pixel occupies pixelSize screen units
    painter.save();
    painter.scale(m_pixelSize, m_pixelSize);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false); // Keep pixels crisp

    // Draw checkerboard (for transparency visualization)
    painter.drawImage(0, 0, m_checkerboard);

    // Draw canvas (composited layers or single canvas)
    if (m_layerManager) {
        QImage composite = m_layerManager->compositeAll(m_canvasWidth, m_canvasHeight);
        painter.drawImage(0, 0, composite);
    } else {
        painter.drawImage(0, 0, m_canvas);
    }
    painter.restore();

    // Draw reference image with blue bounding box
    if (!m_refImage.isNull()) {
        double drawX = m_refOffset.x() * m_pixelSize;
        double drawY = m_refOffset.y() * m_pixelSize;
        double drawW = m_refImage.width() * m_refScale;
        double drawH = m_refImage.height() * m_refScale;
        QRectF imgRect(drawX, drawY, drawW, drawH);

        painter.save();
        painter.setOpacity(m_refOpacity);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.drawImage(imgRect, m_refImage);
        painter.setOpacity(1.0);

        // Blue bounding box
        QPen boxPen(ACCENT_COLOR, 2.0 / m_zoomFactor);
        boxPen.setStyle(m_refActive ? Qt::SolidLine : Qt::DashLine);
        painter.setPen(boxPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(imgRect);

        // Corner drag handles when active
        if (m_refActive) {
            double hs = 8.0 / m_zoomFactor;
            painter.setPen(QPen(Qt::white, 1.0 / m_zoomFactor));
            painter.setBrush(ACCENT_COLOR);
            painter.drawRect(QRectF(imgRect.left() - hs/2, imgRect.top() - hs/2, hs, hs));
            painter.drawRect(QRectF(imgRect.right() - hs/2, imgRect.top() - hs/2, hs, hs));
            painter.drawRect(QRectF(imgRect.left() - hs/2, imgRect.bottom() - hs/2, hs, hs));
            painter.drawRect(QRectF(imgRect.right() - hs/2, imgRect.bottom() - hs/2, hs, hs));
        }

        painter.restore();
    }

    // Draw grid (only if pixels are large enough to be visible)
    const double effectivePixelSize = m_pixelSize * m_zoomFactor;
    if (effectivePixelSize >= 4.0) {
        painter.setPen(QPen(Qt::lightGray, 1.0 / m_zoomFactor));

        for (int x = 0; x <= m_canvasWidth; ++x) {
            const int xPos = x * m_pixelSize;
            painter.drawLine(xPos, 0, xPos, m_canvasHeight * m_pixelSize);
        }

        for (int y = 0; y <= m_canvasHeight; ++y) {
            const int yPos = y * m_pixelSize;
            painter.drawLine(0, yPos, m_canvasWidth * m_pixelSize, yPos);
        }
    }

    // Draw selection marching ants (always, regardless of tool or mouse state)
    drawSelectOverlay(painter);

    // Draw tool overlay on top of everything
    drawToolOverlay(painter);
}

void PixelCanvas::mousePressEvent(QMouseEvent *event)
{
    // When reference layer is selected, handle move/resize — but only if clicking ON the ref
    if (m_refActive && !m_refImage.isNull() && event->button() == Qt::LeftButton) {
        QPointF canvasClick = widgetToCanvas(QPointF(event->pos()));
        double cx = canvasClick.x();
        double cy = canvasClick.y();
        double rw = m_refImage.width() * m_refScale;
        double rh = m_refImage.height() * m_refScale;
        double handleSize = 12.0 / m_zoomFactor;
        QRectF refRect(m_refOffset.x() * m_pixelSize, m_refOffset.y() * m_pixelSize, rw, rh);

        // Check corners first
        int corner = -1;
        if (QRectF(refRect.left() - handleSize, refRect.top() - handleSize, handleSize*2, handleSize*2).contains(cx, cy)) corner = 0;
        else if (QRectF(refRect.right() - handleSize, refRect.top() - handleSize, handleSize*2, handleSize*2).contains(cx, cy)) corner = 1;
        else if (QRectF(refRect.left() - handleSize, refRect.bottom() - handleSize, handleSize*2, handleSize*2).contains(cx, cy)) corner = 2;
        else if (QRectF(refRect.right() - handleSize, refRect.bottom() - handleSize, handleSize*2, handleSize*2).contains(cx, cy)) corner = 3;

        if (corner >= 0) {
            m_refResizing = true;
            m_refDragging = false;
            m_refResizeCorner = corner;
            m_refDragStart = event->pos();
            m_refResizeStartScale = m_refScale;
            m_refResizeStartOffset = m_refOffset;
            setCursor(Qt::SizeFDiagCursor);
            return;
        }

        // Body — intercept for move, UNLESS current tool is eyedropper
        bool isEyedropper = m_toolManager && m_toolManager->getCurrentToolType() == ToolType::Eyedropper;
        if (refRect.contains(cx, cy) && !isEyedropper) {
            m_refDragging = true;
            m_refResizing = false;
            m_refDragStart = event->pos();
            setCursor(Qt::ClosedHandCursor);
            return;
        }

        // Click is outside the ref image — fall through to tools below
    }

    // Handle panning with middle mouse button
    if (event->button() == Qt::MiddleButton) {
        m_panning = true;
        m_lastPanPoint = event->pos();
        setCursor(Qt::ClosedHandCursor);
        return;
    }

    // Delegate to current tool if available
    if (m_toolManager && m_toolManager->getCurrentTool()) {
        const QPoint gridPos = getGridPosition(event->pos());
        m_toolManager->getCurrentTool()->onMousePress(gridPos, event->button());
    }
    // Fallback to old behavior if no tool manager
    else if (event->button() == Qt::LeftButton) {
        m_painting = true;
        const QPoint gridPos = getGridPosition(event->pos());
        drawBrush(gridPos.x(), gridPos.y());
        m_lastPoint = gridPos;
    }
}

void PixelCanvas::mouseMoveEvent(QMouseEvent *event)
{
    // Update mouse position for tool overlay
    m_mousePosition = getGridPosition(event->pos());

    // Emit cursor position for status bar display
    if (m_mousePosition.x() >= 0 && m_mousePosition.x() < m_canvasWidth &&
        m_mousePosition.y() >= 0 && m_mousePosition.y() < m_canvasHeight) {
        emit cursorPositionChanged(m_mousePosition.x(), m_mousePosition.y());
    }

    // Handle reference image resize by dragging corner
    if (m_refResizing && m_refActive && (event->buttons() & Qt::LeftButton)) {
        QPoint delta = event->pos() - m_refDragStart;
        double screenToCanvas = m_zoomFactor * m_pixelSize;

        // Scale based on how far the corner moved diagonally
        double diag = (delta.x() + delta.y()) / screenToCanvas;
        double origW = m_refImage.width() * m_refResizeStartScale;
        double newScale = m_refResizeStartScale * (1.0 + diag / qMax(1.0, origW));
        m_refScale = qBound(0.05, newScale, 20.0);

        update();
        return;
    }

    // Handle reference image move by dragging body
    if (m_refDragging && m_refActive && (event->buttons() & Qt::LeftButton)) {
        QPointF delta = QPointF(event->pos() - m_refDragStart);
        double screenToCanvas = m_zoomFactor * m_pixelSize;
        m_refOffset += QPointF(delta.x() / screenToCanvas, delta.y() / screenToCanvas);
        m_refDragStart = event->pos();
        update();
        return;
    }

    // Update cursor when hovering ref image
    if (m_refActive && !m_refImage.isNull() && !(event->buttons())) {
        QPointF canvasClick = widgetToCanvas(QPointF(event->pos()));
        double cx = canvasClick.x(), cy = canvasClick.y();
        double rw = m_refImage.width() * m_refScale;
        double rh = m_refImage.height() * m_refScale;
        QRectF refRect(m_refOffset.x() * m_pixelSize, m_refOffset.y() * m_pixelSize, rw, rh);
        double hs = 12.0 / m_zoomFactor;

        bool onCorner =
            QRectF(refRect.left()-hs, refRect.top()-hs, hs*2, hs*2).contains(cx,cy) ||
            QRectF(refRect.right()-hs, refRect.top()-hs, hs*2, hs*2).contains(cx,cy) ||
            QRectF(refRect.left()-hs, refRect.bottom()-hs, hs*2, hs*2).contains(cx,cy) ||
            QRectF(refRect.right()-hs, refRect.bottom()-hs, hs*2, hs*2).contains(cx,cy);

        if (onCorner) setCursor(Qt::SizeFDiagCursor);
        else if (refRect.contains(cx, cy)) setCursor(Qt::OpenHandCursor);
        else setCursor(Qt::ArrowCursor);
    }

    // Handle panning
    if (m_panning && (event->buttons() & Qt::MiddleButton)) {
        const QPoint delta = event->pos() - m_lastPanPoint;
        m_panOffset += delta;
        m_lastPanPoint = event->pos();
        update();
        return;
    }

    // Delegate to current tool if available
    if (m_toolManager && m_toolManager->getCurrentTool()) {
        const QPoint gridPos = getGridPosition(event->pos());
        m_toolManager->getCurrentTool()->onMouseMove(gridPos, event->buttons());
    }
    // Fallback to old behavior if no tool manager
    else if (m_painting && (event->buttons() & Qt::LeftButton)) {
        const QPoint gridPos = getGridPosition(event->pos());
        if (gridPos != m_lastPoint) {
            drawBrush(gridPos.x(), gridPos.y());
            m_lastPoint = gridPos;
        }
    }

    // Always update for tool overlay
    update();
}

void PixelCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    // Handle reference image drag/resize end
    if (event->button() == Qt::LeftButton && (m_refDragging || m_refResizing)) {
        m_refDragging = false;
        m_refResizing = false;
        setCursor(m_refActive ? Qt::OpenHandCursor : Qt::ArrowCursor);
        return;
    }

    // Handle panning
    if (event->button() == Qt::MiddleButton) {
        m_panning = false;
        setCursor(Qt::ArrowCursor);
        return;
    }

    // Delegate to current tool if available
    if (m_toolManager && m_toolManager->getCurrentTool()) {
        const QPoint gridPos = getGridPosition(event->pos());
        m_toolManager->getCurrentTool()->onMouseRelease(gridPos, event->button());
    }
    // Fallback to old behavior if no tool manager
    else if (event->button() == Qt::LeftButton) {
        m_painting = false;
    }
}

void PixelCanvas::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        const QPointF mousePos = event->position();
        const QPointF canvasPosBefore = widgetToCanvas(mousePos);

        if (event->angleDelta().y() > 0) {
            setZoomFactor(m_zoomFactor * ZOOM_WHEEL_FACTOR);
        } else {
            setZoomFactor(m_zoomFactor / ZOOM_WHEEL_FACTOR);
        }

        const QPointF canvasPosAfter = widgetToCanvas(mousePos);
        const QPointF canvasDelta = canvasPosBefore - canvasPosAfter;
        m_panOffset += QPointF(canvasDelta.x() * m_zoomFactor, canvasDelta.y() * m_zoomFactor);

        update();
        event->accept();
    } else {
        event->ignore();
    }
}

void PixelCanvas::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event)
    m_mouseOnCanvas = true;
    update();
}

void PixelCanvas::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    m_mouseOnCanvas = false;
    m_mousePosition = QPoint(-1, -1);
    emit cursorLeftCanvas();
    update();
}

// Zoom and Pan methods
void PixelCanvas::zoomIn()
{
    setZoomFactor(m_zoomFactor * ZOOM_WHEEL_FACTOR);
}

void PixelCanvas::zoomOut()
{
    setZoomFactor(m_zoomFactor / ZOOM_WHEEL_FACTOR);
}

void PixelCanvas::resetZoom()
{
    setZoomFactor(DEFAULT_ZOOM_FACTOR);
}

void PixelCanvas::resetPan()
{
    m_panOffset = QPointF(0, 0);
    update();
}

void PixelCanvas::setZoomFactor(double factor)
{
    double oldFactor = m_zoomFactor;
    m_zoomFactor = qBound(MIN_ZOOM_FACTOR, factor, MAX_ZOOM_FACTOR);

    if (oldFactor != m_zoomFactor) {
        emit zoomFactorChanged(m_zoomFactor);
    }

    updateCanvasSize();
    update();
}

// Coordinate transformation methods
QPointF PixelCanvas::canvasToWidget(const QPointF &canvasPos) const
{
    const int canvasPixelWidth = m_canvasWidth * m_pixelSize;
    const int canvasPixelHeight = m_canvasHeight * m_pixelSize;
    const int scaledCanvasWidth = static_cast<int>(canvasPixelWidth * m_zoomFactor);
    const int scaledCanvasHeight = static_cast<int>(canvasPixelHeight * m_zoomFactor);

    const int centerX = width() / 2;
    const int centerY = height() / 2;
    const int canvasStartX = centerX - scaledCanvasWidth / 2;
    const int canvasStartY = centerY - scaledCanvasHeight / 2;

    return QPointF(canvasPos.x() * m_zoomFactor + canvasStartX + m_panOffset.x(),
                   canvasPos.y() * m_zoomFactor + canvasStartY + m_panOffset.y());
}

QPointF PixelCanvas::widgetToCanvas(const QPointF &widgetPos) const
{
    const int canvasPixelWidth = m_canvasWidth * m_pixelSize;
    const int canvasPixelHeight = m_canvasHeight * m_pixelSize;
    const int scaledCanvasWidth = static_cast<int>(canvasPixelWidth * m_zoomFactor);
    const int scaledCanvasHeight = static_cast<int>(canvasPixelHeight * m_zoomFactor);

    const int centerX = width() / 2;
    const int centerY = height() / 2;
    const int canvasStartX = centerX - scaledCanvasWidth / 2;
    const int canvasStartY = centerY - scaledCanvasHeight / 2;

    return QPointF((widgetPos.x() - canvasStartX - m_panOffset.x()) / m_zoomFactor,
                   (widgetPos.y() - canvasStartY - m_panOffset.y()) / m_zoomFactor);
}

void PixelCanvas::drawBrush(int centerX, int centerY)
{
    const int halfSize = m_brushSize / 2;

    for (int dy = -halfSize; dy <= halfSize; ++dy) {
        for (int dx = -halfSize; dx <= halfSize; ++dx) {
            if (m_brushSize > 1) {
                const double distance = qSqrt(dx * dx + dy * dy);
                if (distance > halfSize + 0.5) continue;
            }

            setPixel(centerX + dx, centerY + dy, m_currentColor);
        }
    }
    update();
}

QPoint PixelCanvas::getGridPosition(const QPoint &mousePos)
{
    const QPointF canvasPos = widgetToCanvas(QPointF(mousePos));

    const int x = qBound(0, static_cast<int>(canvasPos.x() / m_pixelSize), m_canvasWidth - 1);
    const int y = qBound(0, static_cast<int>(canvasPos.y() / m_pixelSize), m_canvasHeight - 1);

    return QPoint(x, y);
}

QVector<QVector<QColor>> PixelCanvas::getPixelData() const
{
    QVector<QVector<QColor>> data(m_canvasHeight);
    for (int y = 0; y < m_canvasHeight; ++y) {
        data[y].resize(m_canvasWidth);
        for (int x = 0; x < m_canvasWidth; ++x) {
            data[y][x] = m_canvas.pixelColor(x, y);
        }
    }
    return data;
}

void PixelCanvas::setPixelData(const QVector<QVector<QColor>> &data)
{
    if (data.isEmpty() || data[0].isEmpty()) return;

    m_canvasHeight = data.size();
    m_canvasWidth = data[0].size();
    m_canvas = QImage(m_canvasWidth, m_canvasHeight, QImage::Format_ARGB32);
    m_canvas.fill(Qt::transparent);

    for (int y = 0; y < m_canvasHeight; ++y) {
        for (int x = 0; x < m_canvasWidth; ++x) {
            m_canvas.setPixelColor(x, y, data[y][x]);
        }
    }

    regenerateCheckerboard();
    updateCanvasSize();
    update();
}

// Tool overlay implementations
void PixelCanvas::drawToolOverlay(QPainter &painter)
{
    if (!m_mouseOnCanvas || !m_toolManager || m_mousePosition.x() < 0 || m_mousePosition.y() < 0) {
        return;
    }

    if (m_mousePosition.x() >= m_canvasWidth || m_mousePosition.y() >= m_canvasHeight) {
        return;
    }

    ToolType currentTool = m_toolManager->getCurrentToolType();

    switch (currentTool) {
        case ToolType::Brush:
            drawBrushOverlay(painter, m_mousePosition);
            break;
        case ToolType::Eraser:
            drawEraserOverlay(painter, m_mousePosition);
            break;
        case ToolType::Eyedropper:
            drawEyedropperOverlay(painter, m_mousePosition);
            break;
        case ToolType::PaintBucket:
            drawPaintBucketOverlay(painter, m_mousePosition);
            break;
        case ToolType::Line:
            drawLineToolOverlay(painter, m_mousePosition);
            break;
        case ToolType::Select:
        case ToolType::MagicWand:
        case ToolType::Lasso:
        default:
            break;
    }
}

void PixelCanvas::drawBrushOverlay(QPainter &painter, const QPoint &gridPos)
{
    const int halfSize = m_brushSize / 2;
    const QColor overlayColor = m_currentColor;

    painter.save();
    painter.setOpacity(0.5);

    for (int dy = -halfSize; dy <= halfSize; ++dy) {
        for (int dx = -halfSize; dx <= halfSize; ++dx) {
            if (m_brushSize > 1) {
                const double distance = qSqrt(dx * dx + dy * dy);
                if (distance > halfSize + 0.5) continue;
            }

            const int x = gridPos.x() + dx;
            const int y = gridPos.y() + dy;

            if (x >= 0 && x < m_canvasWidth && y >= 0 && y < m_canvasHeight) {
                const QRect pixelRect(x * m_pixelSize, y * m_pixelSize, m_pixelSize, m_pixelSize);
                painter.fillRect(pixelRect, overlayColor);
            }
        }
    }

    painter.restore();

    // Draw brush outline
    painter.save();
    painter.setPen(QPen(Qt::white, 2.0 / m_zoomFactor));

    if (m_brushSize == 1) {
        const QRect pixelRect(gridPos.x() * m_pixelSize, gridPos.y() * m_pixelSize, m_pixelSize, m_pixelSize);
        painter.drawRect(pixelRect);
    } else {
        const QPointF center((gridPos.x() + 0.5) * m_pixelSize, (gridPos.y() + 0.5) * m_pixelSize);
        const double radius = (halfSize + 0.5) * m_pixelSize;
        painter.drawEllipse(center, radius, radius);
    }

    painter.restore();
}

void PixelCanvas::drawEraserOverlay(QPainter &painter, const QPoint &gridPos)
{
    const int halfSize = m_brushSize / 2;

    painter.save();
    painter.setOpacity(0.7);

    for (int dy = -halfSize; dy <= halfSize; ++dy) {
        for (int dx = -halfSize; dx <= halfSize; ++dx) {
            if (m_brushSize > 1) {
                const double distance = qSqrt(dx * dx + dy * dy);
                if (distance > halfSize + 0.5) continue;
            }

            const int x = gridPos.x() + dx;
            const int y = gridPos.y() + dy;

            if (x >= 0 && x < m_canvasWidth && y >= 0 && y < m_canvasHeight) {
                const QRect pixelRect(x * m_pixelSize, y * m_pixelSize, m_pixelSize, m_pixelSize);
                const bool evenCheck = ((x / 4) + (y / 4)) % 2 == 0;
                painter.fillRect(pixelRect, evenCheck ? CHECKERBOARD_LIGHT : CHECKERBOARD_DARK);
            }
        }
    }

    painter.restore();

    // Draw eraser outline
    painter.save();
    painter.setPen(QPen(Qt::red, 2.0 / m_zoomFactor));

    if (m_brushSize == 1) {
        const QRect pixelRect(gridPos.x() * m_pixelSize, gridPos.y() * m_pixelSize, m_pixelSize, m_pixelSize);
        painter.drawRect(pixelRect);
    } else {
        const QPointF center((gridPos.x() + 0.5) * m_pixelSize, (gridPos.y() + 0.5) * m_pixelSize);
        const double radius = (halfSize + 0.5) * m_pixelSize;
        painter.drawEllipse(center, radius, radius);
    }

    painter.restore();
}

void PixelCanvas::drawEyedropperOverlay(QPainter &painter, const QPoint &gridPos)
{
    painter.save();

    const QRect pixelRect(gridPos.x() * m_pixelSize, gridPos.y() * m_pixelSize, m_pixelSize, m_pixelSize);
    painter.setPen(QPen(Qt::white, 3.0 / m_zoomFactor));
    painter.drawRect(pixelRect);
    painter.setPen(QPen(Qt::black, 1.0 / m_zoomFactor));
    painter.drawRect(pixelRect);

    // Draw color preview circle
    const QColor pixelColor = m_canvas.pixelColor(gridPos.x(), gridPos.y());
    const QPointF center((gridPos.x() + 0.5) * m_pixelSize, (gridPos.y() + 0.5) * m_pixelSize);
    const double radius = m_pixelSize * 0.8;

    painter.setBrush(pixelColor);
    painter.setPen(QPen(Qt::white, 2.0 / m_zoomFactor));
    painter.drawEllipse(center, radius, radius);

    painter.restore();
}

void PixelCanvas::drawPaintBucketOverlay(QPainter &painter, const QPoint &gridPos)
{
    painter.save();

    const QRect pixelRect(gridPos.x() * m_pixelSize, gridPos.y() * m_pixelSize, m_pixelSize, m_pixelSize);
    painter.setPen(QPen(m_currentColor, 3.0 / m_zoomFactor));
    painter.drawRect(pixelRect);

    const QRect outerRect = pixelRect.adjusted(-m_pixelSize/2, -m_pixelSize/2, m_pixelSize/2, m_pixelSize/2);
    painter.setPen(QPen(Qt::white, 2.0 / m_zoomFactor, Qt::DashLine));
    painter.drawRect(outerRect);

    painter.restore();
}

void PixelCanvas::drawLineToolOverlay(QPainter &painter, const QPoint &gridPos)
{
    painter.save();
    painter.setPen(QPen(Qt::white, 1.5 / m_zoomFactor));

    const QPointF center((gridPos.x() + 0.5) * m_pixelSize, (gridPos.y() + 0.5) * m_pixelSize);
    const double size = m_pixelSize * 1.5;

    painter.drawLine(center.x() - size, center.y(), center.x() + size, center.y());
    painter.drawLine(center.x(), center.y() - size, center.x(), center.y() + size);

    if (m_previewStartPoint.x() >= 0 && m_previewStartPoint.y() >= 0) {
        painter.setPen(QPen(m_currentColor, 2.0 / m_zoomFactor, Qt::DashLine));
        const QPointF startCenter((m_previewStartPoint.x() + 0.5) * m_pixelSize, (m_previewStartPoint.y() + 0.5) * m_pixelSize);
        painter.drawLine(startCenter, center);
    }

    painter.restore();
}

// Tool interaction methods
void PixelCanvas::setLinePreviewStart(const QPoint &startPoint)
{
    m_previewStartPoint = startPoint;
    update();
}

void PixelCanvas::clearLinePreviewStart()
{
    m_previewStartPoint = QPoint(-1, -1);
    update();
}

void PixelCanvas::drawSelectOverlay(QPainter &painter)
{
    // Draw lasso preview while drawing
    if (m_toolManager) {
        BaseTool *tool = m_toolManager->getCurrentTool();
        if (tool && tool->getType() == ToolType::Lasso) {
            auto *lasso = static_cast<LassoTool*>(tool);
            if (lasso->isDrawing() && lasso->currentPolygon().size() > 1) {
                painter.save();
                QPen lassPen(ACCENT_COLOR, 2.0 / m_zoomFactor, Qt::DashLine);
                painter.setPen(lassPen);
                painter.setBrush(Qt::NoBrush);

                const QPolygon &poly = lasso->currentPolygon();
                QPolygonF scaled;
                for (const QPoint &pt : poly) {
                    scaled.append(QPointF(pt.x() * m_pixelSize + m_pixelSize / 2.0,
                                          pt.y() * m_pixelSize + m_pixelSize / 2.0));
                }
                painter.drawPolyline(scaled);
                painter.restore();
            }
        }
    }

    // Render marching ants from canvas selection mask
    if (!m_hasSelection || m_selectionMask.isNull()) return;

    painter.save();

    // Build a region from the mask
    QRect bounds = selectionBoundingRect();
    if (bounds.isEmpty()) { painter.restore(); return; }

    // Draw marching ants around the selection boundary
    // Find border pixels: selected pixels that have at least one unselected neighbor
    QPainterPath borderPath;
    for (int y = bounds.top(); y <= bounds.bottom(); ++y) {
        for (int x = bounds.left(); x <= bounds.right(); ++x) {
            if (!isPixelSelected(x, y)) continue;

            // Check 4 neighbors — if any is outside or unselected, this pixel has a border edge
            bool top    = (y == 0 || !isPixelSelected(x, y - 1));
            bool bottom = (y >= m_canvasHeight - 1 || !isPixelSelected(x, y + 1));
            bool left   = (x == 0 || !isPixelSelected(x - 1, y));
            bool right  = (x >= m_canvasWidth - 1 || !isPixelSelected(x + 1, y));

            double px = x * m_pixelSize;
            double py = y * m_pixelSize;
            double ps = m_pixelSize;

            if (top)    { borderPath.moveTo(px, py); borderPath.lineTo(px + ps, py); }
            if (bottom) { borderPath.moveTo(px, py + ps); borderPath.lineTo(px + ps, py + ps); }
            if (left)   { borderPath.moveTo(px, py); borderPath.lineTo(px, py + ps); }
            if (right)  { borderPath.moveTo(px + ps, py); borderPath.lineTo(px + ps, py + ps); }
        }
    }

    // White dashed line
    QPen dashPen(Qt::white, 2.0 / m_zoomFactor, Qt::DashLine);
    painter.setPen(dashPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(borderPath);

    // Inner black dashes for contrast
    QPen innerPen(Qt::black, 1.0 / m_zoomFactor, Qt::DashLine);
    innerPen.setDashOffset(4);
    painter.setPen(innerPen);
    painter.drawPath(borderPath);

    painter.restore();
}

const QImage& PixelCanvas::canvasImage() const
{
    if (m_layerManager) return m_layerManager->activeLayer().image;
    return m_canvas;
}

QImage& PixelCanvas::canvasImageRef()
{
    if (m_layerManager) return m_layerManager->activeLayer().image;
    return m_canvas;
}

QImage PixelCanvas::getCanvasState() const
{
    if (m_layerManager) return m_layerManager->activeLayer().image;
    return m_canvas;
}
