#include <QApplication>
#include <QQmlApplicationEngine>
#include "plot/quickplotqmlplugin.h"

#include "deviceList.h"
#include "libOpenHantek2xxx-5xxx/init.h"
#include "libOpenHantek60xx/init.h"
#include "dataAnalyzer.h"
#include "scopecolors.h"
#include "modeldatasetter.h"
#include "devicemodel.h"
#include "dsoerrorstrings.h"
#include "currentdevice.h"

#include <QQuickWindow>
#include <QQmlContext>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationVersion("1.0");

    // Register QML classes
    QuickPlotQmlPlugin p;
    p.registerTypes("QuickPlot");
    qmlRegisterType<ModelDataSetter>("ModelDataSetter", 1, 0, "ModelDataSetter");

    ErrorStrings errorStrings;
    qmlRegisterType<ErrorStrings>("ErrorStrings", 1, 0, "ErrorStrings");

    DSO::DeviceList deviceList;
    CurrentDevice currentDevice(&deviceList);
    DeviceModel deviceModel(&deviceList);
    ScopeColors screenColors("screen");

    // Connect deviceList with model, register all known usb identifiers

    deviceList._listChanged = [&deviceModel]() {
        deviceModel.update();
    };
    deviceList._modelsChanged = [&deviceModel]() {
        deviceModel.updateSupportedDevices();
    };
    Hantek2xxx_5xxx::registerHantek2xxx_5xxxProducts(deviceList);
    Hantek60xx::registerHantek60xxProducts(deviceList);
    deviceList.setAutoUpdate(true);
    deviceList.update();

    // Start QML engine
    QQmlApplicationEngine engine;
    engine.addImportPath(":/QuickPlot");
    engine.rootContext()->setContextProperty("screenColors", &screenColors);
    engine.rootContext()->setContextProperty("deviceModel", &deviceModel);
    engine.rootContext()->setContextProperty("currentDevice", &currentDevice);
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().size() == 0) {
        qWarning() << "Failed to load qml file!";
        return -1;
    }
    QObject     *GuiHantek  = engine.rootObjects().first();
    GuiHantek->setProperty("version", app.applicationVersion());
    return app.exec();
}
