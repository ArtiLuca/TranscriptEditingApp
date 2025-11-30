// #include <QCoreApplication>
// #include <QDebug>

// #include "Controller/AppController.h"
// #include "Model/Data/Transcript.h"

// using Controller::AppController;
// using Model::Data::Transcript;

// int main(int argc, char *argv[])
// {
//     QCoreApplication app(argc, argv);

//     // 1. Create controller and set the root directory
//     AppController controller;
//     const QString rootDir = QStringLiteral("C:/Users/lucaa/Desktop/Transcripts");
//     controller.setRootDirectory(rootDir);

//     // 2. Load transcripts
//     QString error;
//     if (!controller.loadTranscripts(&error)) {
//         qWarning().noquote() << "loadTranscripts failed:" << error;
//         return 1;
//     }

//     qDebug().noquote()
//         << "Root directory:" << rootDir
//         << "\nTranscripts loaded:" << controller.transcriptCount();

//     if (controller.transcriptCount() == 0) {
//         qWarning() << "No transcripts loaded.";
//         return 0;
//     }

//     // 3. Print titles
//     const QStringList titles = controller.transcriptTitles();
//     qDebug().noquote() << "\n=== Transcript Titles ===";
//     for (int i = 0; i < titles.size(); ++i) {
//         qDebug().noquote() << " [" << i << "]" << titles.at(i);
//     }

//     // 4. Select the first transcript
//     controller.selectTranscript(0);
//     Transcript* t = controller.currentTranscript();
//     if (!t) {
//         qWarning() << "currentTranscript() is null after selectTranscript(0).";
//         return 1;
//     }

//     qDebug().noquote()
//         << "\n=== Current Transcript ==="
//         << "\nIndex:" << controller.currentTranscriptIndex()
//         << "\nTitle:" << t->title
//         << "\nFolder:" << t->folderPath
//         << "\nSegments:" << t->segmentCount();

//     // 5. List speakers
//     QStringList speakers = controller.currentTranscriptSpeakers();
//     qDebug().noquote() << "\nSpeakers:";
//     for (const QString& sp : speakers) {
//         qDebug().noquote() << " - " << sp;
//     }

//     if (t->segmentCount() == 0) {
//         qWarning() << "Transcript has no segments.";
//         return 0;
//     }

//     // 6. Show first segment before editing
//     qDebug().noquote()
//         << "\n--- Original Segment 0 ---"
//         << "\nSpeaker:" << t->segments[0].speakerID
//         << "\nText:\n"  << t->segments[0].text << "\n";

//     // 7. Run a simple search via AppController helpers
//     //    (change the word to something that actually appears)
//     const QString searchPattern = QStringLiteral("vacation");
//     QStringList speakerFilter; // empty = all speakers

//     QVector<int> matches = controller.searchSegments(
//         searchPattern,
//         speakerFilter,
//         Qt::CaseInsensitive
//         );

//     qDebug().noquote()
//         << "\nSearch for pattern:" << searchPattern
//         << "\nMatches count:" << matches.size();

//     if (!matches.isEmpty()) {
//         qDebug().noquote() << "First few match indices:";
//         for (int i = 0; i < matches.size() && i < 5; ++i) {
//             qDebug().noquote() << " -" << matches[i];
//         }
//     }

//     // 8. Small edit via the editor (through AppController)
//     auto* ed = controller.editor();
//     if (!ed) {
//         qWarning() << "editor() returned nullptr.";
//         return 1;
//     }

//     ed->appendToSegment(0, QStringLiteral("\n[NOTE: edited via AppController test]"));

//     qDebug().noquote()
//         << "\n--- After appendToSegment(0) via editor() ---"
//         << "\nText:\n" << t->segments[0].text << "\n";

//     // 9. Save the current transcript (editable + meta.json, no reference rewrite)
//     controller.requestSaveCurrent(/*exportReference=*/false);

//     qDebug().noquote()
//         << "\nTest complete. Check editable.txt and meta.json in folder:\n"
//         << t->folderPath;

//     return 0;
// }


#include <QApplication>

#include "Controller/AppController.h"
#include "View/AppMainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 1. Create controller
    Controller::AppController controller;

    // 2. Create main window and attach controller
    View::AppMainWindow mainWindow;
    mainWindow.setController(&controller);

    mainWindow.show();

    // Optional: if you want to pre-set a root dir on startup, you can do:
    controller.setRootDirectory(QStringLiteral("C:/Users/lucaa/Desktop/Transcripts"));
    QString err;
    controller.loadTranscripts(&err);

    return app.exec();
}
