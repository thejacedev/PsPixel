#include "animationpalette.h"
#include "animationcontroller.h"
#include "ui/layers/layermanager.h"
#include "export/spritesheetexporter.h"
#include "export/gifencoder.h"
#include <QPainter>
#include <QFileDialog>
#include <QStandardPaths>
#include <QMessageBox>

using namespace PixelPaint;

AnimationPalette::AnimationPalette(AnimationController *controller,
                                   LayerManager *layerManager,
                                   QWidget *parent)
    : QDockWidget("Animation", parent)
    , m_controller(controller)
    , m_layerManager(layerManager)
    , m_highlightedFrame(-1)
{
    setupUI();

    connect(m_controller, &AnimationController::frameChanged,
            this, &AnimationPalette::onFrameChanged);
    connect(m_controller, &AnimationController::playbackStateChanged,
            this, &AnimationPalette::onPlaybackStateChanged);

    if (m_layerManager) {
        connect(m_layerManager, &LayerManager::layersChanged,
                this, &AnimationPalette::refreshFrames);
    }

    refreshFrames();
}

void AnimationPalette::setupUI()
{
    setObjectName("AnimationPalette");
    setAllowedAreas(Qt::AllDockWidgetAreas);
    setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);

    QWidget *content = new QWidget();
    setWidget(content);

    QVBoxLayout *mainLayout = new QVBoxLayout(content);
    mainLayout->setContentsMargins(SPACING_SM, SPACING_XS, SPACING_SM, SPACING_XS);
    mainLayout->setSpacing(SPACING_XS);

    // Top row: controls
    QHBoxLayout *controlsLayout = new QHBoxLayout();
    controlsLayout->setSpacing(SPACING_XS);

    QString btnStyle = QString(
        "QPushButton {"
        "    background-color: palette(button);"
        "    border: 1px solid palette(mid);"
        "    border-radius: %1px;"
        "    padding: 4px %2px;"
        "    font-size: %3px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: palette(light);"
        "    border: 1px solid palette(highlight);"
        "}"
        "QPushButton:pressed {"
        "    background-color: palette(dark);"
        "}"
        "QPushButton:checked {"
        "    background-color: palette(highlight);"
        "    color: palette(highlighted-text);"
        "}").arg(RADIUS_CONTROL).arg(SPACING_SM).arg(FONT_SIZE_BODY);

    // Play/Pause
    m_playPauseButton = new QPushButton("Play");
    m_playPauseButton->setToolTip("Play/Pause animation");
    m_playPauseButton->setStyleSheet(btnStyle);
    connect(m_playPauseButton, &QPushButton::clicked, this, &AnimationPalette::onPlayPauseClicked);
    controlsLayout->addWidget(m_playPauseButton);

    // Stop
    m_stopButton = new QPushButton("Stop");
    m_stopButton->setToolTip("Stop and reset to frame 1");
    m_stopButton->setStyleSheet(btnStyle);
    connect(m_stopButton, &QPushButton::clicked, this, &AnimationPalette::onStopClicked);
    controlsLayout->addWidget(m_stopButton);

    // Prev/Next
    m_prevButton = new QPushButton("<");
    m_prevButton->setToolTip("Previous frame");
    m_prevButton->setMaximumWidth(28);
    m_prevButton->setStyleSheet(btnStyle);
    connect(m_prevButton, &QPushButton::clicked, m_controller, &AnimationController::prevFrame);
    controlsLayout->addWidget(m_prevButton);

    m_nextButton = new QPushButton(">");
    m_nextButton->setToolTip("Next frame");
    m_nextButton->setMaximumWidth(28);
    m_nextButton->setStyleSheet(btnStyle);
    connect(m_nextButton, &QPushButton::clicked, m_controller, &AnimationController::nextFrame);
    controlsLayout->addWidget(m_nextButton);

    // Separator
    QFrame *sep1 = new QFrame();
    sep1->setFrameShape(QFrame::VLine);
    sep1->setFrameShadow(QFrame::Sunken);
    controlsLayout->addWidget(sep1);

    // Loop
    m_loopButton = new QPushButton("Loop");
    m_loopButton->setCheckable(true);
    m_loopButton->setChecked(true);
    m_loopButton->setToolTip("Toggle looping");
    m_loopButton->setStyleSheet(btnStyle);
    connect(m_loopButton, &QPushButton::toggled, m_controller, &AnimationController::setLooping);
    controlsLayout->addWidget(m_loopButton);

    // FPS
    QLabel *fpsLabel = new QLabel("FPS:");
    fpsLabel->setStyleSheet(QString("font-size: %1px;").arg(FONT_SIZE_BODY));
    controlsLayout->addWidget(fpsLabel);

    m_fpsSpinBox = new QSpinBox();
    m_fpsSpinBox->setRange(ANIM_MIN_FPS, ANIM_MAX_FPS);
    m_fpsSpinBox->setValue(ANIM_DEFAULT_FPS);
    m_fpsSpinBox->setSuffix("");
    m_fpsSpinBox->setMaximumWidth(60);
    connect(m_fpsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            m_controller, &AnimationController::setFps);
    controlsLayout->addWidget(m_fpsSpinBox);

    // Separator
    QFrame *sep2 = new QFrame();
    sep2->setFrameShape(QFrame::VLine);
    sep2->setFrameShadow(QFrame::Sunken);
    controlsLayout->addWidget(sep2);

    // Frame info
    m_frameInfoLabel = new QLabel("Frame 1/1");
    m_frameInfoLabel->setStyleSheet(QString("font-size: %1px; color: palette(text);").arg(FONT_SIZE_CAPTION));
    controlsLayout->addWidget(m_frameInfoLabel);

    controlsLayout->addStretch();

    // Export buttons
    m_exportSheetButton = new QPushButton("Spritesheet");
    m_exportSheetButton->setToolTip("Export all frames as a spritesheet PNG");
    m_exportSheetButton->setStyleSheet(btnStyle);
    connect(m_exportSheetButton, &QPushButton::clicked, this, &AnimationPalette::onExportSpritesheet);
    controlsLayout->addWidget(m_exportSheetButton);

    m_exportGifButton = new QPushButton("GIF");
    m_exportGifButton->setToolTip("Export as animated GIF");
    m_exportGifButton->setStyleSheet(btnStyle);
    connect(m_exportGifButton, &QPushButton::clicked, this, &AnimationPalette::onExportGif);
    controlsLayout->addWidget(m_exportGifButton);

    mainLayout->addLayout(controlsLayout);

    // Frame strip (scrollable horizontal thumbnails)
    m_frameScrollArea = new QScrollArea();
    m_frameScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_frameScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_frameScrollArea->setWidgetResizable(true);
    m_frameScrollArea->setFixedHeight(ANIM_THUMBNAIL_SIZE + SPACING_SM * 2 + 4);
    m_frameScrollArea->setStyleSheet(
        QString("QScrollArea { border: 1px solid palette(mid); border-radius: %1px; background-color: palette(base); }")
        .arg(RADIUS_CONTROL));

    m_frameStrip = new QWidget();
    m_frameStripLayout = new QHBoxLayout(m_frameStrip);
    m_frameStripLayout->setContentsMargins(SPACING_XS, SPACING_XS, SPACING_XS, SPACING_XS);
    m_frameStripLayout->setSpacing(SPACING_XS);
    m_frameStripLayout->addStretch();

    m_frameScrollArea->setWidget(m_frameStrip);
    mainLayout->addWidget(m_frameScrollArea);
}

void AnimationPalette::refreshFrames()
{
    // Clear existing thumbnails
    qDeleteAll(m_frameThumbnails);
    m_frameThumbnails.clear();

    // Remove all items from layout except the trailing stretch
    QLayoutItem *item;
    while ((item = m_frameStripLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    if (!m_layerManager) return;

    // Create a thumbnail for each layer (frame)
    for (int i = 0; i < m_layerManager->layerCount(); ++i) {
        const Layer &layer = m_layerManager->layerAt(i);

        QLabel *thumb = new QLabel();
        thumb->setFixedSize(ANIM_THUMBNAIL_SIZE + 4, ANIM_THUMBNAIL_SIZE + 4);
        thumb->setAlignment(Qt::AlignCenter);
        thumb->setPixmap(generateThumbnail(layer.image));
        thumb->setToolTip(QString("Frame %1: %2").arg(i + 1).arg(layer.name));
        thumb->setStyleSheet(
            QString("QLabel { border: 2px solid palette(mid); border-radius: %1px; padding: 0px; }")
            .arg(RADIUS_CONTROL));
        thumb->setCursor(Qt::PointingHandCursor);

        // Click to go to frame
        thumb->setProperty("frameIndex", i);
        thumb->installEventFilter(this);

        m_frameStripLayout->addWidget(thumb);
        m_frameThumbnails.append(thumb);
    }

    m_frameStripLayout->addStretch();

    // Update highlight and info
    updateFrameHighlight(m_controller->currentFrame());
    m_frameInfoLabel->setText(QString("Frame %1/%2")
        .arg(m_controller->currentFrame() + 1)
        .arg(m_controller->frameCount()));
}

QPixmap AnimationPalette::generateThumbnail(const QImage &image) const
{
    int size = ANIM_THUMBNAIL_SIZE;

    // Draw checkerboard background + image
    QImage thumb(size, size, QImage::Format_ARGB32);

    // Checkerboard
    int checkSize = qMax(1, size / 8);
    for (int y = 0; y < size; ++y) {
        QRgb *line = reinterpret_cast<QRgb*>(thumb.scanLine(y));
        for (int x = 0; x < size; ++x) {
            bool even = ((x / checkSize) + (y / checkSize)) % 2 == 0;
            line[x] = even ? CHECKERBOARD_LIGHT.rgba() : CHECKERBOARD_DARK.rgba();
        }
    }

    // Scale and center the frame image
    QImage scaled = image.scaled(size, size, Qt::KeepAspectRatio, Qt::FastTransformation);
    QPainter p(&thumb);
    int ox = (size - scaled.width()) / 2;
    int oy = (size - scaled.height()) / 2;
    p.drawImage(ox, oy, scaled);
    p.end();

    return QPixmap::fromImage(thumb);
}

void AnimationPalette::updateFrameHighlight(int frameIndex)
{
    for (int i = 0; i < m_frameThumbnails.size(); ++i) {
        if (i == frameIndex) {
            m_frameThumbnails[i]->setStyleSheet(
                QString("QLabel { border: 2px solid %1; border-radius: %2px; padding: 0px; }")
                .arg(ACCENT_HEX).arg(RADIUS_CONTROL));
        } else {
            m_frameThumbnails[i]->setStyleSheet(
                QString("QLabel { border: 2px solid palette(mid); border-radius: %1px; padding: 0px; }")
                .arg(RADIUS_CONTROL));
        }
    }
}

void AnimationPalette::onPlayPauseClicked()
{
    m_controller->togglePlayPause();
}

void AnimationPalette::onStopClicked()
{
    m_controller->stop();
}

void AnimationPalette::onFrameChanged(int frameIndex)
{
    updateFrameHighlight(frameIndex);
    m_frameInfoLabel->setText(QString("Frame %1/%2")
        .arg(frameIndex + 1)
        .arg(m_controller->frameCount()));

    // Scroll to current frame
    if (frameIndex >= 0 && frameIndex < m_frameThumbnails.size()) {
        m_frameScrollArea->ensureWidgetVisible(m_frameThumbnails[frameIndex]);
    }
}

void AnimationPalette::onPlaybackStateChanged(bool playing)
{
    m_playPauseButton->setText(playing ? "Pause" : "Play");
}

void AnimationPalette::onExportSpritesheet()
{
    if (!m_layerManager || m_layerManager->layerCount() == 0) return;

    SpritesheetExporter::exportSpritesheet(m_layerManager, this);
}

void AnimationPalette::onExportGif()
{
    if (!m_layerManager || m_layerManager->layerCount() == 0) return;

    QString filePath = QFileDialog::getSaveFileName(this, "Export Animated GIF",
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/animation.gif",
        "GIF Files (*.gif)");

    if (filePath.isEmpty()) return;

    int fps = m_fpsSpinBox->value();
    int delayCentiseconds = qMax(1, 100 / fps);

    const Layer &first = m_layerManager->layerAt(0);
    int w = first.image.width();
    int h = first.image.height();

    GifEncoder encoder;
    if (!encoder.begin(filePath, w, h, 0)) {
        QMessageBox::warning(this, "Error", "Failed to create GIF file.");
        return;
    }

    for (int i = 0; i < m_layerManager->layerCount(); ++i) {
        encoder.addFrame(m_layerManager->layerAt(i).image, delayCentiseconds);
    }

    encoder.end();
}

// Event filter for frame thumbnail clicks
bool AnimationPalette::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QLabel *label = qobject_cast<QLabel*>(obj);
        if (label) {
            bool ok;
            int idx = label->property("frameIndex").toInt(&ok);
            if (ok) {
                m_controller->goToFrame(idx);
                return true;
            }
        }
    }
    return QDockWidget::eventFilter(obj, event);
}
