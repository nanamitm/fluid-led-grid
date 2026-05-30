import QtQuick
import QtQuick.Window

Window {
    id: root
    width: 440
    height: 520
    minimumWidth: 360
    minimumHeight: 420
    visible: true
    title: "Fluid LED Grid"
    color: "#0a0a0a"

    // ---- グリッド ----
    FluidGrid {
        anchors {
            top: parent.top; left: parent.left; right: parent.right
            bottom: bottomBar.top; margins: 8
        }
    }

    // ---- 下部バー (設定ボタンのみ) ----
    Rectangle {
        id: bottomBar
        anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
        height: 44; color: "#111820"; z: 10

        Rectangle {
            anchors { right: parent.right; verticalCenter: parent.verticalCenter; rightMargin: 8 }
            width: 36; height: 36; radius: 18
            color: gearMouse.pressed ? "#223" : "transparent"
            Text { anchors.centerIn: parent; text: "⚙"; color: "#88aacc"; font.pixelSize: 22 }
            MouseArea {
                id: gearMouse; anchors.fill: parent
                onClicked: settingsPanel.open = !settingsPanel.open
            }
        }
    }

    // ---- 設定パネル ----
    Rectangle {
        id: settingsPanel
        property bool open: false

        width: 270
        anchors { top: parent.top; bottom: bottomBar.top }
        x: open ? parent.width - width : parent.width
        z: 20; color: "#111827"
        border.color: "#2a3a4a"; border.width: 1
        clip: true

        Behavior on x { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }

        Flickable {
            anchors.fill: parent
            contentHeight: col.implicitHeight + 40
            clip: true

            Column {
                id: col
                anchors { top: parent.top; left: parent.left; right: parent.right; margins: 20 }
                spacing: 20

                // タイトル行
                Row {
                    width: parent.width
                    Text { text: "設定"; color: "#aaccff"; font.pixelSize: 18; font.bold: true
                           width: parent.width - 32 }
                    Rectangle {
                        width: 28; height: 28; radius: 14
                        color: closeMouse.pressed ? "#334" : "transparent"
                        Text { anchors.centerIn: parent; text: "✕"; color: "#88aacc"; font.pixelSize: 16 }
                        MouseArea { id: closeMouse; anchors.fill: parent
                                    onClicked: settingsPanel.open = false }
                    }
                }

                Rectangle { width: parent.width; height: 1; color: "#2a3a4a" }

                // センサー感度 (0.2x〜3.0x を 0〜1 に正規化)
                SettingSlider {
                    width: parent.width
                    label: "センサー感度"; leftLabel: "低"; rightLabel: "高"
                    valueText: FluidSim.sensitivity.toFixed(1) + "x"
                    trackColor: "#44aaff"
                    value: (FluidSim.sensitivity - 0.2) / 2.8
                    onMoved: (v) => FluidSim.sensitivity = 0.2 + v * 2.8
                }

                Rectangle { width: parent.width; height: 1; color: "#2a3a4a" }

                // 粘度
                SettingSlider {
                    width: parent.width
                    label: "粘度"; leftLabel: "水"; rightLabel: "蜜"
                    valueText: Math.round(FluidSim.viscosity * 100) + "%"
                    trackColor: "#4488ff"
                    value: FluidSim.viscosity
                    onMoved: (v) => FluidSim.viscosity = v
                }

                Rectangle { width: parent.width; height: 1; color: "#2a3a4a" }

                // カラー
                Column {
                    width: parent.width; spacing: 10

                    Text { text: "カラー"; color: "#88aacc"; font.pixelSize: 14 }

                    // プリセット色ボタン
                    Row {
                        spacing: 8; anchors.horizontalCenter: parent.horizontalCenter

                        Repeater {
                            model: ListModel {
                                ListElement { clabel: "水"; chue: 200; cval: "#0088ff" }
                                ListElement { clabel: "緑"; chue: 120; cval: "#00cc44" }
                                ListElement { clabel: "赤"; chue:   0; cval: "#ff3322" }
                                ListElement { clabel: "紫"; chue: 270; cval: "#aa44ff" }
                                ListElement { clabel: "橙"; chue:  30; cval: "#ff8800" }
                            }
                            delegate: Rectangle {
                                width: 42; height: 42; radius: 8
                                color: cval
                                opacity: FluidSim.colorHue === chue ? 1.0 : 0.4
                                border.color: FluidSim.colorHue === chue ? "#fff" : "transparent"
                                border.width: 2
                                Text { anchors.centerIn: parent; text: clabel
                                       color: "white"; font.pixelSize: 12 }
                                MouseArea { anchors.fill: parent
                                            onClicked: FluidSim.colorHue = chue }
                            }
                        }
                    }

                    // 色相スライダー
                    Item {
                        width: parent.width; height: 32

                        Rectangle {
                            id: hueTrack
                            anchors.verticalCenter: parent.verticalCenter
                            width: parent.width; height: 8; radius: 4
                            gradient: Gradient {
                                orientation: Gradient.Horizontal
                                GradientStop { position: 0.000; color: "#ff0000" }
                                GradientStop { position: 0.167; color: "#ffff00" }
                                GradientStop { position: 0.333; color: "#00ff00" }
                                GradientStop { position: 0.500; color: "#00ffff" }
                                GradientStop { position: 0.667; color: "#0000ff" }
                                GradientStop { position: 0.833; color: "#ff00ff" }
                                GradientStop { position: 1.000; color: "#ff0000" }
                            }
                        }
                        Rectangle {
                            id: hueHandle
                            anchors.verticalCenter: hueTrack.verticalCenter
                            width: 22; height: 22; radius: 11
                            color: Qt.hsva(FluidSim.colorHue / 360, 0.9, 1.0, 1.0)
                            border.color: "white"; border.width: 2

                            Component.onCompleted: x = (FluidSim.colorHue / 359) * (hueTrack.width - width)
                            Connections {
                                target: FluidSim
                                function onColorHueChanged(h) {
                                    if (!hueMa.drag.active)
                                        hueHandle.x = (h / 359) * (hueTrack.width - hueHandle.width)
                                }
                            }

                            MouseArea {
                                id: hueMa; anchors.fill: parent
                                drag.target: hueHandle; drag.axis: Drag.XAxis
                                drag.minimumX: 0; drag.maximumX: hueTrack.width - hueHandle.width
                                onPositionChanged: if (pressed)
                                    FluidSim.colorHue = Math.round(
                                        hueHandle.x / (hueTrack.width - hueHandle.width) * 359)
                            }
                        }
                    }
                }

                Rectangle { width: parent.width; height: 1; color: "#2a3a4a" }

                // リセット
                Column {
                    width: parent.width; spacing: 8
                    Text { text: "リセット"; color: "#88aacc"; font.pixelSize: 14 }
                    Text { text: "粒子を下部に再配置します"
                           color: "#445566"; font.pixelSize: 11
                           width: parent.width; wrapMode: Text.Wrap }
                    Rectangle {
                        width: parent.width; height: 40; radius: 8
                        color: resetMouse.pressed ? "#1a3a5a" : "#162030"
                        border.color: "#2a4a6a"; border.width: 1
                        Text { anchors.centerIn: parent; text: "↺  リセット"
                               color: "#88bbff"; font.pixelSize: 14 }
                        MouseArea {
                            id: resetMouse; anchors.fill: parent
                            onClicked: { FluidSim.reset(); settingsPanel.open = false }
                        }
                    }
                }

                Item { width: 1; height: 8 }
            }
        }
    }

    // パネル外タップで閉じる
    MouseArea {
        anchors { top: parent.top; left: parent.left; bottom: bottomBar.top }
        width: parent.width - settingsPanel.width
        enabled: settingsPanel.open; z: 15
        onClicked: settingsPanel.open = false
    }
}
