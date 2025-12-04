#ifndef CONTROLLER_APP_CONTROLLER_H
#define CONTROLLER_APP_CONTROLLER_H

#include "Model/Service/TranscriptManager.h"
#include "Model/Service/TranscriptEditor.h"
#include "Model/Service/TranscriptExporter.h"
#include "Model/Service/TranscriptSearch.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMediaPlayer>
#include <QAudioOutput>

namespace Controller {

/**
 * @brief Central application controller connecting View and Model.
 *
 * Owns the TranscriptManager and audio player, tracks the currently selected
 * transcript and exposes slots/signals for the View to drive playback,
 * selection, editing, searching, importing and saving.
 */

class AppController : public QObject {

    Q_OBJECT

public:

    /** @brief Constructs an AppController with optional parent QObject. */
    explicit AppController(QObject* parent = nullptr);

    ~AppController();

    // ==== Root / loading / listing ====

    /** @brief Sets the root directory containing transcript folders. */
    void setRootDirectory(const QString& dir);

    /** @brief Returns the current root directory path. */
    QString rootDirectory() const;


    /** @brief Returns the titles of all loaded transcripts (for sidebars, etc.). */
    QStringList transcriptTitles() const;

    /**
     * @brief Loads all transcripts from the current root directory.
     *
     * Emits transcriptsReloaded() and currentTranscriptChanged() if successful.
     */
    bool loadTranscripts(QString* errorMessage = nullptr);

    /** @brief Returns the number of loaded transcripts. */
    int transcriptCount() const;

    /** @brief Returns a const pointer to the transcript at index, or nullptr on error. */
    const Model::Data::Transcript* transcriptAt(int index) const;

    /** @brief Returns a non-const pointer to the transcript at index, or nullptr on error. */
    Model::Data::Transcript* transcriptAt(int index);

    /** @brief Returns the index of the currently selected transcript, or -1 if none. */
    int currentTranscriptIndex() const;

    /** @brief Returns the currently selected transcript, or nullptr if none. */
    Model::Data::Transcript* currentTranscript();
    const Model::Data::Transcript* currentTranscript() const;

    /** @brief Returns the list of speaker display names of the current transcript. */
    QStringList currentTranscriptSpeakers() const;


    /** @brief Returns the QMediaPlayer used for audio playback. */
    QMediaPlayer* mediaPlayer() const;

    /** @brief Returns the QAudioOutput linked to the media player. */
    QAudioOutput* audioOutput() const;


    /**
     * @brief Creates a search helper for the current transcript.
     *
     * Returns a TranscriptSearch bound to the current Transcript. If there is
     * no current transcript, this returns a search helper bound to a dummy
     * empty transcript.
     */
    Model::Service::TranscriptSearch createSearchForCurrentTranscript() const;


    /** @brief Returns the editor used for the current transcript (may be nullptr). */
    Model::Service::TranscriptEditor* editor();
    const Model::Service::TranscriptEditor* editor() const;

    /**
     * @brief Search helper for the UI: find all segment indices matching pattern.
     *
     * If speakerFilter is empty, performs a plain text search over all segments.
     * Otherwise, restricts search to segments whose speaker is in speakerFilter.
     */
    QVector<int> searchSegments(const QString& pattern,
                                const QStringList& speakerFilter,
                                Qt::CaseSensitivity cs = Qt::CaseInsensitive) const;

    /**
     * @brief Search helper for "Find next" starting from fromIndex (exclusive).
     *
     * If speakerFilter is empty, uses TranscriptSearch::findNext; otherwise it
     * searches within the list returned by findBySpeakersAndText.
     */
    int searchNext(const QString& pattern,
                   const QStringList& speakerFilter,
                   int fromIndex,
                   Qt::CaseSensitivity cs = Qt::CaseInsensitive) const;

Q_SIGNALS:

    /** @brief Emitted after loadTranscripts() completes successfully. */
    void transcriptsReloaded();

    /** @brief Emitted when the current transcript index changes. */
    void currentTranscriptChanged(Model::Data::Transcript* transcript);

    /** @brief Emitted whenever the current transcript content changes. */
    void transcriptContentChanged(Model::Data::Transcript* transcript);

    /** @brief Emitted when a save operation completes successfully. */
    void saveCompleted(Model::Data::Transcript* transcript);

    /**
     * @brief Emitted when a new transcript is imported and added.
     * @param newIndex Index of the newly added transcript.
     * @param transcript Pointer to the new transcript.
     */
    void importCompleted(int newIndex, Model::Data::Transcript* transcript);

    /** @brief Emitted whenever an error occurs that should be shown in the UI. */
    void errorOccurred(const QString& message);

    /** @brief Emitted when undo/redo availability changes. */
    void undoRedoAvailabilityChanged(bool canUndo, bool canRedo);

    /** @brief Emitted when audio playback position changes. */
    void audioPositionChanged(qint64 positionMs, qint64 durationMs);

    /** @brief Emitted when audio playback state changes (playing/paused/stopped). */
    void audioPlaybackStateChanged(QMediaPlayer::PlaybackState state);

public Q_SLOTS:

    /** @brief Selects the transcript at index and updates audio source. */
    void selectTranscript(int index);

    // ==== Editing / undo/redo ====

    /** @brief Performs undo on the current transcript (if possible). */
    void requestUndo();

    /** @brief Performs redo on the current transcript (if possible). */
    void requestRedo();

    /** @brief Sets the text of a segment. */
    void requestSetSegmentText(int index, const QString& text);

    /** @brief Appends text to a segment. */
    void requestAppendToSegment(int index, const QString& text);

    /** @brief Splits a segment at a given character position. */
    void requestSplitSegment(int index, int splitPos);

    bool requestSplitSegmentWithSpeakers(int index,
                                         int splitPos,
                                         const QString& speakerFirst,
                                         const QString& speakerSercond);

    /** @brief Merges the segment at index with the next one. */
    void requestMergeWithNext(int index);

    /** @brief Inserts a new segment at position index. */
    void requestInsertSegment(int index,
                              const QString& speakerID,
                              const QString& text);

    /** @brief Deletes the segment at index. */
    void requestDeleteSegment(int index);

    /** @brief Moves a segment from one index to another. */
    void requestMoveSegment(int fromIndex, int toIndex);

    /** @brief Swaps the segments at positions indexA and indexB. */
    void requestSwapSegments(int indexA, int indexB);

    /** @brief Changes the speaker for a given segment. */
    void requestChangeSegmentSpeaker(int index, const QString& speakerID);

    /** @brief Renames a speaker globally throughout the transcript. */
    void requestRenameSpeakerGlobal(const QString& oldID, const QString& newID);

    /** @brief Applies a global find/replace on the transcript text. */
    void requestReplaceAll(const QString& pattern,
                           const QString& replacement,
                           Qt::CaseSensitivity cs = Qt::CaseInsensitive);

    /** @brief Replaces occurrences of a substring in a given segment. */
    void requestReplaceInSegment(int index,
                                 const QString& from,
                                 const QString& to,
                                 Qt::CaseSensitivity cs = Qt::CaseInsensitive);

    /** @brief Normalizes whitespace across all segments. */
    void requestNormalizeWhitespaceAll();

    // ==== Import / export ====

    /**
     * @brief Imports a new transcript folder via TranscriptManager.
     *
     * On success, emits transcriptsReloaded(), importCompleted() and
     * currentTranscriptChanged().
     */
    bool requestImportTranscript(const QString& folderPath,
                                 const QStringList& speakerNames,
                                 QString* errorMessage = nullptr);

    /**
     * @brief Saves the currently selected transcript to disk.
     *
     * Uses TranscriptExporter to write editable.txt and meta.json.
     */
    void requestSaveCurrent(bool exportReference = false);

    /**
     * @brief Saves all loaded transcripts to disk.
     *
     * Currently calls TranscriptExporter for each transcript without
     * changing the current selection.
     */
    void requestSaveAll(bool exportReference = false);


    // ==== Audio ====

    /** @brief Toggles play/pause for the current audio. */
    void requestPlayPause();

    /** @brief Stops audio playback. */
    void requestStop();

    /** @brief Seeks audio playback to the given position (in ms). */
    void requestSeek(qint64 positionMs);

    /** @brief Jumps relative to the current position (e.g. ±5000 ms). */
    void requestJumpRelativeMs(qint64 deltaMs);

private Q_SLOTS:

    /** @brief Internal slot for QMediaPlayer::positionChanged. */
    void handleMediaPositionChanged(qint64 position);

    /** @brief Internal slot for QMediaPlayer::durationChanged. */
    void handleMediaDurationChanged(qint64 duration);

    /** @brief Internal slot for QMediaPlayer::playbackStateChanged. */
    void handleMediaPlaybackStateChanged(QMediaPlayer::PlaybackState state);

private:

    Model::Service::TranscriptManager m_manager;
    Model::Service::TranscriptEditor* m_editor = nullptr;
    Model::Service::TranscriptExporter m_exporter;

    int m_currentIndex = -1;

    QMediaPlayer* m_mediaPlayer = nullptr;
    QAudioOutput* m_audioOutput = nullptr;
    qint64 m_durationMs = 0;

    /** @brief Updates QMediaPlayer source for the current transcript. */
    void updateMediaForCurrentTranscript();

    /** @brief Recreates the editor for the currently selected transcript. */
    void recreateEditorForCurrentTranscript();

    /** @brief Emits undoRedoAvailabilityChanged based on editor state. */
    void emitUndoRedoAvailability();

};

}

#endif // CONTROLLER_APP_CONTROLLER_H
