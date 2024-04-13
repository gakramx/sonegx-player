#ifndef UTILS_H
#define UTILS_H

#include <QObject>
#include <QNetworkInterface>
#include <QSysInfo>
#include <QCoreApplication>
#include <QDebug>
#include <QtCore/private/qandroidextras_p.h>
#include <QFile>
#include <QSettings>
#include <QUuid>
#include <QStandardPaths>
#include <QTextStream>
#include <QCryptographicHash>
#include <QJniObject>
#include <QJniEnvironment>
#include <QFileInfo>
#include <QDir>
#include <QDesktopServices>
#include <QFileInfo>
#include <QUrl>

class utils : public QObject
{
    Q_OBJECT
public:
    explicit utils(QObject *parent = nullptr);
    Q_INVOKABLE QString convertUriToPathFile(const QString &uriString);
    Q_INVOKABLE QString convertUriToPath(const QString &uriString);
    Q_INVOKABLE void share(const QString &urlfile);
    Q_INVOKABLE  bool isFileExists(QString filePath);
    Q_INVOKABLE QString getAndroidID();
    Q_INVOKABLE bool checkStoragePermission();
    Q_INVOKABLE void setSecureFlag();
    Q_INVOKABLE bool rotateToLandscape();
    Q_INVOKABLE bool rotateToPortrait();
    QString hashAndFormat(const QString& androidId);
signals:

};

#endif // UTILS_H
