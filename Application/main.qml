import QtQuick 2.15
import QtQuick.Controls 2.15

ApplicationWindow {
    width: 800
    height: 600
    color: "gray"
    visible: true
    title: "X11-Window-Application"

    property bool grow: true

    Text {
        id: textid
        anchors.centerIn: parent
        text: "Hello world"
        color: grow ? "white" : "black"
        font.pixelSize: grow ? 100 : 12
        Behavior on font.pixelSize {
            NumberAnimation {
                duration: 1000
                easing.type: Easing.InOutQuad
            }
        }
        Behavior on color {
            ColorAnimation {
                duration: 1000
            }
        }
    }

    Timer {
        id: animationTimer
        interval: 750
        running: true
        repeat: true
        onTriggered: {
            grow = !grow
        }
    }
}