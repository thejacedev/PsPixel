#ifndef LAYERMANAGER_H
#define LAYERMANAGER_H

#include <QObject>
#include <QVector>
#include "layer.h"

namespace PixelPaint {

class LayerManager : public QObject
{
    Q_OBJECT

public:
    explicit LayerManager(QObject *parent = nullptr);

    // Layer operations
    int addLayer(const QString &name, int width, int height);
    void removeLayer(int index);
    void moveLayer(int fromIndex, int toIndex);
    void duplicateLayer(int index);

    // Active layer
    int activeLayerIndex() const { return m_activeIndex; }
    void setActiveLayer(int index);
    Layer& activeLayer();
    const Layer& activeLayer() const;

    // Access
    int layerCount() const { return m_layers.size(); }
    Layer& layerAt(int index) { return m_layers[index]; }
    const Layer& layerAt(int index) const { return m_layers[index]; }

    // Layer properties
    void setLayerVisible(int index, bool visible);
    void setLayerOpacity(int index, double opacity);
    void setLayerName(int index, const QString &name);

    // Compositing
    QImage compositeAll(int width, int height) const;

    // Resize all layers
    void resizeAll(int width, int height);

    // Clear all layers and start fresh
    void reset(int width, int height);

signals:
    void layersChanged();
    void activeLayerChanged(int index);

private:
    QVector<Layer> m_layers;
    int m_activeIndex;
};

} // namespace PixelPaint

#endif // LAYERMANAGER_H
