#ifndef MODEL_SERVICE_TRANSCRIPT_EXPORTER_H
#define MODEL_SERVICE_TRANSCRIPT_EXPORTER_H

#include "Model/Data/Transcript.h"

#include <QString>
#include <QJsonObject>

namespace Model {
namespace Service {

/**
 * @brief Exports Transcript data to text files and meta.json on disk.
 *
 * Responsibilities:
 *  - Build .txt content from segments + speakers
 *  - Save edited transcript (editable version) back to disk
 *  - Optionally save reference transcript
 *  - Export / update metadata (meta.json)
 *
 * This class does not manage multiple transcripts or UI; it works on a single
 * Transcript at a time and assumes TranscriptManager / Controller decide when
 * to call it.
 */

class TranscriptExporter {

public:

    /** @brief Default constructor. */
    TranscriptExporter() = default;


    /** @brief Exports the editable transcript to its editablePath.
     *
     * If editablePath is empty, it defaults to "editable.txt" inside the
     * transcript's folderPath and updates the Transcript accordingly.
     */
    bool exportEditableTranscript(Model::Data::Transcript& transcript,
                                  QString* errorMessage = nullptr) const;

    /** @brief Exports the reference transcript to its referencePath.
     *
     * This is optional; typically reference.txt is created by the importer
     * and rarely changed, but this method allows updating it if needed.
     */
    bool exportReferenceTranscript(Model::Data::Transcript& transcript,
                                   QString* errorMessage = nullptr) const;


    /** @brief Exports the transcript to an arbitrary text file path.
     *
     * This does not modify the Transcript's internal paths or metadata.
     */
    bool exportToTextFile(const Model::Data::Transcript& transcript,
                          const QString& absolutePath,
                          QString* errorMessage = nullptr) const;

    /** @brief Writes or updates meta.json in the transcript's folder.
     *
     * Uses the Transcript's current id, title, dates, paths and speaker list.
     */
    bool exportMetadata(Model::Data::Transcript& transcript,
                        QString* errorMessage = nullptr) const;



    /** @brief Convenience method to export editable text and metadata together.
     *
     * Optionally also exports the reference transcript.
     */
    bool exportAll(Model::Data::Transcript& transcript,
                   bool exportReference = false,
                   QString* errorMessage = nullptr) const;

private:

    /** @brief Builds a .txt representation of the transcript from segments. */
    QString buildTranscriptText(const Model::Data::Transcript& transcript) const;

    /** @brief Builds a JSON object representing meta.json for the transcript. */
    QJsonObject buildMetaJson(const Model::Data::Transcript& transcript) const;

    /** @brief Writes UTF-8 text to a file on disk. */
    bool writeTextFile(const QString& absolutePath, const QString& text, QString* errorMessage) const;

    /** @brief Writes meta.json to the transcript's folder. */
    bool writeMetaFile(const QString& folderPath, const QJsonObject& meta, QString* errorMessage) const;

    /** @brief Returns path relative to folderPath, or empty string if path is empty. */
    static QString toRelativePath(const QString& folderPath, const QString& absoluteOrRelativePath);

};

}
}

#endif // MODEL_SERVICE_TRANSCRIPT_EXPORTER_H
