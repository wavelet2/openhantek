import QtQuick 2.4
import QtQuick.Controls 1.3
import QtQuick.Layouts 1.1
import QtQuick.Dialogs 1.2
import Qt.labs.settings 1.0

Dialog {
    id: dialog
    standardButtons: StandardButton.Ok
    title: qsTr("Export")

    width: 300

    GridLayout {
        id: layout
        columns: 2
        rowSpacing: 10
        anchors.left: parent.left
        anchors.right: parent.right
        Layout.minimumWidth: 300

        Text {
            text: qsTr("Graph")
            Layout.columnSpan: 2
        }

        Label { Layout.leftMargin: 10; text: qsTr("Interpolation") }
        ComboBox {
            id: interpolationType
            model: ListModel {
                ListElement { text: qsTr("Off"); type: "off" }
                ListElement { text: qsTr("Linear"); type: "linear" }
                ListElement { text: qsTr("Cubic"); type: "cubic" }
            }
        }

        Text {
            text: qsTr("Colors")
            Layout.columnSpan: 2
        }

        Label { Layout.leftMargin: 10; text: qsTr("Background") }
        Button { id: btnColorBackground; text: "#000000";
            onClicked: {
                colorDialog.setDest(btnColorBackground)
                colorDialog.open();
            }
        }

        Label { Layout.leftMargin: 10; text: qsTr("Text/Axes") }
        Button { id: btnColorText; text: "#ffffff";
            onClicked: {
                colorDialog.setDest(btnColorText)
                colorDialog.open();
            }
        }

        Label { Layout.leftMargin: 10; text: qsTr("Grid") }
        Button { id: btnColorGrid; text: "#ffffff";
            onClicked: {
                colorDialog.setDest(btnColorText)
                colorDialog.open();
            }
        }
    }

    ColorDialog {
        id: colorDialog
        showAlphaChannel: false

        property var dest

        function setDest(dest) {
            this.dest = dest;
            this.color = dest.text
        }

        onAccepted: {
            dest.text = currentColor
            console.warn("text",dest.text,currentColor)
        }
    }

    Settings {
        property alias interpolationType: interpolationType.currentIndex
        property alias colorBackground: btnColorBackground.text
        property alias colorText: btnColorText.text
        property alias colorGrid: btnColorGrid.text
    }
}

