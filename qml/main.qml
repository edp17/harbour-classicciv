import QtQuick 2.0
import Sailfish.Silica 1.0

ApplicationWindow {
    initialPage: runner.gameReady ? mainPage : firstRunPage
    cover: Qt.resolvedUrl("cover/CoverPage.qml")

    Component { id: firstRunPage; FirstRunPage {} }
    Component { id: mainPage; MainPage {} }
}
