#include "TranscriptSearch.h"

namespace Model {
namespace Service {

using Model::Data::Transcript;
using Model::Data::Segment;

TranscriptSearch::TranscriptSearch(const Transcript& transcript)
    : searchTranscript(transcript)
{}

const Transcript& TranscriptSearch::transcript() const {

    return searchTranscript;
}

QVector<int> TranscriptSearch::findSegmentsContaining(const QString& pattern,
                                                      Qt::CaseSensitivity cs) const {

    QVector<int> result;

    if (pattern.isEmpty())
        return result;

    const auto& segments = searchTranscript.segments;
    for (int i = 0; i < segments.size(); ++i) {
        const QString& text = segments[i].text;
        if (text.contains(pattern, cs)) {
            result.push_back(i);
        }
    }

    return result;
}

int TranscriptSearch::findNext(const QString& pattern,
                               int startIndex,
                               Qt::CaseSensitivity cs) const {

    if (pattern.isEmpty())
        return -1;

    const auto& segments = searchTranscript.segments;
    if (segments.isEmpty())
        return -1;

    int ind = startIndex + 1;
    if (ind < 0)
        ind = 0;

    for (int i = ind; i < segments.size(); ++i) {
        const QString& text = segments[i].text;
        if (text.contains(pattern, cs)) {
            return i;
        }
    }

    return -1;
}

QVector<int> TranscriptSearch::findBySpeaker(const QString& speakerID) const {

    QVector<int> result;

    if (speakerID.isEmpty())
        return result;

    const auto& segments = searchTranscript.segments;
    for (int i = 0; i < segments.size(); ++i) {
        if (segments[i].speakerID == speakerID) {
            result.push_back(i);
        }
    }

    return result;
}

QVector<int> TranscriptSearch::findBySpeakerAndText(const QString& speakerID,
                                                    const QString& pattern,
                                                    Qt::CaseSensitivity cs) const {

    QVector<int> result;

    const bool filterBySpeaker = !speakerID.isEmpty();
    const bool filterByText    = !pattern.isEmpty();

    const auto& segments = searchTranscript.segments;
    for (int i = 0; i < segments.size(); ++i) {
        const Segment& seg = segments[i];

        if (filterBySpeaker && seg.speakerID != speakerID)
            continue;

        if (filterByText) {
            if (!seg.text.contains(pattern, cs))
                continue;
        }

        result.push_back(i);
    }

    return result;

}

QVector<int> TranscriptSearch::findBySpeakersAndText(const QStringList& speakerIDs,
                                                     const QString& pattern,
                                                     Qt::CaseSensitivity cs) const {

    QVector<int> result;

    const bool filterBySpeakers = !speakerIDs.isEmpty();
    const bool filterByText = !pattern.isEmpty();

    const auto& segments = searchTranscript.segments;
    for (int i = 0; i < segments.size(); ++i) {
        const Segment& seg = segments[i];

        if (filterBySpeakers && !speakerIDs.contains(seg.speakerID)) {
            continue;
        }

        if (filterByText) {
            if (!seg.text.contains(pattern, cs))
                continue;
        }

        result.push_back(i);
    }

    return result;
}

}
}
