#include "layermanager.h"
#include <QPainter>

using namespace PixelPaint;

LayerManager::LayerManager(QObject *parent)
    : QObject(parent)
    , m_activeIndex(0)
{
}

int LayerManager::addLayer(const QString &name, int width, int height)
{
    Layer layer(name, width, height);
    m_layers.append(layer);
    int index = m_layers.size() - 1;
    emit layersChanged();
    return index;
}

void LayerManager::removeLayer(int index)
{
    if (m_layers.size() <= 1) return; // Keep at least one layer
    if (index < 0 || index >= m_layers.size()) return;

    m_layers.removeAt(index);

    if (m_activeIndex >= m_layers.size()) {
        m_activeIndex = m_layers.size() - 1;
    }

    emit layersChanged();
    emit activeLayerChanged(m_activeIndex);
}

void LayerManager::moveLayer(int fromIndex, int toIndex)
{
    if (fromIndex < 0 || fromIndex >= m_layers.size()) return;
    if (toIndex < 0 || toIndex >= m_layers.size()) return;
    if (fromIndex == toIndex) return;

    Layer layer = m_layers.takeAt(fromIndex);
    m_layers.insert(toIndex, layer);

    // Update active index
    if (m_activeIndex == fromIndex) {
        m_activeIndex = toIndex;
    } else if (fromIndex < m_activeIndex && toIndex >= m_activeIndex) {
        m_activeIndex--;
    } else if (fromIndex > m_activeIndex && toIndex <= m_activeIndex) {
        m_activeIndex++;
    }

    emit layersChanged();
}

void LayerManager::duplicateLayer(int index)
{
    if (index < 0 || index >= m_layers.size()) return;

    Layer copy = m_layers[index];
    copy.name = copy.name + " Copy";
    m_layers.insert(index + 1, copy);

    emit layersChanged();
}

void LayerManager::setActiveLayer(int index)
{
    if (index < 0 || index >= m_layers.size()) return;
    if (m_activeIndex == index) return;

    m_activeIndex = index;
    emit activeLayerChanged(index);
    emit layersChanged();
}

Layer& LayerManager::activeLayer()
{
    return m_layers[m_activeIndex];
}

const Layer& LayerManager::activeLayer() const
{
    return m_layers[m_activeIndex];
}

void LayerManager::setLayerVisible(int index, bool visible)
{
    if (index < 0 || index >= m_layers.size()) return;
    m_layers[index].visible = visible;
    emit layersChanged();
}

void LayerManager::setLayerOpacity(int index, double opacity)
{
    if (index < 0 || index >= m_layers.size()) return;
    m_layers[index].opacity = qBound(0.0, opacity, 1.0);
    emit layersChanged();
}

void LayerManager::setLayerName(int index, const QString &name)
{
    if (index < 0 || index >= m_layers.size()) return;
    m_layers[index].name = name;
    emit layersChanged();
}

QImage LayerManager::compositeAll(int width, int height) const
{
    QImage result(width, height, QImage::Format_ARGB32);
    result.fill(Qt::transparent);

    QPainter p(&result);
    // Draw from bottom (index 0) to top (last index)
    for (int i = 0; i < m_layers.size(); ++i) {
        const Layer &layer = m_layers[i];
        if (!layer.visible) continue;
        p.setOpacity(layer.opacity);
        p.drawImage(0, 0, layer.image);
    }
    p.end();

    return result;
}

void LayerManager::resizeAll(int width, int height)
{
    for (Layer &layer : m_layers) {
        QImage oldImage = layer.image;
        layer.image = QImage(width, height, QImage::Format_ARGB32);
        layer.image.fill(Qt::transparent);
        QPainter p(&layer.image);
        p.setCompositionMode(QPainter::CompositionMode_Source);
        p.drawImage(0, 0, oldImage);
        p.end();
    }
    emit layersChanged();
}

void LayerManager::reset(int width, int height)
{
    m_layers.clear();
    m_activeIndex = 0;
    addLayer("Background", width, height);
}
