#include <QApplication>
#include <QQmlApplicationEngine>
#include "plot/quickplotqmlplugin.h"

#include <QQuickWindow>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationVersion("1.0");

    QuickPlotQmlPlugin p;
    p.registerTypes("QuickPlot");

    QQmlApplicationEngine engine;
    engine.addImportPath(":/QuickPlot");
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    QObject     *GuiHantek  = engine.rootObjects().first();
    QQuickWindow *window = qobject_cast<QQuickWindow *>(GuiHantek);
    GuiHantek->setProperty("version", app.applicationVersion());

    // Multisampling for antialiasing
    QSurfaceFormat format = window->format();
    format.setSamples(16);
    window->setFormat(format);

    //HT6022bx    *hantek     = new HT6022bx();
    //QObject::connect(GuiHantek,SIGNAL(qmlGetDevicesInfo()),hantek, SLOT(getDevicesInfo()));
    //QObject::connect(GuiHantek,SIGNAL(qmlSearchDevices(QString)),hantek, SLOT(searchDevice(QString)));
    //QObject::connect(GuiHantek,SIGNAL(qmlInstallFW(QString)),hantek, SLOT(FirmwareInstall(QString)));
    return app.exec();
}
