#include "autoupdater.h"
#include "constants.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QSysInfo>
#include <QPushButton>

using namespace PixelPaint;

AutoUpdater::AutoUpdater(QWidget *parentWidget, QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
    , m_parentWidget(parentWidget)
    , m_silent(true)
{
    connect(m_network, &QNetworkAccessManager::finished, this, &AutoUpdater::onCheckFinished);
}

void AutoUpdater::checkForUpdates(bool silent)
{
    m_silent = silent;

    QUrl url(QString("https://api.github.com/repos/%1/releases/latest").arg(GITHUB_REPO));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QString("%1/%2").arg(APP_NAME, VERSION));
    request.setRawHeader("Accept", "application/vnd.github.v3+json");
    m_network->get(request);
}

void AutoUpdater::onCheckFinished(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isObject()) return;

    QJsonObject release = doc.object();
    QString tagName = release["tag_name"].toString();
    QString remoteVersion = tagName.startsWith("v") ? tagName.mid(1) : tagName;

    if (!isNewerVersion(remoteVersion, VERSION)) {
        if (!m_silent) {
            QMessageBox::information(m_parentWidget, "No Updates",
                QString("You're running the latest version (%1).").arg(VERSION));
        }
        return;
    }

    // Find platform-specific download asset
    ReleaseInfo info;
    info.version = remoteVersion;
    info.htmlUrl = release["html_url"].toString();
    info.body = release["body"].toString();

    QString pattern = platformAssetPattern();
    QJsonArray assets = release["assets"].toArray();
    for (const QJsonValue &val : assets) {
        QJsonObject asset = val.toObject();
        QString name = asset["name"].toString();
        if (name.contains(pattern, Qt::CaseInsensitive)) {
            info.downloadUrl = asset["browser_download_url"].toString();
            break;
        }
    }

    // Fallback to release page if no matching asset
    if (info.downloadUrl.isEmpty()) {
        info.downloadUrl = info.htmlUrl;
    }

    showUpdateDialog(info);
}

bool AutoUpdater::isNewerVersion(const QString &remote, const QString &local) const
{
    QStringList remoteParts = remote.split(".");
    QStringList localParts = QString(local).split(".");

    for (int i = 0; i < qMax(remoteParts.size(), localParts.size()); ++i) {
        int r = i < remoteParts.size() ? remoteParts[i].toInt() : 0;
        int l = i < localParts.size() ? localParts[i].toInt() : 0;
        if (r > l) return true;
        if (r < l) return false;
    }
    return false;
}

QString AutoUpdater::platformAssetPattern() const
{
#ifdef Q_OS_WIN
    return "Setup.exe";
#elif defined(Q_OS_MACOS)
    return "macos.zip";
#else
    return "AppImage";
#endif
}

void AutoUpdater::showUpdateDialog(const ReleaseInfo &release)
{
    QMessageBox dialog(m_parentWidget);
    dialog.setWindowTitle("Update Available");
    dialog.setIcon(QMessageBox::Information);
    dialog.setText(QString("<b>PsPixel v%1 is available!</b><br>You're currently on v%2.")
        .arg(release.version, VERSION));

    // Trim release body to first 500 chars for the dialog
    QString notes = release.body;
    if (notes.length() > 500) notes = notes.left(500) + "...";
    dialog.setInformativeText(notes);

    QPushButton *downloadBtn = dialog.addButton("Download Update", QMessageBox::AcceptRole);
    dialog.addButton("Skip", QMessageBox::RejectRole);

    dialog.exec();

    if (dialog.clickedButton() == downloadBtn) {
        QDesktopServices::openUrl(QUrl(release.downloadUrl));
    }
}
