#ifndef PIXELCANVAS_H
#define PIXELCANVAS_H

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QColor>
#include <QImage>
#include <QVector>
#include <QPointF>
#include "constants.h"

namespace PixelPaint {

class ToolManager;
class LayerManager;

class PixelCanvas : public QWidget
{
    Q_OBJECT

public:
    explicit PixelCanvas(QWidget *parent = nullptr);

    // Tool management
    void setToolManager(ToolManager *toolManager);
    void setLayerManager(LayerManager *layerManager);
    void updateComposite();

    // Color and brush management
    void setCurrentColor(const QColor &color);
    void setBrushSize(int size);

    // Canvas operations
    void clearCanvas();
    void resizeCanvas(int width, int height);

    // File operations
    bool saveImage(const QString &fileName);
    bool loadImage(const QString &fileName);

    // Direct pixel access (fast path for tools)
    void setPixel(int x, int y, const QColor &color);
    QColor pixelAt(int x, int y) const;
    const QImage& canvasImage() const;
    QImage& canvasImageRef();

    // Legacy conversion helpers (used by file I/O)
    QVector<QVector<QColor>> getPixelData() const;
    void setPixelData(const QVector<QVector<QColor>> &data);

    // Getters
    int getCanvasWidth() const { return m_canvasWidth; }
    int getCanvasHeight() const { return m_canvasHeight; }
    int getPixelSize() const { return m_pixelSize; }
    QColor getCurrentColor() const { return m_currentColor; }
    int getBrushSize() const { return m_brushSize; }

    // Zoom and Pan operations
    void zoomIn();
    void zoomOut();
    void resetZoom();
    void resetPan();
    void setZoomFactor(double factor);
    double getZoomFactor() const { return m_zoomFactor; }

    // Mirror/Symmetry
    void setMirrorHorizontal(bool on) { m_mirrorH = on; }
    void setMirrorVertical(bool on) { m_mirrorV = on; }
    bool mirrorHorizontal() const { return m_mirrorH; }
    bool mirrorVertical() const { return m_mirrorV; }

    // Reference image
    void loadReferenceImage(const QString &filePath);
    void clearReferenceImage();
    bool hasReferenceImage() const { return !m_refImage.isNull(); }
    void setReferenceOpacity(double opacity) { m_refOpacity = opacity; update(); }
    double referenceOpacity() const { return m_refOpacity; }
    void setReferenceOffset(const QPointF &offset) { m_refOffset = offset; update(); }
    QPointF referenceOffset() const { return m_refOffset; }
    void setReferenceLocked(bool locked) { m_refLocked = locked; }
    bool referenceLocked() const { return m_refLocked; }
    void setReferenceScale(double scale) { m_refScale = qBound(0.1, scale, 10.0); update(); }
    double referenceScale() const { return m_refScale; }
    void setReferenceActive(bool active) { m_refActive = active; update(); }
    bool referenceActive() const { return m_refActive; }
    QColor referencePixelAt(int x, int y) const;

    // Tool interaction methods
    void setLinePreviewStart(const QPoint &startPoint);
    void clearLinePreviewStart();

    // Selection mask — persists across tool switches, gates pixel writes
    void setSelectionMask(const QImage &mask);
    void clearSelection();
    bool hasSelection() const { return m_hasSelection; }
    bool isPixelSelected(int x, int y) const;
    const QImage& selectionMask() const { return m_selectionMask; }
    QRect selectionBoundingRect() const;

    // History support
    QImage getCanvasState() const;

signals:
    void zoomFactorChanged(double factor);
    void referenceImageChanged();
    void cursorPositionChanged(int x, int y);
    void cursorLeftCanvas();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    // Drawing methods (legacy fallback)
    void drawBrush(int centerX, int centerY);
    QPoint getGridPosition(const QPoint &mousePos);

    // Tool overlay methods
    void drawToolOverlay(QPainter &painter);
    void drawBrushOverlay(QPainter &painter, const QPoint &gridPos);
    void drawEraserOverlay(QPainter &painter, const QPoint &gridPos);
    void drawEyedropperOverlay(QPainter &painter, const QPoint &gridPos);
    void drawLineToolOverlay(QPainter &painter, const QPoint &gridPos);
    void drawPaintBucketOverlay(QPainter &painter, const QPoint &gridPos);
    void drawSelectOverlay(QPainter &painter);

    // Coordinate transformation methods
    QPointF canvasToWidget(const QPointF &canvasPos) const;
    QPointF widgetToCanvas(const QPointF &widgetPos) const;

    // Canvas initialization
    void initializeCanvas();
    void updateCanvasSize();
    void regenerateCheckerboard();

    // Canvas properties
    int m_canvasWidth;
    int m_canvasHeight;
    int m_pixelSize;
    int m_brushSize;

    // Zoom and Pan properties
    double m_zoomFactor;
    QPointF m_panOffset;
    bool m_panning;
    QPoint m_lastPanPoint;

    // Drawing state
    QColor m_currentColor;
    QImage m_canvas;
    QImage m_checkerboard;
    bool m_painting;
    QPoint m_lastPoint;

    // Tool system
    ToolManager *m_toolManager;
    LayerManager *m_layerManager;

    // Tool overlay/cursor
    QPoint m_mousePosition;
    bool m_mouseOnCanvas;
    QPoint m_previewStartPoint; // For line tool preview

    // Mirror/Symmetry state
    bool m_mirrorH;
    bool m_mirrorV;

    // Selection mask (grayscale: 0 = unselected, 255 = selected)
    QImage m_selectionMask;
    bool m_hasSelection;

    // Reference image
    QImage m_refImage;
    QPointF m_refOffset;
    double m_refOpacity;
    double m_refScale;
    bool m_refLocked;
    bool m_refActive;
    bool m_refDragging;
    bool m_refResizing;
    QPoint m_refDragStart;
    double m_refResizeStartScale;
    QPointF m_refResizeStartOffset;
    int m_refResizeCorner; // 0=TL 1=TR 2=BL 3=BR
};

} // namespace PixelPaint

#endif // PIXELCANVAS_H
