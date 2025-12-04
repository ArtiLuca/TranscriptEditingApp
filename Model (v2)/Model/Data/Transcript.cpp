#include "Transcript.h"

namespace Model {
namespace Data {

// Checks/Validity

bool Transcript::hasAudio() const { return !audioPath.isEmpty(); }
bool Transcript::hasEditable() const { return !editablePath.isEmpty(); }
bool Transcript::isEmpty() const { return segments.isEmpty(); }

// === SPEAKER HELPERS ===

int Transcript::findSpeakerIndex(const QString& speakerID) const {

    for (int i = 0; i < speakers.size(); ++i)
        if (speakers[i].id == speakerID)
            return i;
    return -1;
}

Speaker* Transcript::speakerFromID(const QString& speakerID) {

    int ind = findSpeakerIndex(speakerID);
    return (ind >= 0 ? &speakers[ind] : nullptr);
}

const Speaker* Transcript::speakerFromID(const QString& speakerID) const {

    int ind = findSpeakerIndex(speakerID);
    return (ind >= 0 ? &speakers[ind] : nullptr);
}

void Transcript::addSpeakerIfMissing(const QString& speakerID) {

    if (findSpeakerIndex(speakerID) == -1)
        speakers.push_back(Speaker(speakerID, speakerID));
}

void Transcript::renameSpeaker(const QString& oldID, const QString& newID) {

    int ind = findSpeakerIndex(oldID);
    if (ind < 0)
        return;

    speakers[ind].id = newID;
    speakers[ind].displayName = newID;

    for (auto& seg : segments)
        if (seg.speakerID == oldID)
            seg.speakerID = newID;
}


// === SEGMENT HELPERS ===

int Transcript::segmentCount() const {

    return segments.size();
}

void Transcript::addSegment(const Segment& s) {

    segments.push_back(s);
}

QVector<Segment> Transcript::segmentsBySpeaker(const QString& speakerID) const {

    QVector<Segment> out;
    out.reserve(segments.size());

    for (const auto& seg : segments)
        if (seg.speakerID == speakerID)
            out.push_back(seg);

    return out;
}

void Transcript::mergeAdjacentSameSpeaker() {

    if (segments.size() < 2)
        return;

    QVector<Segment> merged;
    merged.reserve(segments.size());

    Segment current = segments[0];

    for (int i = 1; i < segments.size(); ++i) {
        const auto& next = segments[i];
        if (next.speakerID == current.speakerID) {
            current.appendText(next.text);
        }
        else {
            merged.push_back(current);
            current = next;
        }
    }

    merged.push_back(current);
    segments = merged;
}

QString Transcript::allText() const {

    QString out;

    for (const auto& seg : segments)
        out += seg.exportFormat();

    return out.trimmed() + "\n";
}


// === RESET ===

void Transcript::clear() {

    speakers.clear();
    segments.clear();
    id.clear();
    title.clear();
    referencePath.clear();
    editablePath.clear();
    audioPath.clear();
}


}
}
