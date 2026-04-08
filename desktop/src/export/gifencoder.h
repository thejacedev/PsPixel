#ifndef GIFENCODER_H
#define GIFENCODER_H

#include <QImage>
#include <QFile>
#include <QVector>
#include <QString>

namespace PixelPaint {

class GifEncoder
{
public:
    GifEncoder();
    ~GifEncoder();

    bool begin(const QString &filePath, int width, int height, int loopCount = 0);
    bool addFrame(const QImage &frame, int delayCentiseconds);
    bool end();

private:
    struct Palette {
        QVector<QRgb> colors;
        QVector<uint8_t> indexed;
        int transparentIndex;
        int bits; // palette bit depth (1-8)
    };

    Palette quantize(const QImage &image) const;
    QByteArray lzwCompress(const QVector<uint8_t> &data, int minCodeSize) const;

    void writeByte(uint8_t b);
    void writeWord(uint16_t w);
    void writeBytes(const char *data, int len);

    QFile m_file;
    int m_width;
    int m_height;
    bool m_started;
};

} // namespace PixelPaint

#endif // GIFENCODER_H
