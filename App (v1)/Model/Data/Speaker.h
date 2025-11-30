#ifndef MODEL_DATA_SPEAKER_H
#define MODEL_DATA_SPEAKER_H

#include <QString>
#include <QColor>

namespace Model {
namespace Data {

/**
 * @brief Represents one speaker who appears in a transcript.
 *
 * Stores the speaker's stable ID, display name, and an optional UI color.
 */

class Speaker {

public:

    /** @brief Default constructor. */
    Speaker() = default;

    /**
     * @brief Constructs a speaker object.
     * @param id Stable identifier (e.g. "Stephen").
     * @param displayName Name shown to the user.
     * @param color Optional highlight color.
     */
    Speaker(const QString& id, const QString& displayName, const QColor& color = QColor());


    /** @brief Returns true if the speaker has valid ID and display name. */
    bool isValid() const;

    /** @brief Returns the speaker label used in text export (e.g. "Stephen:"). */
    QString toLabel() const;

    /** @brief Assigns a random highlight color to the speaker. */
    void assignRandomColor();

    QString id;
    QString displayName;
    QColor color;
};

}
}


#endif // MODEL_DATA_SPEAKER_H
