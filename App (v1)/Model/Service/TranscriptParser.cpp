#include "TranscriptParser.h"

#include <QStringList>
#include <QRegularExpression>
#include <algorithm>

namespace Model {
namespace Service {

using Model::Data::Segment;
using Model::Data::Transcript;


bool TranscriptParser::parse(const QString& rawText,
                             Transcript& outTranscript,
                             const QStringList& knownSpeakers) const
{
    outTranscript.segments.clear();
    outTranscript.speakers.clear();

    if (knownSpeakers.isEmpty())
        return false;

    QVector<Segment> parsedSegments = parseSegments(rawText, knownSpeakers);
    if (parsedSegments.isEmpty())
        return false;

    // Register speakers in the transcript
    for (const QString& sp : knownSpeakers)
        outTranscript.addSpeakerIfMissing(sp);

    // Add segments
    for (const Segment& s : parsedSegments)
        outTranscript.addSegment(s);

    // Optional cleanup: merge consecutive segments that have the same speaker
    outTranscript.mergeAdjacentSameSpeaker();

    return true;
}

bool TranscriptParser::startsWithSpeakerLabel(const QString& line,
                                              const QStringList& knownSpeakers,
                                              QString& outSpeakerID,
                                              QString& outAfterLabel) const {
    outSpeakerID.clear();
    outAfterLabel.clear();

    // Find first non-space character
    int firstNonSpace = 0;
    while (firstNonSpace < line.size() && line[firstNonSpace].isSpace())
        ++firstNonSpace;

    // if empty or whitespace-only line
    if (firstNonSpace >= line.size())
        return false;

    for (const QString& sp : knownSpeakers) {

        const QString label = sp + ":";

        if (line.mid(firstNonSpace, label.size()).compare(label, Qt::CaseInsensitive) == 0) {
            outSpeakerID = sp;
            int afterIndex = firstNonSpace + label.size();
            // keep whatever comes after
            outAfterLabel = line.mid(afterIndex);
            return true;
        }
    }
    return false;
}


QVector<QPair<QString, QString>>
TranscriptParser::splitInlineLabels(const QString& text,
                                    const QStringList& knownSpeakers,
                                    const QString& initialSpeaker) const {

    QVector<QPair<QString, QString>> result;

    QString full = text;
    if (full.isEmpty()) {
        return result;
    }

    struct Hit {
        int pos;
        QString speaker;
    };

    QVector<Hit> hits;

    // Collect all label hits: "Name:"
    for (const QString& sp : knownSpeakers) {

        const QString label = sp + ":";

        int index = full.indexOf(label, 0, Qt::CaseInsensitive);
        while (index != -1) {
            hits.push_back({ index, sp });
            index = full.indexOf(label, index + label.size(), Qt::CaseInsensitive);
        }
    }

    // No labels → entire text belongs to initialSpeaker
    if (hits.isEmpty()) {
        if (!full.trimmed().isEmpty())
            result.append(qMakePair(initialSpeaker, full.trimmed()));
        return result;
    }

    // Sort by position in the line
    std::sort(hits.begin(), hits.end(),
              [](const Hit& a, const Hit& b) {
        return a.pos < b.pos;
    });

    QString activeSpeaker = initialSpeaker;
    int lastPos = 0;

    for (const Hit& hit : hits) {
        int labelPos = hit.pos;
        const QString& newSpeaker = hit.speaker;

        // Text before this label belongs to activeSpeaker
        QString before = full.mid(lastPos, labelPos - lastPos).trimmed();
        if (!before.isEmpty() && !activeSpeaker.isEmpty()) {
            result.append(qMakePair(activeSpeaker, before));
        }

        // Update active speaker to the speaker at this label
        activeSpeaker = newSpeaker;

        // Move position past "Speaker:"
        const QString label = newSpeaker + ":";
        lastPos = labelPos + label.size();
    }

    // Remaining tail belongs to the last active speaker
    QString tail = full.mid(lastPos).trimmed();
    if (!tail.isEmpty() && !activeSpeaker.isEmpty()) {
        result.append(qMakePair(activeSpeaker, tail));
    }

    return result;
}


QVector<Segment> TranscriptParser::parseSegments(const QString& rawText,
                                                 const QStringList& knownSpeakers) const {

    QVector<Segment> segments;

    QStringList lines = rawText.split(QRegularExpression("\\r?\\n"), Qt::KeepEmptyParts);

    QString currentSpeaker;
    QString currentText;

    auto flushSegment = [&](QVector<Segment>& list) {
        QString norm = normalizeText(currentText);
        if (!currentSpeaker.isEmpty() && !norm.isEmpty()) {
            list.append(Segment(currentSpeaker, norm));
        }
        currentText.clear();
    };

    for (const QString& line : lines) {
        // Preserve explicit blank lines as paragraph breaks for the current speaker
        if (line.trimmed().isEmpty()) {
            if (!currentText.isEmpty())
                currentText.append('\n');
            continue;
        }

        // Step 1: check if this line starts with a speaker label
        QString speakerFromLine;
        QString afterLabel;
        bool lineHasSpeaker = startsWithSpeakerLabel(line, knownSpeakers,
                                                     speakerFromLine, afterLabel);

        QString baseSpeaker = currentSpeaker;
        QString content = line;

        if (lineHasSpeaker) {
            // New speaker block encountered
            flushSegment(segments);
            baseSpeaker = speakerFromLine;
            content = afterLabel; // text after "Speaker:"
        }

        // Step 2: run inline splitting on the content (even if it started with a speaker)
        QVector<QPair<QString, QString>> parts =
            splitInlineLabels(content, knownSpeakers, baseSpeaker);

        if (parts.isEmpty()) {
            // No extra labels inside this line → simple continuation for baseSpeaker
            if (!baseSpeaker.isEmpty()) {
                if (currentSpeaker.isEmpty())
                    currentSpeaker = baseSpeaker;

                if (currentSpeaker == baseSpeaker) {
                    if (!currentText.isEmpty())
                        currentText.append('\n');
                    currentText.append(content);
                } else {
                    // Speaker changed mid-stream without explicit label (rare)
                    flushSegment(segments);
                    currentSpeaker = baseSpeaker;
                    currentText = content;
                }
            }
            continue;
        }

        // Step 3: integrate (speaker, text) pairs into the running buffer
        for (const auto& pair : parts) {
            const QString& sp = pair.first;
            const QString& txt = pair.second;

            if (txt.trimmed().isEmpty())
                continue;

            if (currentSpeaker.isEmpty()) {
                // First segment encountered
                currentSpeaker = sp;
                currentText = txt;
            } else if (sp.compare(currentSpeaker, Qt::CaseInsensitive) == 0) {
                // Same speaker → append text
                if (!currentText.isEmpty())
                    currentText.append('\n');
                currentText.append(txt);
            } else {
                // Speaker change → flush and start new
                flushSegment(segments);
                currentSpeaker = sp;
                currentText = txt;
            }
        }
    }

    // Flush the last pending segment, if any
    flushSegment(segments);

    return segments;
}


QString TranscriptParser::normalizeText(const QString& text) const {

    QString t = text;

    // Trim outer whitespace
    t = t.trimmed();

    if (t.isEmpty())
        return t;

    // Collapse 3+ blank lines into 2
    static const QRegularExpression multiBlankLines("\\n{3,}");
    t.replace(multiBlankLines, "\n\n");

    // Trim each line
    QStringList lines = t.split('\n');
    for (QString& line : lines) {
        line = line.trimmed();
    }

    return lines.join('\n');
}


}
}
