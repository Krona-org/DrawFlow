#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QUrl>

class QNetworkReply;
class QTemporaryDir;

namespace editor {

class UpdateManager : public QObject
{
    Q_OBJECT
public:
    explicit UpdateManager(QObject *parent = nullptr);

    void checkForUpdates(bool silent = false);
    void downloadAndInstallUpdate(const QUrl &assetUrl);

signals:
    void updateAvailable(const QString &version, const QUrl &assetUrl);
    void noUpdateAvailable();
    void errorOccurred(const QString &message);
    void downloadProgress(qint64 received, qint64 total);
    // The caller should save user data and quit the application after this signal.
    void installationStarted();

private:
    void reportError(const QString &message, bool silent = false);
    QNetworkAccessManager m_network;
    bool m_busy = false;
};

} // namespace editor
