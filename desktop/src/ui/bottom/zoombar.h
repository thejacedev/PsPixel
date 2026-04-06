#ifndef ZOOMBAR_H
#define ZOOMBAR_H

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOption>
#include <QTimer>

namespace PixelPaint {

class ZoomBar : public QWidget
{
    Q_OBJECT

public:
    explicit ZoomBar(QWidget *parent = nullptr);
    
    void setZoomLevel(int percentage);
    int getZoomLevel() const;

signals:
    void zoomChanged(int percentage);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private slots:
    void onRepeatTimer();
    void onEditingFinished();

private:
    void updateZoomFromPosition(const QPoint &pos);
    void startEditing();
    QRect getLeftArrowRect() const;
    QRect getRightArrowRect() const;
    QRect getPercentageRect() const;
    
    int m_zoomLevel;
    bool m_dragging;
    bool m_leftArrowPressed;
    bool m_rightArrowPressed;
    QPoint m_dragStartPos;
    int m_dragStartZoom;
    QTimer *m_repeatTimer;
    QLineEdit *m_lineEdit;
    bool m_editingMode;
    
    // Constants
    static constexpr int MIN_ZOOM = 10;
    static constexpr int MAX_ZOOM = 1000;
    static constexpr int ARROW_WIDTH = 12;
    static constexpr int HEIGHT = 20;
};

} // namespace PixelPaint

#endif // ZOOMBAR_H 