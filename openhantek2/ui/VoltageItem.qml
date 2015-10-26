import QtQuick 2.4
import QtQuick.Controls 1.3
import QtQuick.Layouts 1.1

Grid {
    property alias channelName: checkBox.text
    id: grid
    Layout.fillWidth: true;
    rows: 2
    flow: GridLayout.LeftToRight
    columnSpacing: 5
    rowSpacing: 5

    CheckBox { id: checkBox; Layout.fillWidth: true;}
    ComboBox { id: voltage; Layout.fillWidth: true; }

    Label {text:" "; elide: Text.ElideRight; width: label_width}
    ComboBox { Layout.fillWidth: true; }
}
