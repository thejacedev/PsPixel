#ifndef ANIMATIONCONTROLLER_H
#define ANIMATIONCONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QVector>
#include "constants.h"

namespace PixelPaint {

class LayerManager;

class AnimationController : public QObject
{
    Q_OBJECT

public:
    explicit AnimationController(LayerManager *layerManager, QObject *parent = nullptr);

    void play();
    void pause();
    void stop();
    void togglePlayPause();

    void goToFrame(int frameIndex);
    void nextFrame();
    void prevFrame();

    void setFps(int fps);
    int fps() const { return m_fps; }
    void setLooping(bool loop);
    bool isLooping() const { return m_looping; }
    bool isPlaying() const { return m_playing; }
    int currentFrame() const { return m_currentFrame; }
    int frameCount() const;

signals:
    void frameChanged(int frameIndex);
    void playbackStateChanged(bool playing);
    void fpsChanged(int fps);

private slots:
    void onTimerTick();

private:
    void showFrame(int frameIndex);
    void saveVisibilityState();
    void restoreVisibilityState();

    LayerManager *m_layerManager;
    QTimer *m_playbackTimer;
    int m_fps;
    int m_currentFrame;
    bool m_playing;
    bool m_looping;
    QVector<bool> m_savedVisibility;
    int m_savedActiveIndex;
};

} // namespace PixelPaint

#endif // ANIMATIONCONTROLLER_H
