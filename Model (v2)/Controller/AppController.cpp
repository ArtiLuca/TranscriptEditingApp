#include "AppController.h"

#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <QDebug>

namespace Controller {

using Model::Data::Transcript;
using Model::Service::TranscriptSearch;
using Model::Service::TranscriptEditor;


AppController::AppController(QObject* parent)
    : QObject(parent),
    m_manager(QString()),
    m_editor(nullptr),
    m_exporter(),
    m_currentIndex(-1),
    m_mediaPlayer(new QMediaPlayer(this)),
    m_audioOutput(new QAudioOutput(this)),
    m_durationMs(0)
{
    m_mediaPlayer->setAudioOutput(m_audioOutput);

    connect(m_mediaPlayer, &QMediaPlayer::positionChanged,
            this, &AppController::handleMediaPositionChanged);
    connect(m_mediaPlayer, &QMediaPlayer::durationChanged,
            this, &AppController::handleMediaDurationChanged);
    connect(m_mediaPlayer, &QMediaPlayer::playbackStateChanged,
            this, &AppController::handleMediaPlaybackStateChanged);
}


AppController::~AppController() {

    delete m_editor;
    m_editor = nullptr;
}

void AppController::setRootDirectory(const QString& dir) {

    m_manager.setRootDirectory(dir);
}

QString AppController::rootDirectory() const {

    return m_manager.rootDirectory();
}


bool AppController::loadTranscripts(QString* errorMessage) {

    const bool ok = m_manager.loadAllFromRoot(errorMessage);
    if (!ok)
        return false;

    // Select first transcript if available
    if (m_manager.transcriptCount() > 0) {
        m_currentIndex = 0;
        updateMediaForCurrentTranscript();
        recreateEditorForCurrentTranscript();
        emit currentTranscriptChanged(currentTranscript());
    }
    else {
        m_currentIndex = -1;
        updateMediaForCurrentTranscript();
        recreateEditorForCurrentTranscript();
        emit currentTranscriptChanged(nullptr);
    }

    emit transcriptsReloaded();
    return true;
}


int AppController::transcriptCount() const {

    return m_manager.transcriptCount();
}

const Model::Data::Transcript* AppController::transcriptAt(int index) const {

    return m_manager.transcriptAt(index);
}

Model::Data::Transcript* AppController::transcriptAt(int index) {

    return m_manager.transcriptAt(index);
}

int AppController::currentTranscriptIndex() const {

    return m_currentIndex;
}

Model::Data::Transcript* AppController::currentTranscript() {

    return m_manager.transcriptAt(m_currentIndex);
}

const Model::Data::Transcript* AppController::currentTranscript() const {

    return m_manager.transcriptAt(m_currentIndex);
}

QStringList AppController::currentTranscriptSpeakers() const {

    const Transcript* t = currentTranscript();
    if (!t)
        return {};

    QStringList result;
    result.reserve(t->speakers.size());
    for (const auto& sp : t->speakers)
        result.append(sp.displayName);
    return result;
}

QStringList AppController::transcriptTitles() const {

    QStringList titles;
    const int count = m_manager.transcriptCount();
    titles.reserve(count);

    for (int i = 0; i < count; ++i) {
        const Transcript* t = m_manager.transcriptAt(i);
        if (t)
            titles.append(t->title);
    }

    return titles;
}




QMediaPlayer* AppController::mediaPlayer() const {

    return m_mediaPlayer;
}

QAudioOutput* AppController::audioOutput() const {

    return m_audioOutput;
}

Model::Service::TranscriptSearch AppController::createSearchForCurrentTranscript() const {

    const Transcript* t = currentTranscript();
    if (!t) {
        // In practice you may want to guard against this in the caller;
        // here we just create a search on an empty dummy transcript if needed.
        static Transcript dummy;
        return TranscriptSearch(dummy);
    }

    return TranscriptSearch(*t);
}

TranscriptEditor* AppController::editor() {

    return m_editor;
}

const TranscriptEditor* AppController::editor() const {

    return m_editor;
}


// ==== Search helpers ====

QVector<int> AppController::searchSegments(const QString& pattern,
                                           const QStringList& speakerFilter,
                                           Qt::CaseSensitivity cs) const {

    const Transcript* t = currentTranscript();
    if (!t || pattern.trimmed().isEmpty())
        return {};

    TranscriptSearch search(*t);

    if (speakerFilter.isEmpty())
        return search.findSegmentsContaining(pattern, cs);

    return search.findBySpeakersAndText(speakerFilter, pattern, cs);
}

int AppController::searchNext(const QString& pattern,
                              const QStringList& speakerFilter,
                              int fromIndex,
                              Qt::CaseSensitivity cs) const {

    const Transcript* t = currentTranscript();
    if (!t || pattern.trimmed().isEmpty())
        return -1;

    TranscriptSearch search(*t);

    if (speakerFilter.isEmpty()) {
        return search.findNext(pattern, fromIndex, cs);
    }

    // With speaker filter: search in the filtered list for first index > fromIndex
    const QVector<int> matches = search.findBySpeakersAndText(speakerFilter, pattern, cs);

    for (int ind : matches) {
        if (ind > fromIndex)
            return ind;
    }
    return -1;

}


// ==== Selection ====


void AppController::selectTranscript(int index) {

    if (index < 0 || index >= m_manager.transcriptCount())
        return;

    if (m_currentIndex == index)
        return;

    m_currentIndex = index;
    updateMediaForCurrentTranscript();
    recreateEditorForCurrentTranscript();
    emit currentTranscriptChanged(currentTranscript());
}

// ==== Editing / undo/redo ====

void AppController::requestUndo() {

    if (!m_editor)
        return;

    m_editor->undo();
    emit transcriptContentChanged(currentTranscript());
    emitUndoRedoAvailability();
}

void AppController::requestRedo() {

    if (!m_editor)
        return;

    m_editor->redo();
    emit transcriptContentChanged(currentTranscript());
    emitUndoRedoAvailability();
}

void AppController::requestSetSegmentText(int index, const QString& text) {

    if (!m_editor)
        return;

    m_editor->setSegmentText(index, text);
    emit transcriptContentChanged(currentTranscript());
    emitUndoRedoAvailability();
}

void AppController::requestAppendToSegment(int index, const QString& text) {

    if (!m_editor)
        return;

    m_editor->appendToSegment(index, text);
    emit transcriptContentChanged(currentTranscript());
    emitUndoRedoAvailability();
}

void AppController::requestSplitSegment(int index, int splitPos) {

    if (!m_editor)
        return;

    m_editor->splitSegment(index, splitPos);
    emit transcriptContentChanged(currentTranscript());
    emitUndoRedoAvailability();
}

bool AppController::requestSplitSegmentWithSpeakers(int index,
                                                    int splitPos,
                                                    const QString& speakerFirst,
                                                    const QString& speakerSercond) {
    if (!m_editor)
        return false;

    const int newIndex = m_editor->splitSegmentWithSpeakers(
        index, splitPos, speakerFirst, speakerSercond);

    if (newIndex < 0)
        return false;

    emit transcriptContentChanged(currentTranscript());
    emitUndoRedoAvailability();
    return true;
}


void AppController::requestMergeWithNext(int index) {

    if (!m_editor)
        return;

    m_editor->mergeWithNext(index);
    emit transcriptContentChanged(currentTranscript());
    emitUndoRedoAvailability();
}

void AppController::requestInsertSegment(int index,
                                         const QString& speakerID,
                                         const QString& text) {

    if (!m_editor)
        return;

    // Allow empty text; the user will type later.
    Model::Data::Segment seg(speakerID, text);

    m_editor->insertSegment(index, seg);
    emit transcriptContentChanged(currentTranscript());
    emitUndoRedoAvailability();
}

void AppController::requestDeleteSegment(int index) {

    if (!m_editor)
        return;

    m_editor->deleteSegment(index);
    emit transcriptContentChanged(currentTranscript());
    emitUndoRedoAvailability();
}

void AppController::requestMoveSegment(int fromIndex, int toIndex) {

    if (!m_editor)
        return;

    m_editor->moveSegment(fromIndex, toIndex);
    emit transcriptContentChanged(currentTranscript());
    emitUndoRedoAvailability();
}

void AppController::requestSwapSegments(int indexA, int indexB) {

    if (!m_editor)
        return;

    m_editor->swapSegments(indexA, indexB);
    emit transcriptContentChanged(currentTranscript());
    emitUndoRedoAvailability();
}

void AppController::requestChangeSegmentSpeaker(int index, const QString& speakerID) {

    if (!m_editor)
        return;

    m_editor->setSegmentSpeaker(index, speakerID);
    emit transcriptContentChanged(currentTranscript());
    emitUndoRedoAvailability();
}

void AppController::requestRenameSpeakerGlobal(const QString& oldID, const QString& newID) {

    if (!m_editor)
        return;

    m_editor->renameSpeakerGlobal(oldID, newID);
    emit transcriptContentChanged(currentTranscript());
    emitUndoRedoAvailability();
}

void AppController::requestReplaceAll(const QString& pattern,
                                      const QString& replacement,
                                      Qt::CaseSensitivity cs) {

    if (!m_editor)
        return;

    m_editor->replaceAll(pattern, replacement, cs);
    emit transcriptContentChanged(currentTranscript());
    emitUndoRedoAvailability();
}

void AppController::requestReplaceInSegment(int index,
                             const QString& from,
                             const QString& to,
                            Qt::CaseSensitivity cs) {
    if (!m_editor)
        return;

    m_editor->replaceInSegment(index, from, to, cs);
    emit transcriptContentChanged(currentTranscript());
    emitUndoRedoAvailability();
}

void AppController::requestNormalizeWhitespaceAll() {

    if (!m_editor)
        return;

    m_editor->normalizeWhitespaceAll();
    emit transcriptContentChanged(currentTranscript());
    emitUndoRedoAvailability();
}



// ==== Import / export ====

bool AppController::requestImportTranscript(const QString& folderPath,
                                            const QStringList& speakerNames,
                                            QString* errorMessage) {

    int newIndex = -1;
    if (!m_manager.importTranscriptFromFolder(folderPath, speakerNames, &newIndex, errorMessage)) {

        if (errorMessage && !errorMessage->isEmpty())
            emit errorOccurred(*errorMessage);
        else
            emit errorOccurred(QStringLiteral("Failed to import transcript from folder."));
        return false;
    }

    // We successfully imported a new transcript
    emit transcriptsReloaded();

    m_currentIndex = newIndex;
    updateMediaForCurrentTranscript();
    recreateEditorForCurrentTranscript();

    Transcript* t = currentTranscript();
    emit importCompleted(newIndex, t);
    emit currentTranscriptChanged(t);

    return true;
}


void AppController::requestSaveCurrent(bool exportReference) {

    Transcript* t = currentTranscript();
    if (!t)
        return;

    QString error;
    if (!m_exporter.exportAll(*t, exportReference, &error)) {
        emit errorOccurred(error.isEmpty()
                               ? tr("Failed to save current transcript.")
                               : error);
        return;
    }

    emit saveCompleted(t);
}

void AppController::requestSaveAll(bool exportReference) {

    QString error;

    for (int i = 0; i < m_manager.transcriptCount(); ++i) {
        Transcript* t = m_manager.transcriptAt(i);
        if (!t)
            continue;

        error.clear();
        if (!m_exporter.exportAll(*t, false, &error)) {
            emit errorOccurred(error.isEmpty()
                               ? tr("Failed to save transcript at index %1").arg(i)
                               : error);
            // you can `continue` or `break` here, depending on what you want
        }
        else {
            emit saveCompleted(t);
        }
    }
}

// ==== Audio ====

void AppController::requestPlayPause() {

    const Transcript* t = currentTranscript();
    if (!t || !t->hasAudio())
        return;

    if (m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
        m_mediaPlayer->pause();
    }
    else {
        m_mediaPlayer->play();
    }
}

void AppController::requestStop() {

    m_mediaPlayer->stop();
}

void AppController::requestSeek(qint64 positionMs) {

    if (positionMs < 0)
        positionMs = 0;
    m_mediaPlayer->setPosition(positionMs);
}

void AppController::requestJumpRelativeMs(qint64 deltaMs) {

    qint64 newPos = m_mediaPlayer->position() + deltaMs;
    if (newPos < 0)
        newPos = 0;
    m_mediaPlayer->setPosition(newPos);
}


// ==== Media slots ====

void AppController::handleMediaPositionChanged(qint64 position) {

    // Store last playback position into the current transcript (if any)
    Transcript* t = currentTranscript();
    if (t)
        t->lastPlaybackPositionMs = position;

    emit audioPositionChanged(position, m_durationMs);
}

void AppController::handleMediaDurationChanged(qint64 duration) {

    m_durationMs = duration;
    emit audioPositionChanged(m_mediaPlayer->position(), m_durationMs);
}

void AppController::handleMediaPlaybackStateChanged(QMediaPlayer::PlaybackState state) {

    emit audioPlaybackStateChanged(state);
}


// ==== Private helpers ====


void AppController::updateMediaForCurrentTranscript() {

    const Transcript* t = currentTranscript();
    if (!t || !t->hasAudio()) {
        m_mediaPlayer->setSource(QUrl());
        m_mediaPlayer->stop();
        m_durationMs = 0;
        emit audioPositionChanged(0, 0);
        return;
    }

    // Resolve audio path (absolute or relative to folder)
    QString audioPath = t->audioPath;
    QFileInfo info(audioPath);
    if (!info.isAbsolute()) {
        QDir folder(t->folderPath);
        audioPath = folder.filePath(audioPath);
    }

    m_mediaPlayer->setSource(QUrl::fromLocalFile(audioPath));
    m_mediaPlayer->stop();
    m_durationMs = 0;
    emit audioPositionChanged(0, 0);
}

void AppController::recreateEditorForCurrentTranscript() {

    delete m_editor;
    m_editor = nullptr;

    Transcript* t = currentTranscript();
    if (!t) {
        emitUndoRedoAvailability();
        return;
    }

    m_editor = new TranscriptEditor(*t);
    emitUndoRedoAvailability();

}

void AppController::emitUndoRedoAvailability() {

    bool canUndo = false;
    bool canRedo = false;

    if (m_editor) {
        canUndo = m_editor->canUndo();
        canRedo = m_editor->canRedo();
    }

    emit undoRedoAvailabilityChanged(canUndo, canRedo);
}

}
