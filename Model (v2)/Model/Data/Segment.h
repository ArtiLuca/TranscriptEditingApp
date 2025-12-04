#ifndef MODEL_DATA_SEGMENT_H
#define MODEL_DATA_SEGMENT_H

#include <QString>

namespace Model {
namespace Data {

/**
 * @brief Represents a single block of transcript text spoken by one speaker.
 *
 * Contains the speaker identifier and the full text for that segment.
 * Used as a simple data container within the Transcript model.
 */

class Segment {

public:

    /** @brief Default constructor. */
    Segment() = default;

    /**
     * @brief Constructs a segment with a speaker ID and text.
     * @param speakerID The identifier of the speaker (e.g. "Stephen").
     * @param text The text spoken in this segment.
     */
    Segment(const QString& speakerID, const QString& text);


    /** @brief Checks if the segment contains valid (non-empty) speaker and text. */
    bool isValid() const;


    /** @brief Checks whether the text begins with a speaker label (e.g. "Stephen:"). */
    bool startsWithLabel() const;

    /** @brief Appends additional text to this segment, adding a newline if needed. */
    void appendText(const QString& extra);

    /** @brief Returns the cleaned (trimmed) text for display or processing. */
    QString cleanText() const;

    /** @brief Returns this segment in exportable text format ("Speaker:\ntext\n\n"). */
    QString exportFormat() const;


    // === Data Members ===

    QString speakerID;
    QString text;
};

}
}

#endif // MODEL_DATA_SEGMENT_H
