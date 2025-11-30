#ifndef MODEL_SERVICE_TRANSCRIPT_MANAGER_H
#define MODEL_SERVICE_TRANSCRIPT_MANAGER_H

#include "Model/Data/Transcript.h"
#include "Model/Service/TranscriptImporter.h"

#include <QString>
#include <QStringList>
#include <QVector>

namespace Model {
namespace Service {


/**
 * @brief Central repository class that owns and manages multiple Transcript objects.
 *
 * Responsibilities:
 *  - Keep a collection of loaded transcripts in memory
 *  - Know the root directory that contains transcript folders
 *  - Load transcripts from the root directory (via meta.json + TranscriptImporter)
 *  - Import new transcripts from arbitrary folders
 *
 * It does NOT perform editing, searching, or audio playback. Those are handled by
 * other Model::Service classes and the Controller layer.
 */

class TranscriptManager {

public:

    /** @brief Constructs a manager with an optional root directory. */
    explicit TranscriptManager(const QString& dir = QString());


    /** @brief Sets the root directory containing transcript folders. */
    void setRootDirectory(const QString& dir);

    /** @brief Returns the current root directory path. */
    QString rootDirectory() const;


    /**
     * @brief Loads all transcripts from the current root directory.
     *
     * The manager scans each subfolder of the root directory that contains a
     * meta.json with a "speakers" array, then uses TranscriptImporter to fully
     * import and parse the transcript.
     */
    bool loadAllFromRoot(QString* errorMessage = nullptr);


    /**
     * @brief Imports a single transcript folder and adds it to the collection.
     *
     * @param folderPath    Path to a folder containing transcript.txt/editable.txt/audio/meta.
     * @param speakerNames  List of known speaker names (e.g. {"Stephen","Stan"}).
     * @param outIndex      Optional pointer to receive the index of the added transcript.
     * @param errorMessage  Optional pointer to receive a human-readable error.
     */
    bool importTranscriptFromFolder(const QString& folderPath,
                                    const QStringList& speakerNames,
                                    int* outIndex = nullptr,
                                    QString* errorMessage = nullptr);


    /** @brief Clears all loaded transcripts from memory. */
    void clear();

    /** @brief Returns the number of loaded transcripts. */
    int transcriptCount() const;


    /** @brief Returns a const reference to the internal transcript list. */
    const QVector<Model::Data::Transcript>& transcripts() const;

    /** @brief Returns a const pointer to the transcript at the given index, or nullptr on error. */
    const Model::Data::Transcript* transcriptAt(int index) const;

    /** @brief Returns a pointer to the transcript at the given index, or nullptr on error. */
    Model::Data::Transcript* transcriptAt(int index);


    /** @brief Finds the index of a transcript by its ID, or -1 if not found. */
    int indexOfTranscriptByID(const QString& id) const;

private:

    QString rootDir;
    QVector<Model::Data::Transcript> transcriptList;
    TranscriptImporter importer;

};

}
}


#endif // MODEL_SERVICE_TRANSCRIPT_MANAGER_H
