import QtQuick

// 汎用スライダー: value (0.0-1.0) で外部制御、moved(v) で変化を通知
Column {
    id: root
    property string label:      ""
    property string leftLabel:  ""
    property string rightLabel: ""
    property string valueText:  ""
    property color  trackColor: "#4488ff"
    property real   value:      0.0   // 外部バインディング用

    signal moved(real newValue)       // ユーザー操作時に emit

    spacing: 8

    Text { text: root.label; color: "#88aacc"; font.pixelSize: 14 }

    Row {
        width: parent.width; spacing: 8

        Text {
            text: root.leftLabel; color: "#556677"; font.pixelSize: 12; width: 18
            anchors.verticalCenter: parent.verticalCenter
        }

        Item {
            width: parent.width - 18 - 18 - 16; height: 28
            anchors.verticalCenter: parent.verticalCenter

            Rectangle {
                id: track
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width; height: 5; radius: 2; color: "#333355"
                Rectangle {
                    width: handle.x + handle.width / 2
                    height: parent.height; radius: 2; color: root.trackColor
                }
            }

            Rectangle {
                id: handle
                anchors.verticalCenter: track.verticalCenter
                width: 22; height: 22; radius: 11
                color: ma.pressed ? "#ffffff" : Qt.lighter(root.trackColor, 1.5)
                border.color: root.trackColor; border.width: 2

                // ドラッグ中でないときのみ外部 value を反映
                onXChanged: { /* 位置変化はドラッグから */ }
                Component.onCompleted: x = root.value * (track.width - width)
                Connections {
                    target: root
                    function onValueChanged() {
                        if (!ma.drag.active)
                            handle.x = root.value * (track.width - handle.width)
                    }
                }

                MouseArea {
                    id: ma
                    anchors.fill: parent
                    drag.target: handle; drag.axis: Drag.XAxis
                    drag.minimumX: 0; drag.maximumX: track.width - handle.width
                    onPositionChanged: if (pressed)
                        root.moved(handle.x / (track.width - handle.width))
                }
            }
        }

        Text {
            text: root.rightLabel; color: "#556677"; font.pixelSize: 12; width: 18
            anchors.verticalCenter: parent.verticalCenter
            horizontalAlignment: Text.AlignRight
        }
    }

    Text { text: root.valueText; color: "#446688"; font.pixelSize: 12 }
}
