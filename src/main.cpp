#include <sailfishapp.h>
#include <QQmlContext>
#include <QGuiApplication>
#include <QQuickView>
#include "DosboxRunner.h"

int main(int argc, char *argv[])
{
    QGuiApplication *app = SailfishApp::application(argc, argv);
    QQuickView *view = SailfishApp::createView();

    DosboxRunner runner;
    view->rootContext()->setContextProperty("runner", &runner);

    view->setSource(SailfishApp::pathTo("qml/main.qml"));
    view->show();

    return app->exec();
}
