import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page
    allowedOrientations: Orientation.LandscapeMask | Orientation.PortraitMask

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: col.height + Theme.paddingLarge

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
                text: "This app requires your original Civilization (DOS) game files.\n\nCopy your Civ folder contents so that CIV.EXE exists here:"
            }

            TextField {
                x: Theme.horizontalPageMargin
                width: parent.width - 2*x
                text: runner.civDir
                readOnly: true
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2*x
                wrapMode: Text.Wrap
                text: runner.gameReady
                      ? "✓ CIV.EXE found. You can launch the game."
                      : "✗ CIV.EXE not found yet."
            }

            Row {
                x: Theme.horizontalPageMargin
                spacing: Theme.paddingMedium

                Button {
                    text: "Open folder"
                    onClicked: runner.openCivFolderInFileManager()
                }

                Button {
                    text: "Re-scan"
                    onClicked: runner.rescan()
                }
            }

            Button {
                x: Theme.horizontalPageMargin
                width: parent.width - 2*x
                text: "Launch"
                enabled: runner.gameReady
                onClicked: {
                    if (runner.launch()) {
                        pageStack.replace(Qt.resolvedUrl("MainPage.qml"))
                    }
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2*x
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                text: "Touch controls:\n• Tap = left click\n• Long-press = right click\n• Drag = move cursor\n• Two-finger tap = Enter\n• Two-finger long-press = Esc"
            }
        }
    }
}
