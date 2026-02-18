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

            // ----- Performance / video -----

            TextField {
                x: Theme.horizontalPageMargin
                width: parent.width - 2*x
                label: "CPU cycles (auto / number)"
                text: runner.cycles
                inputMethodHints: Qt.ImhNoPredictiveText
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
                        MenuItem { text: modelData }
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
                label: "Scaler (deprecated in staging)"
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
                text: "Tip: 'scaler' is deprecated in DOSBox Staging; prefer Window resolution / viewport / integer scaling."
            }

            // ----- Startup auto-selection -----

            SectionHeader {
                text: "Startup (auto-select initial Civ screens)"
            }

            TextSwitch {
                x: Theme.horizontalPageMargin
                width: parent.width - 2*x
                text: "Auto-select the 3 startup menus"
                checked: runner.startupAutoSelect
                onCheckedChanged: {
                    if (runner.startupAutoSelect !== checked)
                        runner.startupAutoSelect = checked
                }
                description: "If enabled, the app sends your chosen options into the game right after launch."
            }

            ComboBox {
                x: Theme.horizontalPageMargin
                width: parent.width - 2*x
                label: "Graphics mode"
                enabled: runner.startupAutoSelect
                currentIndex: Math.max(0, runner.startupGraphicsMode - 1)
                menu: ContextMenu {
                    MenuItem { text: "1) VGA (256 color)" }
                    MenuItem { text: "2) MCGA (256 color)" }
                    MenuItem { text: "3) EGA (16 color)" }
                    MenuItem { text: "4) Tandy 1000 (16 color)" }
                }
                onCurrentIndexChanged: {
                    var v = currentIndex + 1
                    if (runner.startupGraphicsMode !== v)
                        runner.startupGraphicsMode = v
                }
            }

            ComboBox {
                x: Theme.horizontalPageMargin
                width: parent.width - 2*x
                label: "Sound mode"
                enabled: runner.startupAutoSelect
                currentIndex: Math.max(0, runner.startupSoundMode - 1)
                menu: ContextMenu {
                    MenuItem { text: "1) No sounds please" }
                    MenuItem { text: "2) IBM sounds" }
                    MenuItem { text: "3) Tandy sounds" }
                    MenuItem { text: "4) AdLib/Sound Blaster" }
                    MenuItem { text: "5) Roland MT-32 MIDI board" }
                    MenuItem { text: "6) Custom sound driver" }
                }
                onCurrentIndexChanged: {
                    var v = currentIndex + 1
                    if (runner.startupSoundMode !== v)
                        runner.startupSoundMode = v
                }
            }

            ComboBox {
                x: Theme.horizontalPageMargin
                width: parent.width - 2*x
                label: "Control"
                enabled: runner.startupAutoSelect
                currentIndex: Math.max(0, runner.startupControlMode - 1)
                menu: ContextMenu {
                    MenuItem { text: "1) Mouse and Keyboard" }
                    MenuItem { text: "2) Keyboard only" }
                }
                onCurrentIndexChanged: {
                    var v = currentIndex + 1
                    if (runner.startupControlMode !== v)
                        runner.startupControlMode = v
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2*x
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                text: "Note: If you change these options, restart the game."
            }
        }
    }
}