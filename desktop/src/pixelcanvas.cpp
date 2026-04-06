#include "pixelcanvas.h"
#include "ui/tools/toolmanager.h"
#include "ui/tools/basetool.h"
#include "ui/tools/selecttool.h"
#include "ui/layers/layermanager.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QEnterEvent>
#include <QPixmap>
#include <QImage>
#include <QtMath>
#include <QCursor>

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

    // Draw tool overlay on top of everything
    drawToolOverlay(painter);
}

void PixelCanvas::mousePressEvent(QMouseEvent *event)
{
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
            drawSelectOverlay(painter);
            break;
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
    // Get the select tool to check for active selection
    if (!m_toolManager) return;

    BaseTool *tool = m_toolManager->getCurrentTool();
    if (!tool) return;

    // Dynamic cast to check if it's a SelectTool with a selection
    // We use the tool's type to avoid including the header
    if (tool->getType() != ToolType::Select) return;

    // Access the selection rect via a property-like approach
    // The select tool stores its rect publicly
    auto *selectTool = static_cast<SelectTool*>(tool);
    if (!selectTool->hasSelection()) return;

    QRect sel = selectTool->selectionRect();

    painter.save();
    QPen dashPen(Qt::white, 2.0 / m_zoomFactor, Qt::DashLine);
    painter.setPen(dashPen);
    painter.setBrush(Qt::NoBrush);
    QRect drawRect(sel.left() * m_pixelSize, sel.top() * m_pixelSize,
                   sel.width() * m_pixelSize, sel.height() * m_pixelSize);
    painter.drawRect(drawRect);

    // Draw inner black dashes for contrast
    QPen innerPen(Qt::black, 1.0 / m_zoomFactor, Qt::DashLine);
    innerPen.setDashOffset(4);
    painter.setPen(innerPen);
    painter.drawRect(drawRect);

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
