#include "pspxformat.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>
#include <QApplication>

using namespace PixelPaint;

bool PSPXFormat::saveProject(const QString &filePath, const ProjectData &data)
{
    QJsonObject root;
    
    // Metadata
    root["format"] = "PSPX";
    root["version"] = "1.0";
    root["appName"] = QApplication::applicationName();
    root["appVersion"] = QApplication::applicationVersion();
    
    // Project info
    root["projectName"] = data.projectName;
    root["createdTimestamp"] = data.createdTimestamp;
    root["modifiedTimestamp"] = QDateTime::currentSecsSinceEpoch();
    
    // Canvas properties
    QJsonObject canvas;
    canvas["width"] = data.canvasWidth;
    canvas["height"] = data.canvasHeight;
    canvas["pixelSize"] = data.pixelSize;
    root["canvas"] = canvas;
    
    // Pixel data - compress by storing only non-transparent pixels
    QJsonArray pixelArray;
    for (int y = 0; y < data.canvasHeight; ++y) {
        for (int x = 0; x < data.canvasWidth; ++x) {
            QColor color = data.pixelData.pixelColor(x, y);
            if (color.alpha() == 255) {
                QJsonObject pixel;
                pixel["x"] = x;
                pixel["y"] = y;
                pixel["color"] = colorToJson(color);
                pixelArray.append(pixel);
            }
        }
    }
    root["pixels"] = pixelArray;
    
    // Write to file
    QJsonDocument doc(root);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    file.write(doc.toJson());
    return true;
}

bool PSPXFormat::loadProject(const QString &filePath, ProjectData &data)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError) {
        return false;
    }
    
    QJsonObject root = doc.object();
    
    // Validate format
    if (root["format"].toString() != "PSPX") {
        return false;
    }
    
    // Load project info
    data.projectName = root["projectName"].toString();
    data.createdTimestamp = root["createdTimestamp"].toVariant().toLongLong();
    data.modifiedTimestamp = root["modifiedTimestamp"].toVariant().toLongLong();
    data.lastSavedPath = filePath;
    
    // Load canvas properties
    QJsonObject canvas = root["canvas"].toObject();
    data.canvasWidth = canvas["width"].toInt();
    data.canvasHeight = canvas["height"].toInt();
    data.pixelSize = canvas["pixelSize"].toInt();
    
    // Initialize canvas with transparent background
    data.pixelData = QImage(data.canvasWidth, data.canvasHeight, QImage::Format_ARGB32);
    data.pixelData.fill(Qt::transparent);

    // Load pixel data
    QJsonArray pixelArray = root["pixels"].toArray();
    for (const QJsonValue &pixelValue : pixelArray) {
        QJsonObject pixel = pixelValue.toObject();
        int x = pixel["x"].toInt();
        int y = pixel["y"].toInt();
        QColor color = jsonToColor(pixel["color"].toObject());

        if (x >= 0 && x < data.canvasWidth && y >= 0 && y < data.canvasHeight) {
            data.pixelData.setPixelColor(x, y, color);
        }
    }
    
    return true;
}

bool PSPXFormat::isValidPSPXFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError) {
        return false;
    }
    
    QJsonObject root = doc.object();
    return root["format"].toString() == "PSPX";
}

QJsonObject PSPXFormat::colorToJson(const QColor &color)
{
    QJsonObject colorObj;
    colorObj["r"] = color.red();
    colorObj["g"] = color.green();
    colorObj["b"] = color.blue();
    colorObj["a"] = color.alpha();
    return colorObj;
}

QColor PSPXFormat::jsonToColor(const QJsonObject &colorObj)
{
    return QColor(
        colorObj["r"].toInt(),
        colorObj["g"].toInt(),
        colorObj["b"].toInt(),
        colorObj["a"].toInt()
    );
}

QString PSPXFormat::getDefaultProjectsPath()
{
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appDataPath);
    if (!dir.exists()) {
        dir.mkpath(appDataPath);
    }
    return appDataPath + "/Projects";
} 