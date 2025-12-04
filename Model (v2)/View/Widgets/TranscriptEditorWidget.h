#ifndef VIEW_WIDGETS_TRANSCRIPT_EDITOR_WIDGET
#define VIEW_WIDGETS_TRANSCRIPT_EDITOR_WIDGET

#include "Utility/EditableSegmentRowWidget.h"

#include <QWidget>
#include <QHash>
#include <QVector>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QColor>
#include <QStringList>

namespace Controller {
class AppController;
}

namespace Model {
namespace Data {
class Transcript;
}
}


namespace View {
namespace Widgets {

/**
 * @brief Editable view for a single transcript (one editable row per segment).
 *
 * This widget mirrors TranscriptViewerWidget but allows full editing of
 * segment text and speakers. It delegates editing operations to
 * Controller::AppController.
 */

class TranscriptEditorWidget : public QWidget {

    Q_OBJECT

public:

    /** @brief Construct an empty editor with no attached controller/transcript. */
    explicit TranscriptEditorWidget(QWidget* parent = nullptr);
    ~TranscriptEditorWidget() override = default;

    /** @brief Attach the central AppController used for editing operations. */
    void setController(Controller::AppController* ctrl);

    /** @brief Returns the currently edited transcript (may be nullptr). */
    const Model::Data::Transcript* transcript() const;

public Q_SLOTS:

    /** @brief Set the transcript to edit (no ownership taken). */
    void setTranscript(Model::Data::Transcript* transcript);

    /** @brief Rebuild the rows if the given transcript is the one being edited. */
    void onTranscriptContentChanged(Model::Data::Transcript* transcript);

    /** @brief Scroll to the given segment index. */
    void scrollToSegment(int segmentIndex);

    /** @brief Set the current segment index (optionally scroll to it). */
    void setCurrentSegmentIndex(int segmentIndex, bool scrollTo = true);

    // High-level editing actions (typically triggered by toolbar/menu):

    /** @brief Perform undo via AppController. */
    void requestUndo();

    /** @brief Perform redo via AppController. */
    void requestRedo();

    /** @brief Normalize whitespace across all segments. */
    void requestNormalizeWhitespaceAll();

    /** @brief Replace text in the current segment only. */
    void requestReplaceInCurrentSegment(const QString& from,
                                        const QString& to,
                                        Qt::CaseSensitivity cs = Qt::CaseInsensitive);

    /** @brief Replace text across all segments. */
    void requestReplaceAll(const QString& from,
                           const QString& to,
                           Qt::CaseSensitivity cs = Qt::CaseInsensitive);

    /**
     * @brief Split current segment at cursor with same speaker.
     *
     * Convenience wrapper if you later want a toolbar button using current row.
     */
    void requestSplitCurrentSegmentSameSpeaker();

    /**
     * @brief Split current segment at cursor, assigning two speakers.
     *
     * Caller provides both speakers; widget delegates to AppController.
     */
    void requestSplitCurrentSegmentWithSpeakers(const QString& speakerFirst,
                                                const QString& speakerSecond);

    /** @brief Insert a new segment at the given index. */
    void requestInsertSegmentAt(int index,
                                const QString& speakerID,
                                const QString& text);

    /** @brief Increase base font size used by all row widgets. */
    void increaseFontSize();
    /** @brief Decrease base font size used by all row widgets. */
    void decreaseFontSize();
    /** @brief Reset base font size to a value derived from the widget font. */
    void resetFontSize();

    /** @brief Merge the current segment with the next one (speaker of first kept). */
    void requestMergeCurrentWithNext();

Q_SIGNALS:

    /** @brief Emitted when the currently focused/selected segment changes. */
    void currentSegmentChanged(int segmentIndex);

    void currentSegmentChanged(int index, int total);

private Q_SLOTS:

    // Row-level slots wired to EditableSegmentRowWidget signals

    /** @brief Handle row text edits and forward them to the AppController. */
    void handleRowTextEdited(int segmentIndex, const QString& newText);
    /** @brief Handle row speaker changes and forward them to the AppController. */
    void handleRowSpeakerChanged(int segmentIndex, const QString& newSpeakerID);
    /** @brief Handle a split request coming from a row widget. */
    void handleRowSplitRequested(int segmentIndex, int cursorPosition);
    /** @brief Handle a delete request coming from a row widget. */
    void handleRowDeleteRequested(int segmentIndex);

    void handleRowInsertBelowRequested(int segmentIndex);
    void handleRowClicked(int segmentIndex);

    /** @brief Handle a merge-with-next request coming from a row widget. */
    //void handleRowMergeWithNextRequested(int segmentIndex);

private:

    /** @brief Clear and rebuild all row widgets from the current transcript. */
    void rebuildView();
    /** @brief Delete all existing row widgets and clear the layout. */
    void clearRows();
    /** @brief Reload the list of available speakers from the controller. */
    void reloadSpeakerList();
    /** @brief Update which row is visually highlighted as current. */
    void updateRowHighlights();
    /** @brief Compute a color for the given speaker ID, with caching. */
    QColor colorForSpeaker(const QString& speakerID) const;


    Controller::AppController* controller = nullptr;
    const Model::Data::Transcript* editorTranscript = nullptr;

    QScrollArea* editorScrollArea = nullptr;
    QWidget* editorContainer = nullptr;
    QVBoxLayout* editorLayout = nullptr;

    QHash<int, Utility::EditableSegmentRowWidget*> rows;

    QStringList speakers;
    int currentSegmentIndex = -1;

    int baseFontPointSize = 0;
    int minFontPointSize = 9;
    int maxFontPointSize = 24;

    mutable QHash<QString, QColor> speakerColors;

};

}
}





#endif // VIEW_WIDGETS_TRANSCRIPT_EDITOR_WIDGET
