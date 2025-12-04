#include <QApplication>

#include "Controller/AppController.h"
#include "View/AppMainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName(QStringLiteral("LucaFamilyTools"));
    QCoreApplication::setApplicationName(QStringLiteral("TranscriptEditor"));

    Controller::AppController controller;

    View::AppMainWindow mainWindow;
    mainWindow.setController(&controller);

    mainWindow.show();

    // Optional: if you want to pre-set a root dir on startup, you can do:
    //controller.setRootDirectory(QStringLiteral("C:/Users/lucaa/Desktop/Transcripts"));
    //QString err;
    //controller.loadTranscripts(&err);

    return app.exec();
}
