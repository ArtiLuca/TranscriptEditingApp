#ifndef MODEL_SERVICE_TRANSCRIPT_SEARCH_H
#define MODEL_SERVICE_TRANSCRIPT_SEARCH_H

#include "Model/Data/Transcript.h"

#include <QString>
#include <QStringList>
#include <QVector>

namespace Model {
namespace Service {

/**
 * @brief Utility class for searching text and speakers inside a Transcript.
 *
 * Provides word/substring search, find-next, and speaker filters for use by
 * higher-level UI components (search bars, speaker filters) without modifying
 * the underlying Transcript.
 */

class TranscriptSearch {

public:

    /** @brief Constructs a search helper bound to a given Transcript. */
    explicit TranscriptSearch(const Model::Data::Transcript& transcript);

    /** @brief Returns the bound transcript. */
    const Model::Data::Transcript& transcript() const;

    /**
     * @brief Finds all segments whose text contains the given pattern.
     *
     * Returns a list of segment indices. If pattern is empty, returns an empty list.
     */
    QVector<int> findSegmentsContaining(const QString& pattern,
                                        Qt::CaseSensitivity cs = Qt::CaseInsensitive) const;

    /**
     * @brief Finds the next segment index containing pattern after startIndex.
     *
     * Returns -1 if not found or if pattern is empty.
     */
    int findNext(const QString& pattern,
                 int startIndex,
                 Qt::CaseSensitivity cs = Qt::CaseInsensitive) const;


    /**
     * @brief Finds all segments spoken by the given speaker.
     *
     * speakerID should match the Segment::speakerID exactly.
     */
    QVector<int> findBySpeaker(const QString& speakerID) const;


    /**
     * @brief Finds segments spoken by the given speaker whose text contains pattern.
     *
     * Returns a list of segment indices. If pattern is empty, only filters by speaker.
     */
    QVector<int> findBySpeakerAndText(const QString& speakerID,
                                      const QString& pattern,
                                      Qt::CaseSensitivity cs = Qt::CaseInsensitive) const;

    /**
     * @brief Finds segments whose speaker is in speakerIDs and whose text contains pattern.
     *
     * Useful for combining multiple speaker checkboxes in the UI.
     * If speakerIDs is empty, behaves like findSegmentsContaining(pattern).
     * If pattern is empty, returns all segments whose speaker is in speakerIDs.
     */
    QVector<int> findBySpeakersAndText(const QStringList& speakerIDs,
                                       const QString& pattern,
                                       Qt::CaseSensitivity cs = Qt::CaseInsensitive) const;

private:

    const Model::Data::Transcript& searchTranscript;
};

}
}

#endif // MODEL_SERVICE_TRANSCRIPT_SEARCH_H
