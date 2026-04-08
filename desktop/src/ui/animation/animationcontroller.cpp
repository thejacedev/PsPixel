#include "animationcontroller.h"
#include "ui/layers/layermanager.h"

using namespace PixelPaint;

AnimationController::AnimationController(LayerManager *layerManager, QObject *parent)
    : QObject(parent)
    , m_layerManager(layerManager)
    , m_playbackTimer(new QTimer(this))
    , m_fps(ANIM_DEFAULT_FPS)
    , m_currentFrame(0)
    , m_playing(false)
    , m_looping(true)
    , m_savedActiveIndex(0)
{
    connect(m_playbackTimer, &QTimer::timeout, this, &AnimationController::onTimerTick);
}

int AnimationController::frameCount() const
{
    return m_layerManager ? m_layerManager->layerCount() : 0;
}

void AnimationController::play()
{
    if (!m_layerManager || frameCount() < 2) return;

    if (!m_playing) {
        saveVisibilityState();
    }

    m_playing = true;
    showFrame(m_currentFrame);
    m_playbackTimer->start(1000 / m_fps);
    emit playbackStateChanged(true);
}

void AnimationController::pause()
{
    m_playbackTimer->stop();
    m_playing = false;
    emit playbackStateChanged(false);
}

void AnimationController::stop()
{
    m_playbackTimer->stop();

    if (m_playing || !m_savedVisibility.isEmpty()) {
        restoreVisibilityState();
    }

    m_playing = false;
    m_currentFrame = 0;
    emit playbackStateChanged(false);
    emit frameChanged(m_currentFrame);
}

void AnimationController::togglePlayPause()
{
    if (m_playing) {
        pause();
    } else {
        play();
    }
}

void AnimationController::goToFrame(int frameIndex)
{
    if (frameIndex < 0 || frameIndex >= frameCount()) return;

    m_currentFrame = frameIndex;

    if (m_playing || !m_savedVisibility.isEmpty()) {
        showFrame(m_currentFrame);
    }

    emit frameChanged(m_currentFrame);
}

void AnimationController::nextFrame()
{
    int next = m_currentFrame + 1;
    if (next >= frameCount()) {
        next = m_looping ? 0 : frameCount() - 1;
    }
    goToFrame(next);
}

void AnimationController::prevFrame()
{
    int prev = m_currentFrame - 1;
    if (prev < 0) {
        prev = m_looping ? frameCount() - 1 : 0;
    }
    goToFrame(prev);
}

void AnimationController::setFps(int fps)
{
    m_fps = qBound(ANIM_MIN_FPS, fps, ANIM_MAX_FPS);

    if (m_playing) {
        m_playbackTimer->setInterval(1000 / m_fps);
    }

    emit fpsChanged(m_fps);
}

void AnimationController::setLooping(bool loop)
{
    m_looping = loop;
}

void AnimationController::onTimerTick()
{
    int next = m_currentFrame + 1;

    if (next >= frameCount()) {
        if (m_looping) {
            next = 0;
        } else {
            stop();
            return;
        }
    }

    m_currentFrame = next;
    showFrame(m_currentFrame);
    emit frameChanged(m_currentFrame);
}

void AnimationController::showFrame(int frameIndex)
{
    if (!m_layerManager) return;

    // Batch visibility updates — block signals to avoid N repaints
    m_layerManager->blockSignals(true);
    for (int i = 0; i < m_layerManager->layerCount(); ++i) {
        m_layerManager->setLayerVisible(i, i == frameIndex);
    }
    m_layerManager->blockSignals(false);

    // Single repaint
    emit m_layerManager->layersChanged();
}

void AnimationController::saveVisibilityState()
{
    if (!m_layerManager) return;

    m_savedVisibility.clear();
    for (int i = 0; i < m_layerManager->layerCount(); ++i) {
        m_savedVisibility.append(m_layerManager->layerAt(i).visible);
    }
    m_savedActiveIndex = m_layerManager->activeLayerIndex();
}

void AnimationController::restoreVisibilityState()
{
    if (!m_layerManager || m_savedVisibility.isEmpty()) return;

    m_layerManager->blockSignals(true);
    int count = qMin(m_savedVisibility.size(), m_layerManager->layerCount());
    for (int i = 0; i < count; ++i) {
        m_layerManager->setLayerVisible(i, m_savedVisibility[i]);
    }
    if (m_savedActiveIndex < m_layerManager->layerCount()) {
        m_layerManager->setActiveLayer(m_savedActiveIndex);
    }
    m_layerManager->blockSignals(false);
    emit m_layerManager->layersChanged();

    m_savedVisibility.clear();
}
