import QtQuick 2.0
import QtQuick.Controls 1.3
import QuickPlot 1.0
import "qrc:QuickPlot"
//import kde.Plotter 2.0

Rectangle {
    id: rectangle
    color: "black"

    property alias btnPreferences: btnPreferences

    PlotArea {
        anchors.fill: parent

        yScaleEngine: ScaleEngine {
            fixed: true
            max: 1.5
            min: -1.5
        }

        items: [
            ScrollingCurve {
                id: meter;
                numPoints: 300
                color: "red"
            }
        ]
    }

    Canvas {
        anchors.fill: parent

        contextType: "2d"

        Path {
            id: myPath
            startX: 0; startY: 100

            PathCurve { x: 75; y: 75 }
            PathCurve { x: 200; y: 150 }
            PathCurve { x: 325; y: 25 }
            PathCurve { x: 400; y: 100 }
        }

        onPaint: {
            context.strokeStyle = Qt.rgba(.4,.6,.8);
            context.path = myPath;
            context.stroke();
        }
    }

    Timer {
        id: timer;
        interval: 20;
        repeat: true;
        running: true;

        property real pos: 0

        onTriggered: {
            meter.appendDataPoint( Math.sin(pos) );
            pos += 0.05;
        }
    }

// For digital phosphor effect
//    Rectangle {
//        id: rectangle_c
//        anchors.fill: parent
//        color: 'red'
//        visible: false
//    }

//    ShaderEffect {
//        anchors.fill: parent
//        property variant source: rectangle_c
//        property real frequency: 8
//        property real amplitude: 0.1
//        property real time: 0.0
//        NumberAnimation on time {
//            from: 0; to: Math.PI*2; duration: 1000; loops: Animation.Infinite
//        }

//        fragmentShader: "
//                        uniform lowp float qt_Opacity;
//                        void main() {
//                            gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0) * qt_Opacity;
//                        }"

//        fragmentShader: "
//            varying highp vec2 qt_TexCoord0;
//            uniform sampler2D source;
//            uniform lowp float qt_Opacity;
//            uniform highp float frequency;
//            uniform highp float amplitude;
//            uniform highp float time;
//            void main() {
//                highp vec2 pulse = sin(time - frequency * qt_TexCoord0);
//                highp vec2 coord = qt_TexCoord0 + amplitude * vec2(pulse.x, -pulse.x);
//                gl_FragColor = texture2D(source, coord) * qt_Opacity;
//            }"
//    }

    Button {
        id: btnPreferences
        iconSource: "qrc:preferences.png"
        width: 32
        height: 32
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: 10
        anchors.rightMargin: 10
    }
}
