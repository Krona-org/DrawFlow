#include "updatemanager.h"
#include "version.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QTemporaryDir>
#include <QVersionNumber>
#include <memory>

namespace editor {

UpdateManager::UpdateManager(QObject *parent) : QObject(parent) {}

void UpdateManager::reportError(const QString &message, bool silent)
{
    m_busy = false;
    if (!silent)
        emit errorOccurred(message);
}

void UpdateManager::checkForUpdates(bool silent)
{
    if (m_busy) {
        if (!silent)
            emit errorOccurred(tr("Another update operation is in progress."));
        return;
    }
    m_busy = true;
    QNetworkRequest request{QUrl(QStringLiteral("https://api.github.com/repos/") + GITHUB_REPO
                                 + QStringLiteral("/releases/latest"))};
    request.setRawHeader("User-Agent", "DrawFlow-update-checker");
    request.setRawHeader("Accept", "application/vnd.github+json");
    auto *reply = m_network.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, silent] {
        const auto error = reply->error();
        const QString errorText = reply->errorString();
        const QByteArray data = reply->readAll();
        reply->deleteLater();
        if (error != QNetworkReply::NoError) {
            reportError(tr("Update check failed: %1").arg(errorText), silent);
            return;
        }
        QJsonParseError parseError;
        const auto document = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            reportError(tr("Invalid release information."), silent);
            return;
        }
        const auto release = document.object();
        QString tag = release.value(QStringLiteral("tag_name")).toString();
        if (tag.startsWith(QLatin1Char('v'), Qt::CaseInsensitive))
            tag.remove(0, 1);
        qsizetype suffix = 0;
        const auto latest = QVersionNumber::fromString(tag, &suffix);
        const auto current = QVersionNumber::fromString(APP_VERSION);
        if (latest.isNull() || suffix != tag.size() || current.isNull()) {
            reportError(tr("Invalid release version: %1").arg(tag), silent);
            return;
        }
        m_busy = false;
        if (QVersionNumber::compare(latest, current) <= 0) {
            if (!silent)
                emit noUpdateAvailable();
            return;
        }
        for (const auto &item : release.value(QStringLiteral("assets")).toArray()) {
            const auto asset = item.toObject();
            const QUrl url(asset.value(QStringLiteral("browser_download_url")).toString());
            if (asset.value(QStringLiteral("name")).toString().endsWith(QStringLiteral(".zip"), Qt::CaseInsensitive)
                && url.isValid() && url.scheme() == QStringLiteral("https")) {
                emit updateAvailable(tag, url);
                return;
            }
        }
        reportError(tr("The release does not contain a ZIP update."), silent);
    });
}

void UpdateManager::downloadAndInstallUpdate(const QUrl &assetUrl)
{
#ifndef Q_OS_WIN
    Q_UNUSED(assetUrl);
    emit errorOccurred(tr("Automatic installation is only supported on Windows."));
#else
    if (m_busy) {
        emit errorOccurred(tr("Another update operation is in progress."));
        return;
    }
    if (!assetUrl.isValid() || assetUrl.scheme() != QStringLiteral("https")) {
        emit errorOccurred(tr("The update URL must use HTTPS."));
        return;
    }
    auto temp = std::make_shared<QTemporaryDir>();
    if (!temp->isValid()) {
        emit errorOccurred(tr("Cannot create a temporary update directory."));
        return;
    }
    auto archive = std::make_shared<QFile>(temp->filePath(QStringLiteral("update.zip")));
    if (!archive->open(QIODevice::WriteOnly)) {
        emit errorOccurred(tr("Cannot create the update archive."));
        return;
    }
    m_busy = true;
    QNetworkRequest request(assetUrl);
    request.setRawHeader("User-Agent", "DrawFlow-update-checker");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    auto *reply = m_network.get(request);
    auto writeFailed = std::make_shared<bool>(false);
    connect(reply, &QNetworkReply::downloadProgress, this, &UpdateManager::downloadProgress);
    connect(reply, &QIODevice::readyRead, this, [reply, archive, writeFailed] {
        const QByteArray data = reply->readAll();
        if (archive->write(data) != data.size()) {
            *writeFailed = true;
            reply->abort();
        }
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, temp, archive, writeFailed] {
        const QByteArray remaining = reply->readAll();
        if (archive->write(remaining) != remaining.size() || !archive->flush())
            *writeFailed = true;
        archive->close();
        const auto error = reply->error();
        const QString errorText = reply->errorString();
        reply->deleteLater();
        if (*writeFailed || error != QNetworkReply::NoError) {
            reportError(tr("Update download failed: %1").arg(*writeFailed ? tr("Cannot write archive") : errorText));
            return;
        }
        const QString extracted = temp->filePath(QStringLiteral("extracted"));
        if (!QDir().mkpath(extracted)) {
            reportError(tr("Cannot create the extraction directory."));
            return;
        }
        auto *process = new QProcess(this);
        connect(process, &QProcess::errorOccurred, this, [this, process](QProcess::ProcessError error) {
            if (error == QProcess::FailedToStart) {
                reportError(tr("Cannot start tar to extract the update."));
                process->deleteLater();
            }
        });
        connect(process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
                [this, process, temp, extracted](int code, QProcess::ExitStatus status) {
            process->deleteLater();
            if (status != QProcess::NormalExit || code != 0) {
                reportError(tr("Cannot extract the update archive."));
                return;
            }
            const QString executable = QFileInfo(QCoreApplication::applicationFilePath()).fileName();
            QString source = extracted;
            if (!QFileInfo::exists(QDir(source).filePath(executable))) {
                const auto dirs = QDir(extracted).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                if (dirs.size() == 1)
                    source = QDir(extracted).filePath(dirs.first());
            }
            if (!QFileInfo::exists(QDir(source).filePath(executable))) {
                reportError(tr("The update archive does not contain %1.").arg(executable));
                return;
            }
            QFile script(temp->filePath(QStringLiteral("install.bat")));
            if (!script.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                reportError(tr("Cannot create the update script."));
                return;
            }
            const QByteArray batch = "@echo off\r\nsetlocal DisableDelayedExpansion\r\n"
                "powershell.exe -NoProfile -NonInteractive -Command \"Wait-Process -Id %~1 -ErrorAction SilentlyContinue\"\r\n"
                "robocopy %2 %3 /E /R:3 /W:1 >nul\r\n"
                "if errorlevel 8 exit /b 1\r\n"
                "start \"\" %4\r\n";
            if (script.write(batch) != batch.size() || !script.flush()) {
                reportError(tr("Cannot write the update script."));
                return;
            }
            script.close();
            const QStringList arguments = {QStringLiteral("/c"), script.fileName(),
                QString::number(QCoreApplication::applicationPid()), QDir::toNativeSeparators(source),
                QDir::toNativeSeparators(QCoreApplication::applicationDirPath()),
                QDir::toNativeSeparators(QCoreApplication::applicationFilePath())};
            if (!QProcess::startDetached(QStringLiteral("cmd.exe"), arguments)) {
                reportError(tr("Cannot start the update installer."));
                return;
            }
            temp->setAutoRemove(false);
            m_busy = false;
            emit installationStarted();
        });
        process->start(QStringLiteral("tar.exe"), {QStringLiteral("-xf"), archive->fileName(),
                       QStringLiteral("-C"), extracted});
    });
#endif
}

} // namespace editor
