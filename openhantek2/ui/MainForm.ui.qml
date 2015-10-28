import QtQuick 2.4
import QtQuick.Controls 1.3
import QtQuick.Layouts 1.1
import Qt.labs.settings 1.0

Item {
    id: page
    width: 640
    height: 480
    Layout.minimumWidth: 500
    Layout.minimumHeight: 300
    anchors.margins: 10

    property int label_width: 100

    Flickable {
        id: flick
        //clip: true;
        contentWidth: width;
        contentHeight: allControls.height
        Layout.minimumWidth: 200
        Layout.maximumWidth: 400
        width: 250
        anchors {
            left: parent.left
            top: parent.top
            bottom: parent.bottom
        }

        ColumnLayout {
        id: allControls
        anchors {
            left: parent.left
            right: parent.right
        }

        ExpandableItem {
            id: groupHorizontal
            title: qsTr("Horizontal")
            Layout.fillWidth: true;

            Grid {
                Layout.fillWidth: true;
                columns: 2
                flow: GridLayout.LeftToRight
                columnSpacing: 5
                rowSpacing: 5

                Label { text: qsTr("Time/Div"); elide: Text.ElideRight; width: label_width }
                ComboBox { id: comboBox1; Layout.fillWidth: true; }

                Label { text: qsTr("Format"); elide: Text.ElideRight; width: label_width }
                ComboBox { id: comboBox2; Layout.fillWidth: true; }

                Label { text: qsTr("Frequencybase"); elide: Text.ElideRight; width: label_width }
                ComboBox { id: comboBox3; Layout.fillWidth: true; }
            }
        }

        ExpandableItem {
            id: groupAdvanced
            contentVisible: false
            Layout.fillWidth: true;
            title: qsTr("Advanced")

            Grid {
                Layout.fillWidth: true;
                columns: 2
                flow: GridLayout.LeftToRight
                columnSpacing: 5
                rowSpacing: 5

                Label { text: qsTr("Samplerate"); elide: Text.ElideRight; width: label_width }
                ComboBox { id: comboBox1b; Layout.fillWidth: true; }

                Label { text: qsTr("Record length"); elide: Text.ElideRight; width: label_width }
                ComboBox { id: comboBox2b; Layout.fillWidth: true; }
            }
        }

        ExpandableItem {
            id: groupTrigger
            title: qsTr("Trigger")

            Grid {
                Layout.fillWidth: true;
                columns: 2
                flow: GridLayout.TopToBottom
                columnSpacing: 5
                rowSpacing: 5

                Label { text: qsTr("Mode"); elide: Text.ElideRight; width: label_width }
                Label { text: qsTr("Source"); elide: Text.ElideRight; width: label_width }
                Label { text: qsTr("Slope"); elide: Text.ElideRight; width: label_width }

                ComboBox { id: comboBox1a; Layout.fillWidth: true; }
                ComboBox { id: comboBox2a; Layout.fillWidth: true; }
                ComboBox { id: comboBox3a; Layout.fillWidth: true; }
            }
        }

        ExpandableItem {
            id: groupVoltage
            title: qsTr("Voltage")

            VoltageItem {
                id: voltageItem
                channelName: qsTr("Math")
            }
        }

        ExpandableItem {
            id: groupSpectrum
            contentVisible: false
            hasSettings: true
            btnPreferences.onClicked: spectrumSettingsDialog.open()
            title: qsTr("Spectrum")

            SpectrumItem {
                channelName: qsTr("Math")
            }
        }

        } // end column
    }

    Settings {
        property alias groupHorizontal: groupHorizontal.contentVisible
        property alias groupAdvanced: groupAdvanced.contentVisible
        property alias groupTrigger: groupTrigger.contentVisible
        property alias groupSpectrum: groupSpectrum.contentVisible
        property alias groupVoltage: groupVoltage.contentVisible
    }


    PlotterItem {
        anchors.leftMargin: 10
        Layout.minimumWidth: 200
        anchors {
            left: flick.right
            right: parent.right
            top: parent.top
            bottom: parent.bottom
        }
        btnPreferences.onClicked: scopeSettingsDialog.open()
    }

    ScopeSettingsDialog {
        id: scopeSettingsDialog
    }

    SpectrumSettingsDialog {
        id: spectrumSettingsDialog
    }
}
