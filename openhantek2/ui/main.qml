import QtQuick 2.4
import QtQuick.Controls 1.4
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.2
import QtQuick.Extras 1.4 // A straight gauge; CircularGauge; Dial
import Qt.labs.settings 1.0

ApplicationWindow {
    id: window
    title: qsTr("OpenHantek")
    width: 640
    height: 480
    visible: true

    property string version: "0.0"

    signal qmlGetDevicesInfo()
    signal qmlSearchDevices(string deviceName)
    signal qmlInstallFW(string deviceName)

    Settings {
          property alias x: window.x
          property alias y: window.y
          property alias width: window.width
          property alias height: window.height
      }

    menuBar: MenuBar {
        Menu {
            title: qsTr("&OpenHantek")
            MenuItem {
                text: qsTr("&Print")
                shortcut: "Ctrl+P"
                onTriggered: messageDialog.show(qsTr("To be implemented"));
            }
            MenuItem {
                text: qsTr("&Export as...")
                shortcut: "Ctrl+E"
                onTriggered: messageDialog.show(qsTr("To be implemented"));
            }
            MenuSeparator { }
            MenuItem {
                text: qsTr("E&xit")
                onTriggered: Qt.quit();
            }
        }
        Menu {
            title: qsTr("&Help")
            MenuItem {
                text: qsTr("Version: %1").arg(window.version)
                enabled: false
            }
            MenuSeparator { }
            MenuItem {
                text: qsTr("&Show website")
                shortcut: "Ctrl+W"
                onTriggered: Qt.openUrlExternally("http://openhantek.org");
            }
            MenuItem {
                text: qsTr("&Report an issue")
                shortcut: "Ctrl+R"
                onTriggered: Qt.openUrlExternally("http://github.com/openhantek/openhantek/issues");
            }
        }
    }
    MainForm {
        id: main
        //var texto = qsTr("6022BL")
        anchors.fill: parent
        //button2.onClicked:
        //{
            //text: qsTr("6022BL")
            //messageDialog.show(qsTr("Button 2 pressed"))
        //    window.qmlSearchDevices("6022BL");
        //}
    }

    MessageDialog {
        id: messageDialog
        title: qsTr("May I have your attention, please?")
        function show(caption) {
            messageDialog.text = caption;
            messageDialog.open();
        }
    }
}
/**
signals:
    void deviceConnected(HT6022_ErrorTypeDef errorCode);
    void deviceReady(HT6022_ErrorTypeDef errorCode);
    void sendDevicesInfo(QList<HT6022BX_Info> *DeviceName);
    qml slots:
**/
