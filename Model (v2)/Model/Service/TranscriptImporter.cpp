#include "TranscriptImporter.h"
#include "Model/Service/TranscriptParser.h"


#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QCryptographicHash>

namespace Model {
namespace Service {

using Model::Data::Transcript;
using Model::Data::Segment;
using Model::Data::Speaker;

TranscriptImporter::TranscriptImporter(const QString& rootDir)
    : rootDirPath(rootDir)
{}

bool TranscriptImporter::importFromFolder(
    const QString& folderPath,
    const QStringList& speakerNames,
    Transcript& outTranscript,
    QString* errorMessage) const {

    if (speakerNames.isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("No speaker names provided.");
        return false;
    }

    QDir dir(folderPath);
    if (!dir.exists()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Folder does not exist: %1").arg(folderPath);
        return false;
    }

    // Clear and initialize basic metadata
    outTranscript.clear();
    outTranscript.folderPath = dir.absolutePath();
    outTranscript.title = dir.dirName();

    // --- Locate files inside the folder ---

    const QString refFileName = findReferenceTextFile(dir);
    if (refFileName.isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("No reference .txt file found in folder: %1").arg(folderPath);
        return false;
    }

    const QString editableFileName = findEditableTextFile(dir, refFileName);
    const QString audioFileName = findAudioFile(dir);

    outTranscript.referencePath = dir.filePath(refFileName);
    outTranscript.editablePath  = editableFileName.isEmpty()
                                     ? QString()
                                     : dir.filePath(editableFileName);
    outTranscript.audioPath     = audioFileName.isEmpty()
                                  ? QString()
                                  : dir.filePath(audioFileName);

    // --- Load reference text and parse ---

    QString rawText;
    if (!loadTextFile(outTranscript.referencePath, rawText, errorMessage)) {
        return false;
    }

    TranscriptParser parser;
    if (!parser.parse(rawText, outTranscript, speakerNames)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Failed to parse transcript text in: %1")
                                .arg(outTranscript.referencePath);
        return false;
    }

    // --- Load or create metadata (meta.json) ---

    const QString metaPath = dir.filePath(QStringLiteral("meta.json"));
    QJsonObject meta;

    if (!loadMetadata(metaPath, meta, errorMessage)) {
        // If meta.json is unreadable but exists, loadMetadata will have set error.
        // If it doesn't exist, loadMetadata returns true and meta is empty.
        // So we only abort if it actually failed to open/parse.
        // For now, we treat "file missing" as OK.
    }

    // Ensure required fields exist
    if (!meta.contains(QStringLiteral("id"))) {
        const QString id = generateTranscriptID(outTranscript.title, outTranscript.folderPath);
        meta.insert(QStringLiteral("id"), id);
    }

    if (!meta.contains(QStringLiteral("title"))) {
        meta.insert(QStringLiteral("title"), outTranscript.title);
    }

    const QDateTime nowUtc = QDateTime::currentDateTimeUtc();

    if (!meta.contains(QStringLiteral("dateImported"))) {
        meta.insert(QStringLiteral("dateImported"), nowUtc.toString(Qt::ISODate));
        outTranscript.dateImported = nowUtc;
    } else {
        outTranscript.dateImported =
            QDateTime::fromString(meta.value(QStringLiteral("dateImported")).toString(),
                                  Qt::ISODate);
    }

    // Always update lastEdited to now on import
    meta.insert(QStringLiteral("lastEdited"), nowUtc.toString(Qt::ISODate));
    outTranscript.lastEdited = nowUtc;

    // Paths stored relative to transcript folder
    meta.insert(QStringLiteral("referencePath"), refFileName);
    meta.insert(QStringLiteral("editablePath"), editableFileName);
    meta.insert(QStringLiteral("audioPath"), audioFileName);

    meta.insert(QStringLiteral("numSpeakers"), speakerNames.size());
    QJsonArray spArray;
    for (const QString& sp : speakerNames)
        spArray.append(sp);
    meta.insert(QStringLiteral("speakers"), spArray);

    // Commit metadata to disk
    if (!saveMetadata(metaPath, meta, errorMessage)) {
        return false;
    }

    // Now that meta is finalized, propagate ID into the Transcript object
    outTranscript.id = meta.value(QStringLiteral("id")).toString();

    return true;
}


QString TranscriptImporter::findReferenceTextFile(const QDir& dir) const {

    // Strategy:
    // 1. Look for a file named "transcript.txt"
    // 2. Otherwise, take the first *.txt in alphabetical order

    QStringList txtFiles = dir.entryList(QStringList() << "*.txt",
                                         QDir::Files | QDir::Readable,
                                         QDir::Name);

    if (txtFiles.isEmpty())
        return QString();

    // Prefer a conventional name if present
    for (const QString& f : txtFiles) {
        if (f.compare(QStringLiteral("transcript.txt"), Qt::CaseInsensitive) == 0 ||
            f.compare(QStringLiteral("ref.txt"), Qt::CaseInsensitive) == 0) {
            return f;
        }
    }

    return txtFiles.first();
}

QString TranscriptImporter::findEditableTextFile(
    const QDir& dir,
    const QString& referenceFileName) const {

    // Strategy:
    // 1. Look for "editable.txt"
    // 2. If not found, and there's exactly one other txt file, use that

    QStringList txtFiles = dir.entryList(QStringList() << "*.txt",
                                         QDir::Files | QDir::Readable,
                                         QDir::Name);

    txtFiles.removeAll(referenceFileName);

    for (const QString& f : txtFiles) {
        if (f.compare(QStringLiteral("editable.txt"), Qt::CaseInsensitive) == 0 ||
            f.compare(QStringLiteral("edit.txt"), Qt::CaseInsensitive) == 0) {
            return f;
        }
    }

    if (txtFiles.size() == 1)
        return txtFiles.first();

    // No editable file found (it can be created later)
    return QString();
}

QString TranscriptImporter::findAudioFile(const QDir& dir) const {

    // Accept several common formats
    const QStringList patterns = { "*.m4a", "*.mp3", "*.wav", "*.aac", "*.flac" };

    for (const QString& pattern : patterns) {
        QStringList files = dir.entryList(QStringList() << pattern,
                                          QDir::Files | QDir::Readable,
                                          QDir::Name);

        if (!files.isEmpty())
            return files.first();
    }

    return QString();
}

bool TranscriptImporter::loadTextFile(
    const QString& absolutePath,
    QString& outText,
    QString* errorMessage) const {

    QFile f(absolutePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Cannot open transcript text file: %1").arg(absolutePath);
        return false;
    }

    QByteArray data = f.readAll();
    outText = QString::fromUtf8(data);
    return true;
}



bool TranscriptImporter::loadMetadata(
    const QString& metaPath,
    QJsonObject& outMeta,
    QString* errorMessage) const {

    QFileInfo info(metaPath);
    if (!info.exists()) {

        // meta.json missing is OK; we just start from empty object
        outMeta = QJsonObject();
        return true;
    }

    QFile f(metaPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Cannot open meta.json: %1").arg(metaPath);
        return false;
    }

    const QByteArray raw = f.readAll();
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(raw, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Error parsing meta.json: %1").arg(parseError.errorString());
        return false;
    }

    if (!doc.isObject()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("meta.json is not a JSON object.");
        return false;
    }

    outMeta = doc.object();
    return true;
}

bool TranscriptImporter::saveMetadata(
    const QString& metaPath,
    const QJsonObject& meta,
    QString* errorMessage) const {

    QFile f(metaPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Cannot write meta.json: %1").arg(metaPath);
        return false;
    }

    QJsonDocument doc(meta);
    f.write(doc.toJson(QJsonDocument::Indented));
    f.close();
    return true;
}



QString TranscriptImporter::generateTranscriptID(
    const QString& title,
    const QString& folderPath) const {

    const QByteArray input = (title + "|" + folderPath).toUtf8();
    const QByteArray hash = QCryptographicHash::hash(input, QCryptographicHash::Sha1);

    // Shorten for readability
    return QString::fromLatin1(hash.toHex().left(16));
}

}
}

