#include "TranscriptManager.h"

#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace Model {
namespace Service {

using Model::Data::Transcript;

TranscriptManager::TranscriptManager(const QString& dir)
    : rootDir(dir),
    importer(dir)
{}

void TranscriptManager::setRootDirectory(const QString& dir) {

    rootDir = dir;
    // The importer may later use this for copying, etc.
    importer = TranscriptImporter(dir);
}

QString TranscriptManager::rootDirectory() const {

    return rootDir;
}

bool TranscriptManager::loadAllFromRoot(QString* errorMessage)
{
    transcriptList.clear();

    if (rootDir.isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Root directory is not set.");
        return false;
    }

    QDir root(rootDir);
    if (!root.exists()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Root directory does not exist: %1").arg(rootDir);
        return false;
    }

    QStringList subDirs = root.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    bool anyLoaded = false;

    for (const QString& subName : subDirs) {
        QDir subDir(root.filePath(subName));
        QString metaPath = subDir.filePath(QStringLiteral("meta.json"));

        QFileInfo metaInfo(metaPath);
        if (!metaInfo.exists())
            continue; // Not a transcript folder (no meta.json), skip.

        // Load meta.json to get speaker list
        QFile metaFile(metaPath);
        if (!metaFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            // If one folder fails, record the first error but keep trying others
            if (errorMessage && errorMessage->isEmpty())
                *errorMessage = QStringLiteral("Cannot open meta.json: %1").arg(metaPath);
            continue;
        }

        const QByteArray rawMeta = metaFile.readAll();
        QJsonParseError parseErr;
        QJsonDocument doc = QJsonDocument::fromJson(rawMeta, &parseErr);
        if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
            if (errorMessage && errorMessage->isEmpty())
                *errorMessage = QStringLiteral("Error parsing meta.json at %1: %2")
                                    .arg(metaPath, parseErr.errorString());
            continue;
        }

        QJsonObject metaObj = doc.object();
        QJsonArray speakersArray = metaObj.value(QStringLiteral("speakers")).toArray();
        if (speakersArray.isEmpty()) {
            // If no speakers in meta, we skip this transcript
            continue;
        }

        QStringList speakerNames;
        for (const QJsonValue& v : speakersArray) {
            if (v.isString())
                speakerNames << v.toString().trimmed();
        }

        if (speakerNames.isEmpty())
            continue;

        // Use TranscriptImporter to fully import and parse this transcript
        Transcript transcript;
        QString localError;
        if (!importer.importFromFolder(subDir.absolutePath(), speakerNames,
                                         transcript, &localError)) {
            if (errorMessage && errorMessage->isEmpty())
                *errorMessage = QStringLiteral("Failed to import %1: %2")
                                    .arg(subDir.absolutePath(), localError);
            continue;
        }

        transcriptList.push_back(transcript);
        anyLoaded = true;
    }

    // It's not an error if root exists but contains no valid transcripts.
    if (!anyLoaded && errorMessage && errorMessage->isEmpty()) {
        *errorMessage = QStringLiteral("No transcripts found in root directory: %1").arg(rootDir);
    }

    return true;
}


bool TranscriptManager::importTranscriptFromFolder(
    const QString& folderPath,
    const QStringList& speakerNames,
    int* outIndex,
    QString* errorMessage) {

    Transcript transcript;
    if (!importer.importFromFolder(folderPath, speakerNames, transcript, errorMessage)) {
        return false;
    }

    transcriptList.push_back(transcript);
    if (outIndex) {
        *outIndex = transcriptList.size() - 1;
    }
    return true;
}




void TranscriptManager::clear() {

    transcriptList.clear();
}

int TranscriptManager::transcriptCount() const {

    return transcriptList.size();
}


const QVector<Transcript>& TranscriptManager::transcripts() const {

    return transcriptList;
}

const Transcript* TranscriptManager::transcriptAt(int index) const {

    if (index < 0 || index >= transcriptList.size())
        return nullptr;
    return &transcriptList[index];
}

Transcript* TranscriptManager::transcriptAt(int index) {

    if (index < 0 || index >= transcriptList.size())
        return nullptr;
    return &transcriptList[index];
}


int TranscriptManager::indexOfTranscriptByID(const QString& id) const {

    for (int i = 0; i < transcriptList.size(); ++i) {
        if (transcriptList[i].id == id)
            return i;
    }
    return -1;
}


}
}
