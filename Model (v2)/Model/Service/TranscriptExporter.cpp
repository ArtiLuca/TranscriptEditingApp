#include "TranscriptExporter.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDateTime>

namespace Model {
namespace Service {

using namespace Model::Data;

bool TranscriptExporter::exportEditableTranscript(Model::Data::Transcript& transcript,
                                                  QString* errorMessage) const {
    if (transcript.folderPath.isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Transcript folderPath is empty; cannot export editable transcript.");
        return false;
    }

    QDir folder(transcript.folderPath);
    if (!folder.exists()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Transcript folder does not exist: %1").arg(transcript.folderPath);
        return false;
    }

    QString editablePath = transcript.editablePath;

    // If no editablePath yet, default to editable.txt inside the folder
    if (editablePath.isEmpty()) {
        editablePath = folder.filePath(QStringLiteral("editable.txt"));
    }
    else {
        QFileInfo info(editablePath);
        if (!info.isAbsolute())
            editablePath = folder.filePath(editablePath);
    }

    const QString text = buildTranscriptText(transcript);
    if (!writeTextFile(editablePath, text, errorMessage))
        return false;

    // Update lastEdited timestamp to now (save moment)
    transcript.lastEdited = QDateTime::currentDateTimeUtc();
    return true;
}

bool TranscriptExporter::exportReferenceTranscript(Model::Data::Transcript& transcript,
                                                   QString* errorMessage) const {
    if (transcript.folderPath.isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Transcript folderPath is empty; cannot export reference transcript.");
        return false;
    }

    if (transcript.referencePath.isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Transcript referencePath is empty; nothing to export.");
        return false;
    }

    QDir folder(transcript.folderPath);
    if (!folder.exists()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Transcript folder does not exist: %1").arg(transcript.folderPath);
        return false;
    }

    QString refPath = transcript.referencePath;
    QFileInfo refInfo(refPath);
    if (!refInfo.isAbsolute())
        refPath = folder.filePath(refPath);

    const QString text = buildTranscriptText(transcript);
    if (!writeTextFile(refPath, text, errorMessage))
        return false;

    // Reference export does not necessarily mean content was edited now,
    // so we don't touch lastEdited here.
    return true;
}

bool TranscriptExporter::exportToTextFile(const Model::Data::Transcript& transcript,
                                          const QString& absolutePath,
                                          QString* errorMessage) const {
    if (absolutePath.isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Target path for exportToTextFile is empty.");
        return false;
    }

    const QString text = buildTranscriptText(transcript);
    return writeTextFile(absolutePath, text, errorMessage);
}

bool TranscriptExporter::exportMetadata(Model::Data::Transcript& transcript,
                                        QString* errorMessage) const {

    if (transcript.folderPath.isEmpty())  {
        if (errorMessage)
            *errorMessage = QStringLiteral("Transcript folderPath is empty; cannot export metadata.");
        return false;
    }

    QDir folder(transcript.folderPath);
    if (!folder.exists()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Transcript folder does not exist: %1").arg(transcript.folderPath);
        return false;
    }

    QJsonObject meta = buildMetaJson(transcript);
    if (!writeMetaFile(folder.absolutePath(), meta, errorMessage))
        return false;

    return true;
}

bool TranscriptExporter::exportAll(Model::Data::Transcript& transcript,
                                   bool exportReference,
                                   QString* errorMessage) const {
    // 1) Editable text
    if (!exportEditableTranscript(transcript, errorMessage))
        return false;

    // 2) Optional reference text
    if (exportReference) {
        if (!exportReferenceTranscript(transcript, errorMessage))
            return false;
    }

    // 3) Metadata
    if (!exportMetadata(transcript, errorMessage))
        return false;

    return true;
}


QString TranscriptExporter::buildTranscriptText(const Model::Data::Transcript& transcript) const {

    // Build something close to the original format:
    // Each segment begins with "Speaker: first line of text"
    // and continuation lines follow, then a blank line between segments.

    QStringList lines;
    for (const Segment& seg : transcript.segments) {

        const QString speaker = seg.speakerID.isEmpty()
                                    ? QStringLiteral("UNKNOWN")
                                    : seg.speakerID;

        QString segText = seg.text;
        segText = segText.trimmed();

        if (segText.isEmpty())
            continue;

        QStringList segLines = segText.split(QLatin1Char('\n'), Qt::KeepEmptyParts);

        if (segLines.isEmpty())
            continue;

        // First line is prefixed with "Speaker: "
        lines << QStringLiteral("%1: %2").arg(speaker, segLines.takeFirst());

        // Remaining lines are appended as-is
        for (const QString& l : segLines) lines << l;

        // Blank line between segments for readability
        lines << QString();
    }

    // Remove trailing empty lines
    while (!lines.isEmpty() && lines.last().trimmed().isEmpty())
        lines.removeLast();

    return lines.join(QLatin1Char('\n')) + QLatin1Char('\n');
}

QJsonObject TranscriptExporter::buildMetaJson(const Model::Data::Transcript& transcript) const {

    QJsonObject meta;

    // Basic identifiers
    if (!transcript.id.isEmpty())
        meta.insert(QStringLiteral("id"), transcript.id);

    meta.insert(QStringLiteral("title"), transcript.title);

    // Dates: use existing if valid, otherwise generate sensible defaults
    const QDateTime nowUtc = QDateTime::currentDateTimeUtc();
    const QDateTime imported = transcript.dateImported.isValid() ? transcript.dateImported : nowUtc;
    const QDateTime edited = transcript.lastEdited.isValid() ? transcript.lastEdited : nowUtc;

    meta.insert(QStringLiteral("dateImported"), imported.toString(Qt::ISODate));
    meta.insert(QStringLiteral("lastEdited"), edited.toString(Qt::ISODate));

    // referencePath editablePath audioPath speakers numSpeakers
    // Paths stored relative to folderPath
    const QString folderPath = transcript.folderPath;
    meta.insert(QStringLiteral("referencePath"), toRelativePath(folderPath, transcript.referencePath));
    meta.insert(QStringLiteral("editablePath"), toRelativePath(folderPath, transcript.editablePath));
    meta.insert(QStringLiteral("audioPath"), toRelativePath(folderPath, transcript.audioPath));

    // Speakers
    QJsonArray speakersArray;
    for (const Speaker& sp : transcript.speakers) {
        speakersArray.append(sp.id);
    }
    meta.insert(QStringLiteral("speakers"), speakersArray);
    meta.insert(QStringLiteral("numSpeakers"), speakersArray.size());

    return meta;
}

bool TranscriptExporter::writeTextFile(const QString& absolutePath, const QString& text, QString* errorMessage) const {

    QFile f(absolutePath);

    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {

        if (errorMessage)
            *errorMessage = QStringLiteral("Cannot write text file: %1").arg(absolutePath);
        return false;
    }

    const QByteArray utf8 = text.toUtf8();

    if (f.write(utf8) != utf8.size()) {

        if (errorMessage)
            *errorMessage = QStringLiteral("Failed to write full contents to file: %1").arg(absolutePath);
        return false;
    }

    f.close();

    return true;
}

bool TranscriptExporter::writeMetaFile(const QString& folderPath, const QJsonObject& meta, QString* errorMessage) const {

    QDir folder(folderPath);
    const QString metaPath = folder.filePath(QStringLiteral("meta.json"));

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

QString TranscriptExporter::toRelativePath(const QString& folderPath, const QString& absoluteOrRelativePath) {

    if (absoluteOrRelativePath.isEmpty())
        return QString();

    QDir folder(folderPath);
    return folder.relativeFilePath(absoluteOrRelativePath);
}

}
}
