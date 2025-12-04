#ifndef MODEL_DATA_TRANSCRIPT_H
#define MODEL_DATA_TRANSCRIPT_H

#include <QString>
#include <QVector>
#include <QDateTime>

#include "Speaker.h"
#include "Segment.h"

namespace Model {
namespace Data {

/**
 * @brief Represents an entire transcript session (audio + text + speakers).
 *
 * Holds paths, metadata, speaker list, and segments. Acts as a simple data
 * container with lightweight helper functions for searching and merging.
 * Complex editing, parsing, and I/O are handled by Model::Service classes.
 */

class Transcript {

public:

    /** @brief Default constructor. */
    Transcript() = default;

    /** @brief Returns true if an audio file path is set. */
    bool hasAudio() const;

    /** @brief Returns true if an editable transcript path is set. */
    bool hasEditable() const;

    /** @brief Returns true if there are no segments. */
    bool isEmpty() const;


    // === Speaker Helpers ===

    /**
     * @brief Finds the index of a speaker with the given ID.
     * @param speakerID Identifier to search for.
     * @return Index in the speakers vector, or -1 if not found.
     */
    int findSpeakerIndex(const QString& speakerID) const;

    /**
     * @brief Returns a pointer to a speaker by ID (modifiable).
     */
    Speaker* speakerFromID(const QString& speakerID);

    /**
     * @brief Returns a pointer to a speaker by ID (const version).
     */
    const Speaker* speakerFromID(const QString& speakerID) const;

    /**
     * @brief Adds a speaker with the given ID if they do not already exist.
     */
    void addSpeakerIfMissing(const QString& speakerID);

    /**
     * @brief Renames a speaker ID throughout the transcript.
     * @param oldID Existing speaker ID.
     * @param newID New ID to replace it with.
     */
    void renameSpeaker(const QString& oldID, const QString& newID);


    // === Segment Helpers ===

    /** @brief Returns the number of segments. */
    int segmentCount() const;

    /**
     * @brief Adds a new segment to the transcript.
     * @param s The segment to add.
     */
    void addSegment(const Segment& s);

    /**
     * @brief Returns all segments belonging to a speaker.
     * @param speakerID The speaker whose segments to fetch.
     */
    QVector<Segment> segmentsBySpeaker(const QString& speakerID) const;

    /**
     * @brief Merges consecutive segments spoken by the same speaker.
     */
    void mergeAdjacentSameSpeaker();

    /**
     * @brief Returns the entire transcript as a single exportable text block.
     */
    QString allText() const;

    /** @brief Clears all data within this transcript. */
    void clear();


    QString id;
    QString title;
    QString folderPath;

    QString referencePath;
    QString editablePath;
    QString audioPath;

    QVector<Speaker> speakers;
    QVector<Segment> segments;

    QDateTime dateImported;
    QDateTime lastEdited;

    qint64 lastPlaybackPositionMs = 0;
};

}
}


#endif // MODEL_DATA_TRANSCRIPT_H
