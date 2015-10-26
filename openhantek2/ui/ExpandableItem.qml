import QtQuick 2.4
import QtQuick.Controls 1.3
import QtQuick.Layouts 1.1
import QtQuick.Controls.Styles 1.4

ColumnLayout {
    id: item
    anchors.left: parent.left
    anchors.right: parent.right

    property alias title: btnHeader.text
    property string collapseImage: "qrc:/collapse.png"
    property string expandImage: "qrc:/expand.png"
    property bool contentVisible: true
    property bool hasSettings: false
    property alias btnPreferences: btnPreferences

    onContentVisibleChanged: {
        var a = item.children;
        for (var c in a) {
            if (item.children[c]===btnRow) continue;
            item.children[c].visible = contentVisible;
        }
    }

    Component.onCompleted: {
        var a = item.children;
        for (var c in a) {
            if (item.children[c]===btnRow) continue;
            item.children[c].visible = contentVisible;
        }
    }

//    states: [
//        State {
//         name: "show"
//         when: contentVisible
//         PropertyChanges {
//             target: screen
//             x: 0
//             opacity:1
//         }
//        },
//        State {
//         name: "hide"
//         when: !contentVisible
//         PropertyChanges {
//             target: screen
//             opacity:0
//         }
//        }
//    ]

//    transitions: [
//        Transition {
//         from:"hide"
//         to:"show"
//         NumberAnimation { properties: "x"; duration:500}
//         NumberAnimation { properties: "opacity"; duration: 700 }
//        },
//        Transition {
//         //from: "show"
//         to: "hide"
//         NumberAnimation { properties: "x"; duration:500}
//         NumberAnimation { properties: "opacity"; duration: 700 }
//        }
//    ]

    RowLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        id: btnRow

        Button {
            id: btnHeader
            text: "default header"
            Layout.fillWidth: true
            style: ButtonStyle {
                label: Component {
                    Row {
                        spacing: 5
                        Image {
                            id: image
                            source: btnHeader.iconSource
                        }
                        Text {
                            text: btnHeader.text
                            clip: true
                            wrapMode: Text.WordWrap
                            verticalAlignment: Text.AlignVCenter
                            horizontalAlignment: Text.AlignLeft
                        }
                    }
                }
            }

            iconSource: contentVisible ? collapseImage : expandImage
            onClicked: { contentVisible = !contentVisible }
        }

        Button {
            id: btnPreferences
            iconSource: "qrc:preferences.png"
            visible: item.hasSettings
            height: btnHeader.height
        }
    }
}

