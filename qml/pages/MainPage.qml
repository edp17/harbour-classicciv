import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
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

Button {
    text: "Send 1"
    onClicked: runner.pressKey("1")
}
Button {
    text: "Enter"
    onClicked: runner.pressKey("Return")
}
        }
    }
}
