import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page
    property bool showKeyboard: false
    allowedOrientations: Orientation.LandscapeMask | Orientation.PortraitMask

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: col.height + Theme.paddingLarge

        PullDownMenu {
            MenuItem {
                text: "Settings"
                onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
            }
            MenuItem {
                text: "Open Civ folder"
                onClicked: runner.openCivFolderInFileManager()
            }
            MenuItem {
                text: "Re-scan"
                onClicked: runner.rescan()
            }
        }

        Column {
            id: col
            width: parent.width
            spacing: Theme.paddingLarge
            anchors.top: parent.top
            anchors.topMargin: Theme.paddingLarge

            PageHeader { title: "Classic Civ (DOS)" }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2*x
                wrapMode: Text.Wrap
                text: runner.gameReady
                      ? "Ready. Civ files detected."
                      : "Civ files missing. Go back and copy CIV.EXE."
            }

            Button {
                x: Theme.horizontalPageMargin
                width: parent.width - 2*x
                text: "Launch"
                enabled: runner.gameReady
                onClicked: runner.launch()
            }
        }

    // ===== Keyboard Overlay (sibling of mainColumn) =====
    Item {
        id: kbOverlay
        anchors.fill: parent
        visible: page.showKeyboard
        z: 999

        // dim background
        Rectangle {
            anchors.fill: parent
            color: "black"
            opacity: 0.35
        }

        // bottom panel
        Rectangle {
            id: panel
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            color: Theme.rgba(Theme.overlayBackgroundColor, 0.95)

            // Choose a sensible height; adjust later
            height: Math.min(parent.height * 0.55, Theme.itemSizeLarge * 8)

            Column {
                id: panelCol
                anchors.fill: parent
                anchors.margins: Theme.paddingMedium
                spacing: Theme.paddingSmall

                // Hotkey row
                Row {
                    spacing: Theme.paddingSmall
                    Button { text: "Esc";   onClicked: runner.sendKey("ESCAPE") }
                    Button { text: "Enter"; onClicked: runner.sendKey("ENTER") }
                    Button { text: "Bksp";  onClicked: runner.sendKey("BACKSPACE") }
                    Button { text: "Space"; onClicked: runner.sendKey("SPACE") }
                    Button { text: "↑";     onClicked: runner.sendKey("UP") }
                    Button { text: "↓";     onClicked: runner.sendKey("DOWN") }
                    Button { text: "←";     onClicked: runner.sendKey("LEFT") }
                    Button { text: "→";     onClicked: runner.sendKey("RIGHT") }
                }

                // Text entry field (system keyboard)
                TextField {
                    id: textEntry
                    placeholderText: "Type here → sent to Civ"
                    focus: true

                    // Simple: send last typed character
                    onTextChanged: {
                        if (text.length > 0) {
                            runner.sendText(text.charAt(text.length - 1))
                        }
                    }
                }

                // Key grid
                Grid {
                    columns: 10
                    spacing: Theme.paddingSmall

                    Repeater {
                        model: ["1","2","3","4","5","6","7","8","9","0",
                                "Q","W","E","R","T","Y","U","I","O","P",
                                "A","S","D","F","G","H","J","K","L","-",
                                "Z","X","C","V","B","N","M",",",".","/"]
                        delegate: Button {
                            text: modelData
                            onClicked: runner.sendKey(modelData)
                        }
                    }
                }

                Button {
                    text: "Hide keyboard"
                    onClicked: page.showKeyboard = false
                }
            }
        }

        // tap outside panel closes overlay (optional)
        MouseArea {
            anchors.fill: parent
            enabled: true
            propagateComposedEvents: true
            onClicked: {
                // Only close if click was above the panel
                if (mouse.y < panel.y)
                    page.showKeyboard = false
            }
        }
    }
}
}
