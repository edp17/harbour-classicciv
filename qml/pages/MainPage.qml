import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page

    // Keyboard visibility toggle
    property bool showKeyboard: false

    // Sizing tuned for small screens
    property int keyH: Theme.itemSizeSmall * 0.85
    property int keyW: Theme.itemSizeSmall * 0.55 //0.9
    property int keyWide: Math.round(keyW * 1.05)  //1.9
    property int keyWider: Math.round(keyW * 1.05) //2.4

    // Helper: send a key name to DOSBox via your control socket
    function k(name) { runner.sendKey(name) }

    SilicaFlickable {
        id: flick
        anchors.fill: parent
        contentHeight: contentCol.height + (page.showKeyboard ? keyboardOverlay.height : 0)

        VerticalScrollDecorator {}

        PullDownMenu {
            MenuItem {
                text: page.showKeyboard ? qsTr("Hide keyboard") : qsTr("Show keyboard")
                onClicked: page.showKeyboard = !page.showKeyboard
            }
            MenuItem {
                text: qsTr("Launch")
                onClicked: runner.launch()
            }
            MenuItem {
                text: qsTr("Stop")
                // If you don't have runner.stop(), remove this MenuItem.
                onClicked: runner.stop()
            }
            MenuItem {
                text: "Settings"
                onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
            }
        }

        Column {
            id: contentCol
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: qsTr("Classic Civ")
            }

            // Top controls area
            Column {
                width: parent.width
                spacing: Theme.paddingMedium

                Row {
                    width: parent.width
                    spacing: Theme.paddingMedium

                    Button {
                        text: qsTr("Launch")
                        onClicked: runner.launch()
                    }

                    Button {
                        text: page.showKeyboard ? qsTr("Hide keyboard") : qsTr("Show keyboard")
                        onClicked: page.showKeyboard = !page.showKeyboard
                    }
                }

                // Hotkey bar (horizontal scroll)
                Flickable {
                    width: parent.width
                    height: Theme.itemSizeSmall * 2
                    contentWidth: hotkeysRow.implicitWidth
                    contentHeight: hotkeysRow.implicitHeight
                    clip: true
                    flickableDirection: Flickable.HorizontalFlick
                    boundsBehavior: Flickable.StopAtBounds

                    Row {
                        id: hotkeysRow
                        spacing: Theme.paddingSmall

                        Button { width: page.keyWide; height: page.keyH; text: "⎋";   onClicked: page.k("ESC") }
                        Button { width: page.keyWide; height: page.keyH; text: "⏎"; onClicked: page.k("ENTER") }
                        Button { width: page.keyWide; height: page.keyH; text: "␠"; onClicked: page.k("SPACE") }
                        Button { width: page.keyWide; height: page.keyH; text: "⇥";   onClicked: page.k("TAB") }
                        Button { width: page.keyWide; height: page.keyH; text: "⌫";  onClicked: page.k("BACKSPACE") }

                        Button { width: page.keyWide; height: page.keyH; text: "↑";    onClicked: page.k("UP") }
                        Button { width: page.keyWide; height: page.keyH; text: "↓";  onClicked: page.k("DOWN") }
                        Button { width: page.keyWide; height: page.keyH; text: "←";  onClicked: page.k("LEFT") }
                        Button { width: page.keyWide; height: page.keyH; text: "→"; onClicked: page.k("RIGHT") }

                        Button { width: page.keyWide; height: page.keyH; text: "F1";    onClicked: page.k("F1") }
                        Button { width: page.keyWide; height: page.keyH; text: "F2";    onClicked: page.k("F2") }
                        Button { width: page.keyWide; height: page.keyH; text: "F3";    onClicked: page.k("F3") }
                        Button { width: page.keyWide; height: page.keyH; text: "F4";    onClicked: page.k("F4") }
                    }
                }

                // Optional: send a short text (if you implemented runner.sendText)
                // Comment this whole block out if you don't have sendText().
                /*
                Row {
                    width: parent.width
                    spacing: Theme.paddingMedium

                    TextField {
                        id: textEntry
                        width: parent.width - sendBtn.width - Theme.paddingMedium
                        placeholderText: qsTr("Send text to game (optional)")
                        focus: false           // avoid system keyboard
                        inputMethodHints: Qt.ImhNoPredictiveText
                    }

                    Button {
                        id: sendBtn
                        text: qsTr("Send")
                        onClicked: {
                            if (textEntry.text.length > 0) {
                                runner.sendText(textEntry.text)
                                textEntry.text = ""
                            }
                        }
                    }
                }
                */
            }

            // Spacer so the content above doesn't sit under the keyboard overlay
            Item {
                width: 1
                height: page.showKeyboard ? keyboardOverlay.height + Theme.paddingLarge : Theme.paddingLarge
            }
        }
    }

    // --- Keyboard overlay at bottom ---
    Item {
        id: keyboardOverlay
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        visible: page.showKeyboard
        height: visible ? (keyboardFlick.height + Theme.paddingMedium * 2) : 0
        clip: true

        Rectangle {
            anchors.fill: parent
            color: "#20232a" //Theme.rgba(Theme.primaryColor, 0.10) //Theme.highlightBackgroundColor, 0.18)
            border.width: 1
            border.color: Theme.rgba(Theme.primaryColor, 0.18)
        }

        // Horizontal scroll for all keyboard rows
        Flickable {
            id: keyboardFlick
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: Theme.paddingMedium

            height: keyboardCol.implicitHeight
            contentWidth: keyboardCol.implicitWidth
            contentHeight: keyboardCol.implicitHeight
            clip: true

            flickableDirection: Flickable.HorizontalFlick
            boundsBehavior: Flickable.StopAtBounds

            Column {
                id: keyboardCol
                spacing: Theme.paddingSmall

                // Row 1
                Row {
                    spacing: Theme.paddingSmall
                    Button { width: page.keyW; height: page.keyH; text: "1"; onClicked: page.k("1") }
                    Button { width: page.keyW; height: page.keyH; text: "2"; onClicked: page.k("2") }
                    Button { width: page.keyW; height: page.keyH; text: "3"; onClicked: page.k("3") }
                    Button { width: page.keyW; height: page.keyH; text: "4"; onClicked: page.k("4") }
                    Button { width: page.keyW; height: page.keyH; text: "5"; onClicked: page.k("5") }
                    Button { width: page.keyW; height: page.keyH; text: "6"; onClicked: page.k("6") }
                    Button { width: page.keyW; height: page.keyH; text: "7"; onClicked: page.k("7") }
                    Button { width: page.keyW; height: page.keyH; text: "8"; onClicked: page.k("8") }
                    Button { width: page.keyW; height: page.keyH; text: "9"; onClicked: page.k("9") }
                    Button { width: page.keyW; height: page.keyH; text: "0"; onClicked: page.k("0") }
                    Button { width: page.keyWider; height: page.keyH; text: "⌫"; onClicked: page.k("BACKSPACE") }
                }

                // Row 2
                Row {
                    spacing: Theme.paddingSmall
                    Button { width: page.keyW; height: page.keyH; text: "Q"; onClicked: page.k("Q") }
                    Button { width: page.keyW; height: page.keyH; text: "W"; onClicked: page.k("W") }
                    Button { width: page.keyW; height: page.keyH; text: "E"; onClicked: page.k("E") }
                    Button { width: page.keyW; height: page.keyH; text: "R"; onClicked: page.k("R") }
                    Button { width: page.keyW; height: page.keyH; text: "T"; onClicked: page.k("T") }
                    Button { width: page.keyW; height: page.keyH; text: "Y"; onClicked: page.k("Y") }
                    Button { width: page.keyW; height: page.keyH; text: "U"; onClicked: page.k("U") }
                    Button { width: page.keyW; height: page.keyH; text: "I"; onClicked: page.k("I") }
                    Button { width: page.keyW; height: page.keyH; text: "O"; onClicked: page.k("O") }
                    Button { width: page.keyW; height: page.keyH; text: "P"; onClicked: page.k("P") }
                }

                // Row 3
                Row {
                    spacing: Theme.paddingSmall
                    Button { width: page.keyWider; height: page.keyH; text: "⇥"; onClicked: page.k("TAB") }
                    Button { width: page.keyW; height: page.keyH; text: "A"; onClicked: page.k("A") }
                    Button { width: page.keyW; height: page.keyH; text: "S"; onClicked: page.k("S") }
                    Button { width: page.keyW; height: page.keyH; text: "D"; onClicked: page.k("D") }
                    Button { width: page.keyW; height: page.keyH; text: "F"; onClicked: page.k("F") }
                    Button { width: page.keyW; height: page.keyH; text: "G"; onClicked: page.k("G") }
                    Button { width: page.keyW; height: page.keyH; text: "H"; onClicked: page.k("H") }
                    Button { width: page.keyW; height: page.keyH; text: "J"; onClicked: page.k("J") }
                    Button { width: page.keyW; height: page.keyH; text: "K"; onClicked: page.k("K") }
                    Button { width: page.keyW; height: page.keyH; text: "L"; onClicked: page.k("L") }
                    Button { width: page.keyWide; height: page.keyH; text: "⏎"; onClicked: page.k("ENTER") }
                }

                // Row 4
                Row {
                    spacing: Theme.paddingSmall
                    Button { width: page.keyWider; height: page.keyH; text: "⇧"; onClicked: page.k("LSHIFT") }
                    Button { width: page.keyW; height: page.keyH; text: "Z"; onClicked: page.k("Z") }
                    Button { width: page.keyW; height: page.keyH; text: "X"; onClicked: page.k("X") }
                    Button { width: page.keyW; height: page.keyH; text: "C"; onClicked: page.k("C") }
                    Button { width: page.keyW; height: page.keyH; text: "V"; onClicked: page.k("V") }
                    Button { width: page.keyW; height: page.keyH; text: "B"; onClicked: page.k("B") }
                    Button { width: page.keyW; height: page.keyH; text: "N"; onClicked: page.k("N") }
                    Button { width: page.keyW; height: page.keyH; text: "M"; onClicked: page.k("M") }
                    Button { width: page.keyWider * 2; height: page.keyH; text: "Esc"; onClicked: page.k("ESC") }
                }

                // Row 5 (space + arrows)
                Row {
                    spacing: Theme.paddingSmall
                    Button { width: page.keyWider * 2; height: page.keyH; text: "Ctrl"; onClicked: page.k("LCTRL") }
                    Button { width: page.keyWider * 2.1; height: page.keyH; text: "Space"; onClicked: page.k("SPACE") }
                    Button { width: page.keyWider * 2; height: page.keyH; text: "Alt"; onClicked: page.k("LALT") }

                    Button { width: page.keyWide; height: page.keyH; text: "←"; onClicked: page.k("LEFT") }
                    Button { width: page.keyWide; height: page.keyH; text: "↑"; onClicked: page.k("UP") }
                    Button { width: page.keyWide; height: page.keyH; text: "↓"; onClicked: page.k("DOWN") }
                    Button { width: page.keyWide; height: page.keyH; text: "→"; onClicked: page.k("RIGHT") }
                }
            }
        }
    }
}
