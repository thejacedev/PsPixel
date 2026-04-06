#include "zoombar.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTimer>
#include <QApplication>
#include <QStyleOption>
#include <QLineEdit>
#include <cmath>

using namespace PixelPaint;

ZoomBar::ZoomBar(QWidget *parent)
    : QWidget(parent)
    , m_zoomLevel(100)
    , m_dragging(false)
    , m_leftArrowPressed(false)
    , m_rightArrowPressed(false)
    , m_dragStartZoom(100)
    , m_repeatTimer(new QTimer(this))
    , m_lineEdit(new QLineEdit(this))
    , m_editingMode(false)
{
    setFixedHeight(HEIGHT);
    setMinimumWidth(80);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setMouseTracking(true);
    setCursor(Qt::ArrowCursor);
    
    m_repeatTimer->setInterval(100);
    connect(m_repeatTimer, &QTimer::timeout, this, &ZoomBar::onRepeatTimer);
    
    // Setup line edit
    m_lineEdit->hide();
    m_lineEdit->setAlignment(Qt::AlignCenter);
    m_lineEdit->setFrame(false);
    connect(m_lineEdit, &QLineEdit::editingFinished, this, &ZoomBar::onEditingFinished);
}

void ZoomBar::setZoomLevel(int percentage)
{
    int newZoom = qBound(MIN_ZOOM, percentage, MAX_ZOOM);
    if (newZoom != m_zoomLevel) {
        m_zoomLevel = newZoom;
        update();
    }
}

int ZoomBar::getZoomLevel() const
{
    return m_zoomLevel;
}

void ZoomBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Background
    painter.fillRect(rect(), palette().window());
    
    // Get rects
    QRect leftArrow = getLeftArrowRect();
    QRect rightArrow = getRightArrowRect();
    QRect percentRect = getPercentageRect();
    
    // Draw left arrow "<"
    painter.setPen(QPen(palette().text().color(), 1));
    if (m_leftArrowPressed) {
        painter.fillRect(leftArrow, palette().highlight());
    }
    
    QPolygon leftTriangle;
    int arrowCenterY = leftArrow.center().y();
    int arrowSize = 4;
    leftTriangle << QPoint(leftArrow.right() - 3, arrowCenterY - arrowSize)
                << QPoint(leftArrow.left() + 3, arrowCenterY)
                << QPoint(leftArrow.right() - 3, arrowCenterY + arrowSize);
    painter.drawPolygon(leftTriangle);
    
    // Draw right arrow ">"
    if (m_rightArrowPressed) {
        painter.fillRect(rightArrow, palette().highlight());
    }
    
    QPolygon rightTriangle;
    rightTriangle << QPoint(rightArrow.left() + 3, arrowCenterY - arrowSize)
                 << QPoint(rightArrow.right() - 3, arrowCenterY)
                 << QPoint(rightArrow.left() + 3, arrowCenterY + arrowSize);
    painter.drawPolygon(rightTriangle);
    
    // Draw percentage text
    painter.setPen(palette().text().color());
    if (!m_editingMode) {
        painter.drawText(percentRect, Qt::AlignCenter, QString("%1%").arg(m_zoomLevel));
    }
    
    // Draw border around the whole widget
    painter.setPen(QPen(palette().mid().color(), 1));
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
}

void ZoomBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && !m_editingMode) {
        QRect leftArrow = getLeftArrowRect();
        QRect rightArrow = getRightArrowRect();
        QRect percentRect = getPercentageRect();
        
        if (leftArrow.contains(event->pos())) {
            m_leftArrowPressed = true;
            setZoomLevel(m_zoomLevel - 10);
            emit zoomChanged(m_zoomLevel);
            m_repeatTimer->start();
            update();
        }
        else if (rightArrow.contains(event->pos())) {
            m_rightArrowPressed = true;
            setZoomLevel(m_zoomLevel + 10);
            emit zoomChanged(m_zoomLevel);
            m_repeatTimer->start();
            update();
        }
        else if (percentRect.contains(event->pos())) {
            // Check if this is a potential drag or just a click
            m_dragStartPos = event->pos();
            m_dragStartZoom = m_zoomLevel;
            // Don't set dragging immediately - wait for mouse movement
        }
    }
}

void ZoomBar::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_editingMode) {
        QRect percentRect = getPercentageRect();
        
        // Start dragging if mouse moved enough from press position
        if (!m_dragging && (event->buttons() & Qt::LeftButton) && 
            percentRect.contains(m_dragStartPos) &&
            (event->pos() - m_dragStartPos).manhattanLength() > 3) {
            m_dragging = true;
            setCursor(Qt::SizeHorCursor);
        }
        
        if (m_dragging) {
            int deltaX = event->pos().x() - m_dragStartPos.x();
            int zoomChange = deltaX; // 1 pixel = 1% change
            int newZoom = m_dragStartZoom + zoomChange;
            
            setZoomLevel(newZoom);
            emit zoomChanged(m_zoomLevel);
        }
        else {
            // Update cursor based on hover area
            QRect leftArrow = getLeftArrowRect();
            QRect rightArrow = getRightArrowRect();
            
            if (leftArrow.contains(event->pos()) || rightArrow.contains(event->pos())) {
                setCursor(Qt::PointingHandCursor);
            }
            else if (percentRect.contains(event->pos())) {
                setCursor(Qt::IBeamCursor);
            }
            else {
                setCursor(Qt::ArrowCursor);
            }
        }
    }
}

void ZoomBar::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // If we were not dragging and clicked in the percentage area, start editing
        if (!m_dragging && !m_editingMode) {
            QRect percentRect = getPercentageRect();
            if (percentRect.contains(event->pos()) && percentRect.contains(m_dragStartPos)) {
                startEditing();
                return;
            }
        }
        
        m_dragging = false;
        m_leftArrowPressed = false;
        m_rightArrowPressed = false;
        m_repeatTimer->stop();
        setCursor(Qt::ArrowCursor);
        update();
    }
}

void ZoomBar::wheelEvent(QWheelEvent *event)
{
    if (event->angleDelta().y() > 0) {
        setZoomLevel(m_zoomLevel + 10);
    } else {
        setZoomLevel(m_zoomLevel - 10);
    }
    emit zoomChanged(m_zoomLevel);
    event->accept();
}

void ZoomBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QRect percentRect = getPercentageRect();
        if (percentRect.contains(event->pos())) {
            startEditing();
        }
    }
}

void ZoomBar::onRepeatTimer()
{
    if (m_leftArrowPressed) {
        setZoomLevel(m_zoomLevel - 10);
        emit zoomChanged(m_zoomLevel);
    }
    else if (m_rightArrowPressed) {
        setZoomLevel(m_zoomLevel + 10);
        emit zoomChanged(m_zoomLevel);
    }
}

void ZoomBar::startEditing()
{
    m_editingMode = true;
    QRect percentRect = getPercentageRect();
    
    // Position and size the line edit
    m_lineEdit->setGeometry(percentRect);
    m_lineEdit->setText(QString::number(m_zoomLevel));
    m_lineEdit->show();
    m_lineEdit->setFocus();
    m_lineEdit->selectAll();
    
    update();
}

void ZoomBar::onEditingFinished()
{
    if (!m_editingMode) return;
    
    bool ok;
    int newZoom = m_lineEdit->text().toInt(&ok);
    
    if (ok) {
        setZoomLevel(newZoom);
        emit zoomChanged(m_zoomLevel);
    }
    
    m_editingMode = false;
    m_lineEdit->hide();
    clearFocus();
    update();
}

QRect ZoomBar::getLeftArrowRect() const
{
    return QRect(0, 0, ARROW_WIDTH, height());
}

QRect ZoomBar::getRightArrowRect() const
{
    return QRect(width() - ARROW_WIDTH, 0, ARROW_WIDTH, height());
}

QRect ZoomBar::getPercentageRect() const
{
    return QRect(ARROW_WIDTH, 0, width() - 2 * ARROW_WIDTH, height());
} 