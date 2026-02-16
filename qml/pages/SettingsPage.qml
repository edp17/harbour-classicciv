import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    allowedOrientations: Orientation.LandscapeMask | Orientation.PortraitMask

    property var windowResOptions: ["original", "desktop", "1280x720", "1920x1080"]
    property var scalerOptions: ["normal", "normal2x", "normal3x", "hq2x", "none"]

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: col.height + Theme.paddingLarge

        Column {
            id: col
            width: parent.width
            spacing: Theme.paddingLarge
            anchors.top: parent.top
            anchors.topMargin: Theme.paddingLarge

            PageHeader { title: "Settings" }

            TextField {
                x: Theme.horizontalPageMargin
                width: parent.width - 2*x
                label: "CPU cycles (auto / number)"
                text: runner.cycles

                onTextChanged: {
                    if (runner.cycles !== text)
                        runner.cycles = text
                }
            }

            ComboBox {
                x: Theme.horizontalPageMargin
                width: parent.width - 2*x
                label: "Window resolution"

                currentIndex: Math.max(0, windowResOptions.indexOf(runner.windowRes))

                menu: ContextMenu {
                    Repeater {
                        model: windowResOptions
                        MenuItem {
                            text: modelData
                        }
                    }
                }

                onCurrentIndexChanged: {
                    var v = windowResOptions[currentIndex]
                    if (runner.windowRes !== v)
                        runner.windowRes = v
                }
            }

            ComboBox {
                x: Theme.horizontalPageMargin
                width: parent.width - 2*x
                label: "Scaler"

                currentIndex: Math.max(0, scalerOptions.indexOf(runner.scaler))

                menu: ContextMenu {
                    Repeater {
                        model: scalerOptions
                        MenuItem { text: modelData }
                    }
                }

                onCurrentIndexChanged: {
                    var v = scalerOptions[currentIndex]
                    if (runner.scaler !== v)
                        runner.scaler = v
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2*x
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                text: "Tip: If the game feels too fast or too slow, try setting cycles to a fixed value (e.g. 3000–8000)."
            }
        }
    }
}
