#ifndef ANIMATIONPALETTE_H
#define ANIMATIONPALETTE_H

#include <QDockWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QScrollArea>
#include <QFrame>
#include <QEvent>
#include "constants.h"

namespace PixelPaint {

class AnimationController;
class LayerManager;

class AnimationPalette : public QDockWidget
{
    Q_OBJECT

public:
    explicit AnimationPalette(AnimationController *controller,
                              LayerManager *layerManager,
                              QWidget *parent = nullptr);

    void refreshFrames();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onPlayPauseClicked();
    void onStopClicked();
    void onFrameChanged(int frameIndex);
    void onPlaybackStateChanged(bool playing);
    void onExportSpritesheet();
    void onExportGif();

private:
    void setupUI();
    QPixmap generateThumbnail(const QImage &image) const;
    void updateFrameHighlight(int frameIndex);

    AnimationController *m_controller;
    LayerManager *m_layerManager;

    // Frame strip
    QScrollArea *m_frameScrollArea;
    QWidget *m_frameStrip;
    QHBoxLayout *m_frameStripLayout;
    QVector<QLabel*> m_frameThumbnails;

    // Controls
    QPushButton *m_playPauseButton;
    QPushButton *m_stopButton;
    QPushButton *m_prevButton;
    QPushButton *m_nextButton;
    QPushButton *m_loopButton;
    QSpinBox *m_fpsSpinBox;
    QLabel *m_frameInfoLabel;
    QPushButton *m_exportSheetButton;
    QPushButton *m_exportGifButton;

    int m_highlightedFrame;
};

} // namespace PixelPaint

#endif // ANIMATIONPALETTE_H
