#ifndef LAYER_H
#define LAYER_H

#include <QImage>
#include <QString>

namespace PixelPaint {

struct Layer {
    QString name;
    QImage image;
    bool visible;
    double opacity;

    Layer() : visible(true), opacity(1.0) {}
    Layer(const QString &n, int w, int h)
        : name(n), visible(true), opacity(1.0)
    {
        image = QImage(w, h, QImage::Format_ARGB32);
        image.fill(Qt::transparent);
    }
};

} // namespace PixelPaint

#endif // LAYER_H
