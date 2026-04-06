#ifndef AUTOUPDATER_H
#define AUTOUPDATER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>
#include <QWidget>

namespace PixelPaint {

class AutoUpdater : public QObject
{
    Q_OBJECT

public:
    explicit AutoUpdater(QWidget *parentWidget, QObject *parent = nullptr);

    void checkForUpdates(bool silent = true);

private slots:
    void onCheckFinished(QNetworkReply *reply);

private:
    struct ReleaseInfo {
        QString version;
        QString htmlUrl;
        QString body;
        QString downloadUrl; // platform-specific asset
    };

    bool isNewerVersion(const QString &remote, const QString &local) const;
    QString platformAssetPattern() const;
    void showUpdateDialog(const ReleaseInfo &release);

    QNetworkAccessManager *m_network;
    QWidget *m_parentWidget;
    bool m_silent;
};

} // namespace PixelPaint

#endif // AUTOUPDATER_H
