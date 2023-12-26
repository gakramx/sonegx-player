#include "aes.h"
#include <qaesencryption.h>
#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QDebug>
#include <QJniObject>
#include <QJniEnvironment>
#include <QUrl>
#include <QDir>
#include <QtCore/private/qandroidextras_p.h>
AES::AES(QObject *parent)
    : QObject{parent},tempDir(nullptr)
{
    m_customPath=createCustomPath();
}
QString AES::convertUriToPath(const QString &uriString){
    qDebug()<<"uriString : "<<uriString;
    // Create a QJniObject from the URI string
    QJniObject uriObject = QJniObject::fromString(uriString);
    QJniObject uri = QJniObject::callStaticObjectMethod(
        "android/net/Uri",
        "parse",
        "(Ljava/lang/String;)Landroid/net/Uri;",
        uriObject.object<jstring>()
        );

    // Get the context from the main application instance
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    QJniObject context = activity.callObjectMethod("getApplicationContext", "()Landroid/content/Context;");
    // Call the Java method to get the file path
    QJniObject filePath= QJniObject::callStaticObjectMethod(
        "org/sonegx/sonegxplayer/MyUtils",
        "getPath",
        "(Landroid/content/Context;Landroid/net/Uri;)Ljava/lang/String;",
        context.object<jobject>(),
        uri.object<jobject>()
        );

    // Check if filePath is null
    if (!filePath.isValid()) {
        qDebug() << "Error: Unable to get file path from URI";
        return QString();  // Return an empty string on error
    }

    // Convert the Java string to a Qt string and return it
    return QDir::fromNativeSeparators(filePath.toString());
//    // Create a QJniObject from the URI string
//    QJniObject uriObject = QJniObject::fromString(uriString);

//    // Get the Java environment
//    QJniEnvironment env;

//    // Call the appropriate Java method to get the path
//    QJniObject pathObject = QJniObject::callStaticObjectMethod(
//        "android/net/Uri",
//        "parse",
//        "(Ljava/lang/String;)Landroid/net/Uri;",
//        env->NewStringUTF(uriString.toStdString().c_str())
//        );

//    // Check if pathObject is null
//    if (!pathObject.isValid()) {
//        qDebug() << "Error: Unable to parse URI";
//        return QString();  // Return an empty string on error
//    }

//    QJniObject pathString = pathObject.callObjectMethod(
//        "getPath",
//        "()Ljava/lang/String;"
//        );

//    // Check if pathString is null
//    if (!pathString.isValid()) {
//        qDebug() << "Error: Unable to get path from URI";
//        return QString();  // Return an empty string on error
//    }

//    // Convert the Java string to a Qt string and return it
//    return QDir::fromNativeSeparators(pathString.toString());
}
QVariant AES::encrypt(const QString& filePath, QByteArray key)
{
    QFile inputFile(filePath);
    if (!inputFile.open(QIODevice::ReadOnly))
    {
        // Handle file opening error
        return QVariant();
    }

    QByteArray plainText = inputFile.readAll();
    inputFile.close();

    QAESEncryption encryption(QAESEncryption::AES_128, QAESEncryption::ECB);
    QByteArray encodedText = encryption.encode(plainText, key);

    // Save the encrypted content to a new file
    QString encryptedFilePath = filePath + ".sgr";
    QFile outputFile(encryptedFilePath);
    if (!outputFile.open(QIODevice::WriteOnly))
    {
        // Handle file saving error
        return QVariant();
    }

    outputFile.write(encodedText);
    outputFile.close();
    qDebug()<<"encryp "<<encryptedFilePath;
    return QVariant::fromValue(encryptedFilePath);
}

QVariant AES::decrypt(const QString& filePath, QByteArray key)
{
    QUrl url(filePath);
    QString localName = url.isLocalFile() ? url.toLocalFile() : filePath;
    qDebug()<<"FILES : "<<localName;
    QFile inputFile(localName);
    QFileInfo fileInfo(inputFile);
    QString baseName = fileInfo.baseName();
    qDebug()<<"baseName : "<<baseName;
    if (!inputFile.open(QIODevice::ReadOnly))
    {
        // Handle file opening error
        return QVariant();
    }

    QByteArray encodedText = inputFile.readAll();
    inputFile.close();

    QAESEncryption encryption(QAESEncryption::AES_128, QAESEncryption::ECB);
    QByteArray decodedText = encryption.decode(encodedText, key);
    QString decodedString = QString(encryption.removePadding(decodedText));

    // Save the decrypted content to a new file
    QString _fullname = getoutputFullFilename();
    QString decryptedFilePath = _fullname + "/" + baseName;
    qDebug()<<"decryptedFilePath : "<<decryptedFilePath;
    QFile outputFile(decryptedFilePath);
    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        // Handle file saving error
        return QVariant();
    }

    QTextStream stream(&outputFile);
    stream << decodedString;
    outputFile.close();
    // qDebug()<<"Decrypt "<<decryptedFilePath;
    emit decryptionProjectFinished(decryptedFilePath);
    return QVariant::fromValue(decryptedFilePath);
}

QFuture<bool> AES::encryptVideo(const QString &inputFilePath, const QString &outputFilePath, const QByteArray &encryptionKey)
{

    return QtConcurrent::run([this,inputFilePath, outputFilePath, encryptionKey]() {
        QUrl url(inputFilePath);
        QString local_inputFilePath = url.isLocalFile() ? url.toLocalFile() : inputFilePath;
        qDebug()<<"FILES : "<<local_inputFilePath;
        QFile inputFile(local_inputFilePath);
        QFile outputFile(outputFilePath);

        if (!inputFile.open(QIODevice::ReadOnly)) {
            // Failed to open input file
            return false;
        }
        if (!outputFile.open(QIODevice::WriteOnly)) {
            // Failed to open output file
            inputFile.close();
            return false;
        }


        qint64 totalBytes = inputFile.size();
        qint64 bytesProcessed = 0;

        const int bufferSize = 1024 * 1024; // 1MB
        char buffer[bufferSize];

        int keyLength = encryptionKey.length();
        int keyIndex = 0;
        while (!inputFile.atEnd()) {
            qint64 bytesRead = inputFile.read(buffer, bufferSize);

            for (qint64 i = 0; i < bytesRead; ++i) {
                buffer[i] = buffer[i] ^ encryptionKey[keyIndex];

                keyIndex++;
                if (keyIndex == keyLength) {
                    keyIndex = 0;
                }
            }

            outputFile.write(buffer, bytesRead);

            bytesProcessed += bytesRead;
            int progress = static_cast<int>((bytesProcessed * 100) / totalBytes);
            qDebug() << "Encryption progress:" << progress << "%";
            emit encryptionVideoProgressChanged(progress);
        }
        outputFile.close();
        inputFile.close();

        return true;
    });
}

QFuture<bool> AES::decryptVideo(const QString &inputFilePath, const QString &outputFilePath, const QByteArray &encryptionKey)
{

    return QtConcurrent::run([this, inputFilePath, outputFilePath, encryptionKey]() {
        createTempDir(m_customPath);
        setoutputFullFilename(tempDir->path());
        // TODO: check file is exist before
        if (!tempDir->isValid()) {
            return false;
        }
        QUrl url(inputFilePath);

        QString local_inputFilePath = url.isLocalFile() ? url.toLocalFile() : inputFilePath;
        qDebug() << "FILES: " << local_inputFilePath;


        QString _fullname = getoutputFullFilename();
        QString file=inputFilePath+_fullname;
         QUrl url2(file);
        QString local_inputFilePath2 = url2.toLocalFile();
            qDebug() << "FILES local_inputFilePath2 : " << local_inputFilePath2;
        QString fullname = _fullname + "/" + outputFilePath;
        QFile inputFile(local_inputFilePath);
        QFile outputFile(fullname);
        qDebug() << "PATH: " << fullname;

        if (!inputFile.open(QIODevice::ReadOnly)) {
            // Failed to open input file
            return false;
        }

        if (!outputFile.open(QIODevice::WriteOnly)) {
            // Failed to open output file
            inputFile.close();
            return false;
        }

        qint64 totalBytes = inputFile.size();
        qint64 chunkSize = 1024 * 1024; // 1MB
        qint64 totalChunks = totalBytes / chunkSize;
        if (totalBytes % chunkSize != 0) {
            totalChunks++;
        }

        QList<QByteArray> chunks;
        QByteArray buffer(chunkSize, 0);

        while (!inputFile.atEnd()) {
            emit preparingVideoProgressChanged();
            qint64 bytesRead = inputFile.read(buffer.data(), chunkSize);
            QByteArray chunk(buffer.constData(), bytesRead);
            chunks.append(chunk);
        }

        inputFile.close();

        QMutex mutex;
        qint64 chunksProcessed = 0;

        QList<QByteArray> decryptedChunks = QtConcurrent::blockingMapped(chunks, [this,encryptionKey, &mutex, &chunksProcessed, totalChunks](const QByteArray& chunk) {
            int keyLength = encryptionKey.length();
            int keyIndex = 0;

            QByteArray decryptedChunk(chunk.size(), 0);

            for (int i = 0; i < decryptedChunk.size(); ++i) {
                decryptedChunk[i] = chunk[i] ^ encryptionKey[keyIndex];
                keyIndex = (keyIndex + 1) % keyLength;
            }

            {
                QMutexLocker locker(&mutex);
                chunksProcessed++;
                int progress = static_cast<int>((chunksProcessed * 100) / totalChunks);
                qDebug() << "Decryption progress:" << progress << "%";
                emit encryptionVideoProgressChanged(progress);
            }

            return decryptedChunk;
        });

        for (const QByteArray& decryptedChunk : decryptedChunks) {
            outputFile.write(decryptedChunk.constData(), decryptedChunk.size());
        }

        outputFile.close();

        emit decryptionVideoFinished(fullname);
        return true;
    });
}
QString AES::getoutputFullFilename() const {
    return outputFullFilename;
}
void AES::setoutputFullFilename(const QString& newFilename)
{
    outputFullFilename = newFilename;
}
QString AES::createCustomPath() {
    QString cacheLocation = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QString folder = "Video";
    QString customPath = cacheLocation + "/" + folder+ "/";
    // Delete all files and folders within the customPath
    QDir dirCustomPath(customPath);
    dirCustomPath.removeRecursively();
    QDir().mkpath(customPath);
    return customPath;
}
void AES::createTempDir(const QString &templatePath){

    if (tempDir != nullptr && tempDir->isValid()) {
        delete tempDir;
    }
    tempDir = new QTemporaryDir(templatePath);
    if (!tempDir->isValid()) {
        qDebug() << "Not created ! ! ";
        // Handle error, e.g., by throwing an exception or returning an error code
    }
}
AES::~AES()
{
    if (tempDir != nullptr) {
        delete tempDir;
    }
    // Destructor implementation
    //   dir.removeRecursively();
}
