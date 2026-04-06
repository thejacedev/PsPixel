#ifndef BASETOOL_H
#define BASETOOL_H

#include <QObject>
#include <QMouseEvent>
#include <QColor>
#include <QPoint>
#include <QVector>
#include "constants.h"

namespace PixelPaint {

class PixelCanvas;
class HistoryManager;

class BaseTool : public QObject
{
    Q_OBJECT

public:
    explicit BaseTool(QObject *parent = nullptr);
    virtual ~BaseTool() = default;

    // Tool information
    virtual ToolType getType() const = 0;
    virtual QString getName() const = 0;
    virtual QString getIconPath() const = 0;
    virtual QString getTooltip() const = 0;

    // Tool state
    virtual void setColor(const QColor &color);
    virtual void setBrushSize(int size);
    virtual void setCanvas(PixelCanvas *canvas);

    // Mouse event handling
    virtual void onMousePress(const QPoint &canvasPos, Qt::MouseButton button) = 0;
    virtual void onMouseMove(const QPoint &canvasPos, Qt::MouseButtons buttons) = 0;
    virtual void onMouseRelease(const QPoint &canvasPos, Qt::MouseButton button) = 0;

    // Tool activation/deactivation
    virtual void onActivate() {}
    virtual void onDeactivate() {}

    // New operation-based history support (Photoshop-style)
    void startHistoryOperation();
    void commitHistoryOperation(const QString &actionName);
    void cancelHistoryOperation();
    bool isHistoryOperationInProgress() const { return m_operationInProgress; }
    
    // Legacy method - deprecated, use operation-based methods instead
    void saveCanvasState(const QString &actionName);

signals:
    void canvasModified();

protected:
    // Helper methods for tools
    void drawPixel(int x, int y, const QColor &color);
    bool isValidPosition(int x, int y) const;
    QVector<QPoint> getLinePoints(const QPoint &start, const QPoint &end);
    QVector<QPoint> getCirclePoints(const QPoint &center, int radius);
    QVector<QPoint> getRectanglePoints(const QPoint &topLeft, const QPoint &bottomRight);

    QColor m_color;
    int m_brushSize;
    PixelCanvas *m_canvas;
    bool m_isDrawing;
    QPoint m_lastPoint;
    bool m_operationInProgress; // Track if a history operation is active
};

} // namespace PixelPaint

#endif // BASETOOL_H 