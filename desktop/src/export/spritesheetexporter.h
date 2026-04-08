#ifndef SPRITESHEETEXPORTER_H
#define SPRITESHEETEXPORTER_H

#include <QImage>
#include <QWidget>

namespace PixelPaint {

class LayerManager;

class SpritesheetExporter
{
public:
    enum Layout { HorizontalStrip, VerticalStrip, Grid };

    struct Options {
        Layout layout = HorizontalStrip;
        int gridColumns = 4;
        int padding = 0;
    };

    static bool exportSpritesheet(LayerManager *layerManager, QWidget *parent);
    static QImage assembleSpritesheet(LayerManager *layerManager, const Options &options);
};

} // namespace PixelPaint

#endif // SPRITESHEETEXPORTER_H
