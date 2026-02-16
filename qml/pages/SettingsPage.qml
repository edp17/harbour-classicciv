import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
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

            PageHeader { title: "Settings" }

            TextField {
                x: Theme.horizontalPageMargin
                width: parent.width - 2*x
                label: "CPU cycles (auto / number)"
                text: runner.cycles
                onTextChanged: runner.cycles = text
            }

            ComboBox {
                x: Theme.horizontalPageMargin
                width: parent.width - 2*x
                label: "Scaler"
                menu: ContextMenu {
                    MenuItem { text: "normal";    onClicked: runner.scaler = "normal" }
                    MenuItem { text: "normal2x";  onClicked: runner.scaler = "normal2x" }
                    MenuItem { text: "advmame2x"; onClicked: runner.scaler = "advmame2x" }
                }
                // show current choice
                description: runner.scaler
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
