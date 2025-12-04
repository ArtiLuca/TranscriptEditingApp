#ifndef MODEL_SERVICE_TRANSCRIPT_PARSER_H
#define MODEL_SERVICE_TRANSCRIPT_PARSER_H

#include "Model/Data/Transcript.h"

#include <QString>
#include <QStringList>
#include <QVector>
#include <QPair>

namespace Model {
namespace Service {


/**
 * @brief Parses raw transcript text into structured segments using a known list of speaker names (given by user in View and passed by Controller)
 *
 * The parser:
 *  - Detects speaker labels at the beginning of lines (e.g. "Stephen:")
 *  - Splits lines that contain multiple inline speaker labels
 *  - Preserves multi-line segments for the same speaker
 *
 * It assumes that all valid speaker labels are of the form "Name:" where Name is one of the knownSpeakers provided by the caller.
 */

class TranscriptParser {

public:

    /** @brief Default constructor. */
    TranscriptParser() = default;

    /**
     * @brief Parses the given text into a Transcript.
     *
     * @param rawText       Full contents of the transcript text file.
     * @param outTranscript Transcript object to populate (segments + speaker list).
     * @param knownSpeakers List of speaker IDs/names that are valid in this transcript.
     *                      Each label is expected to appear as "Name:" in the text.
     *
     * @return true if at least one segment was successfully parsed.
     */
    bool parse(const QString& rawText,
               Model::Data::Transcript& outTranscript,
               const QStringList& knownSpeakers) const;

private:


    /**
     * @brief Checks if the line begins with a speaker label of the form "Name:"
     *        where Name is in knownSpeakers.
     *
     * @param line          The line to inspect.
     * @param knownSpeakers The list of valid speaker IDs.
     * @param outSpeakerID  On success, receives the detected speaker ID.
     * @param outAfterLabel On success, receives the text following the label.
     *
     * @return true if line starts with a valid speaker label.
     */
    bool startsWithSpeakerLabel(const QString& line,
                                const QStringList& knownSpeakers,
                                QString& outSpeakerID,
                                QString& outAfterLabel) const;


    /**
     * @brief Parses the entire raw text into an ordered list of segments.
     *
     * This function:
     *  - Walks line by line
     *  - Handles speaker lines and continuation lines
     *  - Uses splitInlineLabels() to handle multiple speakers in a single line
     */
    QVector<Model::Data::Segment> parseSegments(const QString& rawText,
                                                const QStringList& knownSpeakers) const;


    /**
     * @brief Splits a line of text into (speaker, text) pieces based on all
     *        occurrences of speaker labels "Name:" for known speakers.
     *
     * The algorithm:
     *  - Scans the entire line for all "Name:" occurrences
     *  - Sorts them by position
     *  - Slices the line into ranges belonging to each speaker in order
     *
     * @param text          The line (or remainder of a line) to split.
     * @param knownSpeakers The list of valid speaker IDs.
     * @param initialSpeaker The speaker ID to use for any text that appears
     *                       before the first label (may be empty).
     *
     * @return A list of (speakerID, text) pairs in chronological order.
     */
    QVector<QPair<QString, QString>>
    splitInlineLabels(const QString& text,
                      const QStringList& knownSpeakers,
                      const QString& initialSpeaker) const;


    /**
     * @brief Normalizes a block of text for a segment.
     *
     * Operations:
     *  - Trim leading/trailing whitespace
     *  - Collapse 3+ consecutive newlines into 2
     *  - Trim each individual line
     */
    QString normalizeText(const QString& text) const;

};

}
}


#endif // MODEL_SERVICE_TRANSCRIPT_PARSER_H
