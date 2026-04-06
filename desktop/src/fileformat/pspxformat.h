#ifndef PSPXFORMAT_H
#define PSPXFORMAT_H

#include <QString>
#include <QVector>
#include <QColor>
#include <QImage>
#include <QJsonObject>
#include <QJsonDocument>

namespace PixelPaint {

struct ProjectData {
    int canvasWidth;
    int canvasHeight;
    int pixelSize;
    QImage pixelData;
    QString projectName;
    QString lastSavedPath;
    qint64 createdTimestamp;
    qint64 modifiedTimestamp;
};

class PSPXFormat
{
public:
    static bool saveProject(const QString &filePath, const ProjectData &data);
    static bool loadProject(const QString &filePath, ProjectData &data);
    static bool isValidPSPXFile(const QString &filePath);
    
private:
    static QJsonObject colorToJson(const QColor &color);
    static QColor jsonToColor(const QJsonObject &colorObj);
    static QString getDefaultProjectsPath();
};

} // namespace PixelPaint

#endif // PSPXFORMAT_H 