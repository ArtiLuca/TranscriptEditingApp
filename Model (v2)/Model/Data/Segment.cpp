#include "Segment.h"

namespace Model {
namespace Data {

Segment::Segment(const QString& speakerID,
                 const QString& text)
    : speakerID(speakerID),
    text(text)
{}

bool Segment::isValid() const {

    return !speakerID.isEmpty() && !text.trimmed().isEmpty();
}

bool Segment::startsWithLabel() const {

    // A label ends with ":" — simple heuristic
    return text.contains(":") && text.indexOf(":") < text.indexOf(" ");
}

void Segment::appendText(const QString& extra) {

    if (extra.isEmpty())
        return;
    if (!text.endsWith("\n"))
        text.append("\n");
    text.append(extra);
}

QString Segment::cleanText() const {

    return text.trimmed();
}

QString Segment::exportFormat() const {

    return speakerID + ":\n" + text.trimmed() + "\n\n";
}

}
}
