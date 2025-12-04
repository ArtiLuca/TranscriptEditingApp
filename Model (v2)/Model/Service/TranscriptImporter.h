#ifndef MODEL_SERVICE_TRANSCRIPT_IMPORTER_H
#define MODEL_SERVICE_TRANSCRIPT_IMPORTER_H

#include "Model/Data/Transcript.h"

#include <QDir>
#include <QString>
#include <QStringList>
#include <QJsonObject>

namespace Model {
namespace Service {

/**
 * @brief Imports a transcript from a folder on disk into a Transcript object.
 *
 * Responsibilities:
 *  - Validate folder structure
 *  - Detect reference text file, optional editable file, and audio file
 *  - Load and parse the reference transcript using TranscriptParser
 *  - Create or update meta.json with basic metadata (id, dates, paths, speakers)
 *
 * This class does NOT manage multiple transcripts or UI; that is handled
 * by Model::Service::TranscriptManager and the Controller layer.
 */

class TranscriptImporter {

public:

    /** @brief Constructs an importer with an optional application root directory. */
    explicit TranscriptImporter(const QString& rootDir = QString());

    /**
     * @brief Imports a transcript from the given folder into outTranscript.
     *
     * The method locates text/audio files, parses the reference text using
     * TranscriptParser, and creates/updates meta.json with basic metadata.
     */
    bool importFromFolder(const QString& folderPath,
                          const QStringList& speakerNames,
                          Model::Data::Transcript& outTranscript,
                          QString* errorMessage = nullptr) const;


private:

    /** @brief Finds the reference text file (e.g. transcript.txt / ref.txt) in the folder. */
    QString findReferenceTextFile(const QDir& dir) const;
    /** @brief Finds an editable text file distinct from the reference (e.g. editable.txt). */
    QString findEditableTextFile(const QDir& dir, const QString& referenceFileName) const;
    /** @brief Finds an audio file (m4a/mp3/wav/…) in the folder. */
    QString findAudioFile(const QDir& dir) const;

    /** @brief Loads a UTF-8 text file into a QString. */
    bool loadTextFile(const QString& absolutePath,
                      QString& outText,
                      QString* errorMessage) const;

    /** @brief Loads meta.json from disk into a QJsonObject if it exists. */
    bool loadMetadata(const QString& metaPath,
                      QJsonObject& outMeta,
                      QString* errorMessage) const;

    /** @brief Saves a QJsonObject as meta.json on disk. */
    bool saveMetadata(const QString& metaPath,
                      const QJsonObject& meta,
                      QString* errorMessage) const;

      /** @brief Generates a stable transcript ID from title and folder path. */
    QString generateTranscriptID(const QString& title, const QString& folderPath) const;

    QString rootDirPath;

};

}
}

#endif // MODEL_SERVICE_TRANSCRIPT_IMPORTER_H
