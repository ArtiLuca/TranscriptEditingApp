#include "TranscriptEditor.h"

#include <QDateTime>
#include <QRegularExpression>

namespace Model {
namespace Service {

using namespace Model::Data;

// === Construction / access ===

TranscriptEditor::TranscriptEditor(Transcript& transcript)
    : editedTranscript(transcript)
{}

const Transcript& TranscriptEditor::transcript() const { return editedTranscript; }

Transcript& TranscriptEditor::transcript() { return editedTranscript; }


// === Segment-level editing ===

bool TranscriptEditor::setSegmentText(int index, const QString& newText) {

    if (!isValidSegmentIndex(index))
        return false;

    saveSnapshot();
    editedTranscript.segments[index].text = newText;
    markEdited();
    return true;
}

bool TranscriptEditor::appendToSegment(int index, const QString& extraText) {

    if (!isValidSegmentIndex(index) || extraText.isEmpty())
        return false;

    saveSnapshot();
    editedTranscript.segments[index].appendText(extraText);
    markEdited();
    return true;
}

int TranscriptEditor::splitSegment(int index, int splitPosition) {

    if (!isValidSegmentIndex(index))
        return -1;

    Segment& seg = editedTranscript.segments[index];
    const QString& text = seg.text;

    if (splitPosition <= 0 ||splitPosition >= text.size())
        return -1;

    saveSnapshot();

    const QString firstPart = text.left(splitPosition).trimmed();
    const QString secondPart = text.mid(splitPosition).trimmed();

    if (firstPart.isEmpty() || secondPart.isEmpty()) {
        // We require both parts to be non-empty for a split.
        // If needed, this behavior can be relaxed later.
        // Revert snapshot to avoid half-changes.
        // (But since we haven't modified anything yet, nothing to revert.)
        return -1;
    }

    seg.text = firstPart;
    Segment newSeg(seg.speakerID, secondPart);

    editedTranscript.segments.insert(index + 1, newSeg);
    markEdited();
    return index + 1;

}

bool TranscriptEditor::mergeWithNext(int index) {

    if (!isValidSegmentIndex(index))
        return false;

    int nextIndex = index + 1;
    if (!isValidSegmentIndex(nextIndex))
        return false;

    saveSnapshot();

    Segment& current = editedTranscript.segments[index];
    const Segment& next = editedTranscript.segments[nextIndex];

    // Append text with a newline separator if needed
    QString mergedText = current.text;
    if (!mergedText.isEmpty() && !mergedText.endsWith("\n"))
        mergedText.append('\n');
    mergedText.append(next.text);

    current.text = mergedText;
    // Speaker remains the same as the original current segment;
    // if needed, this behavior can be customized later.

    editedTranscript.segments.removeAt(nextIndex);
    markEdited();
    return true;
}

bool TranscriptEditor::deleteSegment(int index) {

    if (!isValidSegmentIndex(index))
        return false;

    saveSnapshot();
    editedTranscript.segments.removeAt(index);
    markEdited();
    return true;
}

bool TranscriptEditor::insertSegment(int index, const Segment& segment) {

    if (index < 0 || index > editedTranscript.segments.size())
        return false;

    saveSnapshot();
    editedTranscript.segments.insert(index, segment);
    ensureSpeakerExists(segment.speakerID);
    markEdited();
    return true;
}

bool TranscriptEditor::moveSegment(int fromIndex, int toIndex) {

    if (!isValidSegmentIndex(fromIndex))
        return false;
    if (toIndex < 0 || toIndex >= editedTranscript.segments.size())
        return false;
    if (fromIndex == toIndex)
        return true;

    saveSnapshot();

    Segment seg = editedTranscript.segments.takeAt(fromIndex);
    // If we removed an element before the target index, the target shifts by -1
    if (fromIndex < toIndex)
        --toIndex;

    editedTranscript.segments.insert(toIndex, seg);
    markEdited();
    return true;
}

bool TranscriptEditor::swapSegments(int indexA, int indexB) {

    if (!isValidSegmentIndex(indexA) || !isValidSegmentIndex(indexB))
        return false;
    if (indexA == indexB)
        return true;

    saveSnapshot();
    editedTranscript.segments.swapItemsAt(indexA, indexB);
    markEdited();
    return true;
}

bool TranscriptEditor::removeSegment(int index) {

    return deleteSegment(index);
}

void TranscriptEditor::setSegments(const QVector<Segment>& newSegments) {

    saveSnapshot();
    editedTranscript.segments = newSegments;
    markEdited();
}



// === Speaker-level editing ===

bool TranscriptEditor::setSegmentSpeaker(int index, const QString& speakerID) {

    if (!isValidSegmentIndex(index) || speakerID.trimmed().isEmpty())
        return false;

    saveSnapshot();
    editedTranscript.segments[index].speakerID = speakerID.trimmed();
    ensureSpeakerExists(speakerID.trimmed());
    markEdited();
    return true;
}

bool TranscriptEditor::renameSpeakerGlobal(const QString& oldID, const QString& newID) {

    const QString trimmedOld = oldID.trimmed();
    const QString trimmedNew = newID.trimmed();

    if (trimmedOld.isEmpty() || trimmedNew.isEmpty())
        return false;
    if (trimmedOld == trimmedNew)
        return false;
    if (!hasSpeaker(trimmedOld))
        return false;

    saveSnapshot();
    editedTranscript.renameSpeaker(trimmedOld, trimmedNew);
    markEdited();
    return true;
}


bool TranscriptEditor::hasSpeaker(const QString& speakerID) const {

    return editedTranscript.findSpeakerIndex(speakerID) >= 0;
}

void TranscriptEditor::ensureSpeakerExists(const QString& speakerID) {

    editedTranscript.addSpeakerIfMissing(speakerID);
}


// === Text operations ===

int TranscriptEditor::replaceInSegment(
    int index,
    const QString& from,
    const QString& to,
    Qt::CaseSensitivity cs) {

    if (!isValidSegmentIndex(index))
        return 0;
    if (from.isEmpty())
        return 0;

    saveSnapshot();
    int count = replaceAllInString(editedTranscript.segments[index].text, from, to, cs);
    if (count > 0)
        markEdited();
    else
        undoStack.removeLast(); // No effective change -> discard snapshot

    return count;
}

int TranscriptEditor::replaceAll(
    const QString& from,
    const QString& to,
    Qt::CaseSensitivity cs) {

    if (from.isEmpty())
        return 0;

    saveSnapshot();
    int total = 0;

    for (Segment& seg : editedTranscript.segments) {
        total += replaceAllInString(seg.text, from, to, cs);
    }

    if (total > 0) {
        markEdited();
    }
    else {
        // Nothing changed: discard the snapshot we just saved
        undoStack.removeLast();
    }

    return total;
}

void TranscriptEditor::normalizeWhitespaceAll() {

    if (editedTranscript.segments.isEmpty())
        return;

    saveSnapshot();

    // Simple normalization: trim each segment's text and remove excessive blank lines.
    for (Segment& seg : editedTranscript.segments) {
        QString t = seg.text;

        // Trim each line
        QStringList lines = t.split(QRegularExpression(QStringLiteral("\\r?\\n")),
                                    Qt::KeepEmptyParts);

        for (QString& line : lines) {
            line = line.trimmed();
        }

        // Rebuild with at most one blank line in a row
        QStringList cleaned;
        bool lastWasEmpty = false;
        for (const QString& line : lines) {
            bool isEmpty = line.isEmpty();
            if (isEmpty) {
                if (!lastWasEmpty) {
                    cleaned << QString();
                    lastWasEmpty = true;
                }
            }
            else {
                cleaned << line;
                lastWasEmpty = false;
            }
        }

        seg.text = cleaned.join('\n').trimmed();
    }

    markEdited();
}


// === Undo / Redo ===

void TranscriptEditor::clearHistory() {

    undoStack.clear();
    redoStack.clear();
}

bool TranscriptEditor::canUndo() const {

    return !undoStack.isEmpty();
}

bool TranscriptEditor::canRedo() const {

    return !redoStack.isEmpty();
}

bool TranscriptEditor::undo() {

    if (!canUndo())
        return false;

    Snapshot snapshot = undoStack.takeLast();
    // Save current state to redo before restoring previous
    Snapshot current;
    current.speakers = editedTranscript.speakers;
    current.segments = editedTranscript.segments;
    redoStack.append(current);

    restoreSnapshot(snapshot);
    markEdited();
    return true;
}

bool TranscriptEditor::redo() {

    if (!canRedo())
        return false;

    Snapshot snapshot = redoStack.takeLast();
    // Save current state to undo before restoring next
    Snapshot current;
    current.speakers = editedTranscript.speakers;
    current.segments = editedTranscript.segments;
    undoStack.append(current);

    restoreSnapshot(snapshot);
    markEdited();
    return true;
}


// === Private helpers ===

void TranscriptEditor::saveSnapshot() {

    Snapshot snap;
    snap.speakers = editedTranscript.speakers;
    snap.segments = editedTranscript.segments;
    undoStack.append(snap);
    // new edit invalidates redo history
    redoStack.clear();
}

void TranscriptEditor::restoreSnapshot(const Snapshot& snapshot) {

    editedTranscript.speakers = snapshot.speakers;
    editedTranscript.segments = snapshot.segments;
}

void TranscriptEditor::markEdited() {

    editedTranscript.lastEdited = QDateTime::currentDateTimeUtc();
}

bool TranscriptEditor::isValidSegmentIndex(int index) const {

    return (index >= 0 && index < editedTranscript.segments.size());
}

int TranscriptEditor::replaceAllInString(
    QString& text,
    const QString& from,
    const QString& to,
    Qt::CaseSensitivity cs) {

    if (from.isEmpty())
        return 0;

    int count = 0;
    int pos = 0;

    while ((pos = text.indexOf(from, pos, cs)) != -1) {
        text.replace(pos, from.length(), to);
        pos += to.length();
        ++count;
    }
    return count;
}

}
}
