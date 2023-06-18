#include <QApplication>
#include <QQmlApplicationEngine>
#ifdef Q_OS_ANDROID
#include "utils.h"
#endif
#include <QQmlContext>
#include "clipboardextension.h"
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
   // qputenv("QT_QUICK_CONTROLS_STYLE", QByteArray("Material"));
   // qputenv("QT_QUICK_CONTROLS_MATERIAL_THEME", QByteArray("Dark"));
    QQmlApplicationEngine engine;
    #ifdef Q_OS_ANDROID
    utils android;
    engine.rootContext()->setContextProperty("androidUtils", &android);
    #endif
    ClipboardExtension ClipboardExt;
    engine.rootContext()->setContextProperty("clipboardExtension", &ClipboardExt);
      //  qmlRegisterType<ClipboardExtension>("ClipboardExtension", 1, 0, "ClipboardExtension");
    const QUrl url(u"qrc:/qml/Main.qml"_qs);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
