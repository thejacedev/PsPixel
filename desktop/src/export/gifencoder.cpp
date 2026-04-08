#include "gifencoder.h"
#include <QSet>
#include <QHash>
#include <QtMath>

using namespace PixelPaint;

GifEncoder::GifEncoder()
    : m_width(0)
    , m_height(0)
    , m_started(false)
{
}

GifEncoder::~GifEncoder()
{
    if (m_started) end();
}

bool GifEncoder::begin(const QString &filePath, int width, int height, int loopCount)
{
    m_width = width;
    m_height = height;

    m_file.setFileName(filePath);
    if (!m_file.open(QIODevice::WriteOnly)) return false;

    // GIF89a header
    writeBytes("GIF89a", 6);

    // Logical Screen Descriptor
    writeWord(width);
    writeWord(height);
    writeByte(0x70); // no global color table, 8-bit color depth
    writeByte(0);    // background color index
    writeByte(0);    // pixel aspect ratio

    // Netscape Application Extension (for looping)
    writeByte(0x21); // extension introducer
    writeByte(0xFF); // application extension
    writeByte(11);   // block size
    writeBytes("NETSCAPE2.0", 11);
    writeByte(3);    // sub-block size
    writeByte(1);    // sub-block ID
    writeWord(loopCount); // 0 = infinite
    writeByte(0);    // block terminator

    m_started = true;
    return true;
}

bool GifEncoder::addFrame(const QImage &frame, int delayCentiseconds)
{
    if (!m_started) return false;

    QImage img = frame.convertToFormat(QImage::Format_ARGB32);
    if (img.width() != m_width || img.height() != m_height) {
        img = img.scaled(m_width, m_height, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    }

    Palette palette = quantize(img);
    int paletteSize = 1 << palette.bits;

    // Graphic Control Extension
    writeByte(0x21); // extension introducer
    writeByte(0xF9); // graphic control
    writeByte(4);    // block size
    uint8_t flags = 0x00;
    if (palette.transparentIndex >= 0) {
        flags |= 0x01; // transparent color flag
    }
    flags |= (2 << 2); // disposal method: restore to background
    writeByte(flags);
    writeWord(delayCentiseconds);
    writeByte(palette.transparentIndex >= 0 ? palette.transparentIndex : 0);
    writeByte(0); // block terminator

    // Image Descriptor
    writeByte(0x2C); // image separator
    writeWord(0);    // left
    writeWord(0);    // top
    writeWord(m_width);
    writeWord(m_height);
    uint8_t imgFlags = 0x80 | (palette.bits - 1); // local color table, size
    writeByte(imgFlags);

    // Local Color Table
    for (int i = 0; i < paletteSize; ++i) {
        if (i < palette.colors.size()) {
            QRgb c = palette.colors[i];
            writeByte(qRed(c));
            writeByte(qGreen(c));
            writeByte(qBlue(c));
        } else {
            writeByte(0); writeByte(0); writeByte(0);
        }
    }

    // LZW compressed image data
    int minCodeSize = qMax(2, palette.bits);
    writeByte(minCodeSize);

    QByteArray compressed = lzwCompress(palette.indexed, minCodeSize);

    // Write in sub-blocks (max 255 bytes each)
    int pos = 0;
    while (pos < compressed.size()) {
        int blockSize = qMin(255, (int)compressed.size() - pos);
        writeByte(blockSize);
        writeBytes(compressed.constData() + pos, blockSize);
        pos += blockSize;
    }
    writeByte(0); // block terminator

    return true;
}

bool GifEncoder::end()
{
    if (!m_started) return false;

    writeByte(0x3B); // GIF trailer
    m_file.close();
    m_started = false;
    return true;
}

GifEncoder::Palette GifEncoder::quantize(const QImage &image) const
{
    Palette result;
    result.transparentIndex = -1;

    // Collect unique colors
    QHash<QRgb, int> colorMap;
    bool hasTransparency = false;

    for (int y = 0; y < image.height(); ++y) {
        const QRgb *line = reinterpret_cast<const QRgb*>(image.constScanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            QRgb pixel = line[x];
            if (qAlpha(pixel) < 128) {
                hasTransparency = true;
            } else {
                // Force full opacity for palette
                pixel = pixel | 0xFF000000;
                if (!colorMap.contains(pixel)) {
                    colorMap[pixel] = 0; // placeholder
                }
            }
        }
    }

    // Build palette
    QVector<QRgb> colors = colorMap.keys().toVector();

    // Reserve slot 0 for transparency if needed
    if (hasTransparency) {
        result.transparentIndex = 0;
        result.colors.append(qRgb(0, 0, 0)); // transparent placeholder
        int idx = 1;
        for (QRgb c : colors) {
            colorMap[c] = idx++;
            result.colors.append(c);
            if (idx >= 256) break;
        }
    } else {
        int idx = 0;
        for (QRgb c : colors) {
            colorMap[c] = idx++;
            result.colors.append(c);
            if (idx >= 256) break;
        }
    }

    // Calculate palette bit depth
    int paletteCount = result.colors.size();
    result.bits = 1;
    while ((1 << result.bits) < paletteCount) result.bits++;
    if (result.bits > 8) result.bits = 8;

    // Map pixels to indices
    result.indexed.resize(image.width() * image.height());
    for (int y = 0; y < image.height(); ++y) {
        const QRgb *line = reinterpret_cast<const QRgb*>(image.constScanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            QRgb pixel = line[x];
            if (qAlpha(pixel) < 128) {
                result.indexed[y * image.width() + x] = result.transparentIndex;
            } else {
                pixel = pixel | 0xFF000000;
                result.indexed[y * image.width() + x] = colorMap.value(pixel, 0);
            }
        }
    }

    return result;
}

QByteArray GifEncoder::lzwCompress(const QVector<uint8_t> &data, int minCodeSize) const
{
    int clearCode = 1 << minCodeSize;
    int eoiCode = clearCode + 1;

    // Output bit accumulator
    QByteArray output;
    uint32_t bitBuffer = 0;
    int bitCount = 0;

    auto emitCode = [&](int code, int codeSize) {
        bitBuffer |= (code << bitCount);
        bitCount += codeSize;
        while (bitCount >= 8) {
            output.append(static_cast<char>(bitBuffer & 0xFF));
            bitBuffer >>= 8;
            bitCount -= 8;
        }
    };

    // LZW dictionary
    QHash<QByteArray, int> dictionary;
    int nextCode = eoiCode + 1;
    int codeSize = minCodeSize + 1;

    // Initialize dictionary with single-byte entries
    auto resetDictionary = [&]() {
        dictionary.clear();
        for (int i = 0; i < clearCode; ++i) {
            QByteArray key(1, static_cast<char>(i));
            dictionary[key] = i;
        }
        nextCode = eoiCode + 1;
        codeSize = minCodeSize + 1;
    };

    resetDictionary();
    emitCode(clearCode, codeSize);

    if (data.isEmpty()) {
        emitCode(eoiCode, codeSize);
        if (bitCount > 0) output.append(static_cast<char>(bitBuffer & 0xFF));
        return output;
    }

    QByteArray current(1, static_cast<char>(data[0]));

    for (int i = 1; i < data.size(); ++i) {
        QByteArray next = current;
        next.append(static_cast<char>(data[i]));

        if (dictionary.contains(next)) {
            current = next;
        } else {
            emitCode(dictionary[current], codeSize);

            if (nextCode < 4096) {
                dictionary[next] = nextCode++;
                if (nextCode > (1 << codeSize) && codeSize < 12) {
                    codeSize++;
                }
            } else {
                // Table full — emit clear code and reset
                emitCode(clearCode, codeSize);
                resetDictionary();
            }

            current = QByteArray(1, static_cast<char>(data[i]));
        }
    }

    // Emit final code
    emitCode(dictionary[current], codeSize);
    emitCode(eoiCode, codeSize);

    // Flush remaining bits
    if (bitCount > 0) {
        output.append(static_cast<char>(bitBuffer & 0xFF));
    }

    return output;
}

void GifEncoder::writeByte(uint8_t b)
{
    m_file.write(reinterpret_cast<const char*>(&b), 1);
}

void GifEncoder::writeWord(uint16_t w)
{
    writeByte(w & 0xFF);
    writeByte((w >> 8) & 0xFF);
}

void GifEncoder::writeBytes(const char *data, int len)
{
    m_file.write(data, len);
}
