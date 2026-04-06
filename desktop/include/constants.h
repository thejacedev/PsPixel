#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QColor>
#include <QList>
#include "version.h"

namespace PixelPaint {

// Canvas default settings
constexpr int DEFAULT_CANVAS_WIDTH = 64;
constexpr int DEFAULT_CANVAS_HEIGHT = 64;
constexpr int DEFAULT_PIXEL_SIZE = 8;
constexpr int MIN_CANVAS_SIZE = 10;
constexpr int MAX_CANVAS_SIZE = 4096;

// Brush settings
constexpr int DEFAULT_BRUSH_SIZE = 1;
constexpr int MIN_BRUSH_SIZE = 1;
constexpr int MAX_BRUSH_SIZE = 10;

// Zoom and Pan settings
constexpr double DEFAULT_ZOOM_FACTOR = 1.0;
constexpr double MIN_ZOOM_FACTOR = 0.1;
constexpr double MAX_ZOOM_FACTOR = 10.0;
constexpr double ZOOM_STEP = 0.1;
constexpr double ZOOM_WHEEL_FACTOR = 1.2;

// UI settings
constexpr int COLOR_BUTTON_SIZE = 25;
constexpr int MAIN_COLOR_BUTTON_WIDTH = 100;
constexpr int MAIN_COLOR_BUTTON_HEIGHT = 30;
constexpr int MIN_WINDOW_WIDTH = 800;
constexpr int MIN_WINDOW_HEIGHT = 600;
constexpr int DEFAULT_WINDOW_WIDTH = 1000;
constexpr int DEFAULT_WINDOW_HEIGHT = 700;

// Toolbar settings
constexpr int TOOL_BUTTON_SIZE = 32;
constexpr int TOOLBAR_SPACING = 5;
constexpr int TOOLBAR_MARGIN = 10;

// Tool types
enum class ToolType {
    Select,
    Brush,
    Eraser,
    Eyedropper,
    PaintBucket,
    Line,
    Rectangle,
    Circle
};

// Default colors for palette
inline QList<QColor> getDefaultPalette() {
    return {
        Qt::black, Qt::white, Qt::red, Qt::green, Qt::blue,
        Qt::yellow, Qt::cyan, Qt::magenta, Qt::gray, Qt::darkRed,
        Qt::darkGreen, Qt::darkBlue, Qt::darkYellow, Qt::darkCyan,
        Qt::darkMagenta, Qt::lightGray
    };
}

// Application info
constexpr const char* APP_NAME = "PsPixel";
constexpr const char* APP_DESCRIPTION = "Professional Pixel Art Editor";
constexpr const char* VERSION = PSPIXEL_VERSION;
constexpr const char* GITHUB_REPO = "thejacedev/PsPixel";

// Canvas colors
inline QColor getDefaultBackgroundColor() {
    return QColor(0, 0, 0, 0); // Transparent
}
const QColor CHECKERBOARD_LIGHT(240, 240, 240);
const QColor CHECKERBOARD_DARK(200, 200, 200);
const QColor CANVAS_BACKGROUND(64, 64, 64);

// History palette colors
const QColor HISTORY_CURRENT_BG(100, 150, 200, 100);
const QColor HISTORY_FUTURE_TEXT(128, 128, 128);

// File filters
constexpr const char* SAVE_FILE_FILTER = "PNG Files (*.png);;All Files (*)";
constexpr const char* LOAD_FILE_FILTER = "Image Files (*.png *.jpg *.jpeg *.bmp);;All Files (*)";

} // namespace PixelPaint

#endif // CONSTANTS_H 