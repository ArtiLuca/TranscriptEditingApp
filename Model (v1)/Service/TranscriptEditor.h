#ifndef MODEL_SERVICE_TRANSCRIPT_EDITOR_H
#define MODEL_SERVICE_TRANSCRIPT_EDITOR_H

#include "Model/Data/Transcript.h"

#include <QString>
#include <QVector>

namespace Model {
namespace Service {

class TranscriptEditor {

public:

    /** @brief Constructs an editor operating on the given transcript. */
    explicit TranscriptEditor(Model::Data::Transcript& transcript);

    /** @brief Returns a const reference to the underlying transcript. */
    const Model::Data::Transcript& transcript() const;
    /** @brief Returns a mutable reference to the underlying transcript. */
    Model::Data::Transcript& transcript();


    /** @brief Changes the text of the segment at the given index. */
    bool setSegmentText(int index, const QString& newText);

    /** @brief Appends extra text to the segment at the given index. */
    bool appendToSegment(int index, const QString& extraText);

    /**
     * @brief Splits a segment into two at the given character position.
     *
     * The original segment becomes the first part; the second part is inserted
     * immediately after with the same speaker. Returns the index of the new
     * segment on success, or -1 on error.
     */
    int splitSegment(int index, int splitPosition);

    /**
     * @brief Merges the segment at index with the following segment.
     *
     * If speakers differ, both texts are concatenated into the first segment.
     */
    bool mergeWithNext(int index);

    /** @brief Deletes the segment at the given index. */
    bool deleteSegment(int index);

    /** @brief Inserts a new segment at the given index. */
    bool insertSegment(int index, const Model::Data::Segment& segment);

    /**
     * @brief Moves a segment from one index to another.
     *
     * Indices are adjusted as expected when moving towards higher/lower positions.
     */
    bool moveSegment(int fromIndex, int toIndex);

    /** @brief Swaps two segments by index. */
    bool swapSegments(int indexA, int indexB);

    /** @brief Removes the segment at the given index (alias for deleteSegment). */
    bool removeSegment(int index);

    /** @brief Replaces all segments with a new vector of segments. */
    void setSegments(const QVector<Model::Data::Segment>& newSegments);

    // === Speaker-level editing ===

    /** @brief Changes the speaker for a single segment. */
    bool setSegmentSpeaker(int index, const QString& speakerID);

    /**
     * @brief Renames a speaker globally across the transcript.
     *
     * This updates both the speaker list and all segments using that speaker ID.
     */
    bool renameSpeakerGlobal(const QString& oldID, const QString& newID);

    /** @brief Checks whether the given speaker ID exists in the transcript. */
    bool hasSpeaker(const QString& speakerID) const;

    /** @brief Ensures a speaker exists, adding it if missing. */
    void ensureSpeakerExists(const QString& speakerID);

    // === Text operations ===

    /**
     * @brief Replaces occurrences of a substring in a given segment.
     *
     * @return The number of replacements performed.
     */
    int replaceInSegment(int index,
                         const QString& from,
                         const QString& to,
                         Qt::CaseSensitivity cs = Qt::CaseInsensitive);

    /**
     * @brief Replaces occurrences of a substring across all segments.
     *
     * @return The total number of replacements performed.
     */
    int replaceAll(const QString& from,
                   const QString& to,
                   Qt::CaseSensitivity cs = Qt::CaseInsensitive);

    /**
     * @brief Normalizes whitespace in all segments (trim, collapse multiple blank lines).
     *
     * This is a simple normalization pass and can be extended in future.
     */
    void normalizeWhitespaceAll();

    // === Undo / Redo ===

    /** @brief Clears all undo/redo history. */
    void clearHistory();

    /** @brief Returns true if there is at least one undo step available. */
    bool canUndo() const;

    /** @brief Returns true if there is at least one redo step available. */
    bool canRedo() const;

    /** @brief Undoes the last editing operation, if possible. */
    bool undo();

    /** @brief Redoes the last undone operation, if possible. */
    bool redo();


private:

    struct Snapshot {
        QVector<Model::Data::Speaker> speakers;
        QVector<Model::Data::Segment> segments;
    };

    QVector<Snapshot> undoStack;
    QVector<Snapshot> redoStack;

    /** @brief Saves the current speakers/segments to the undo stack. */
    void saveSnapshot();

    /** @brief Restores the transcript from a given snapshot. */
    void restoreSnapshot(const Snapshot& snapshot);

    /** @brief Marks the transcript as edited by updating lastEdited. */
    void markEdited();

    /** @brief Checks whether a segment index is valid. */
    bool isValidSegmentIndex(int index) const;

    /** @brief Helper to perform a string replacement inside one QString. */
    static int replaceAllInString(QString& text,
                                  const QString& from,
                                  const QString& to,
                                  Qt::CaseSensitivity cs);

    Model::Data::Transcript& editedTranscript;

};

}
}

#endif // MODEL_SERVICE_TRANSCRIPT_EDITOR_H
