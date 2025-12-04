#ifndef VIEW_UTILITY_TRANSCRIPT_VIEWER_WIDGET_H
#define VIEW_UTILITY_TRANSCRIPT_VIEWER_WIDGET_H

#include <QWidget>
#include <QHash>
#include <QSet>
#include <QColor>
#include <QVector>
#include <QScrollArea>
#include <QVBoxLayout>

namespace Model {
namespace Data {
class Transcript;
}
}

namespace View {
namespace Widgets {

/**
 * @brief Read-only viewer for a single transcript with color-coded speakers.
 *
 * Shows one row per segment with a colored speaker label and wrapped text.
 * The widget does not own the Transcript pointer; the caller is responsible
 * for ensuring its lifetime.
 */

class TranscriptViewerWidget : public QWidget {

    Q_OBJECT

public:

    /**
     * @brief Constructs an empty transcript viewer widget.
     *
     * Initially no transcript is displayed; call setTranscript() to show one.
     */
    explicit TranscriptViewerWidget(QWidget* parent = nullptr);
    ~TranscriptViewerWidget() override = default;


    /** @brief Returns the currently displayed transcript (may be nullptr). */
    const Model::Data::Transcript* transcript() const;


public Q_SLOTS:

    /** @brief Display the given transcript (no ownership taken). */
    void setTranscript(Model::Data::Transcript* transcript);

    /**
     * @brief Handles notification that a transcript's content has changed.
     *
     * Typically connected to Controller::AppController::transcriptContentChanged().
     * If the given transcript matches the currently displayed one, the view is
     * rebuilt.
     */
    void onTranscriptContentChanged(Model::Data::Transcript* transcript);



    /** @brief Scrolls to the given segment index if it is valid. */
    void scrollToSegment(int segmentIndex);

    /**
     * @brief Highlights a "current" segment (e.g. active search result or audio position).
     *
     * @param segmentIndex Segment index to highlight; -1 clears the current segment highlight.
     * @param scrollTo If true, the view will also scroll to make the segment visible.
     */
    void setCurrentSegmentIndex(int segmentIndex, bool scrollTo = true);



    /**
     * @brief Highlights a collection of segments (e.g. all search matches).
     *
     * Existing highlight set is replaced.
     */
    void setHighlightedSegments(const QVector<int>& segmentIndices);

    /** @brief Clears all search/extra highlighting. */
    void clearHighlights();


    /** @brief Increase base font size for all rows (up to a max). */
    void increaseFontSize();

    /** @brief Decrease base font size for all rows (down to a min). */
    void decreaseFontSize();

    /** @brief Reset font size to default. */
    void resetFontSize();


Q_SIGNALS:

    /** @brief Emitted when the user clicks on a segment row. */
    void segmentActivated(int segmentIndex);


private:

    /** @brief Rebuilds all segment rows from the current transcript. */
    void rebuildView();

    /** @brief Clears and deletes all current row widgets. */
    void clearRows();

    /** @brief Updates highlight state for all row widgets. */
    void updateRowHighlights();

    /** @brief Returns a color for the given speaker ID, caching the result. */
    QColor colorForSpeaker(const QString& speakerID) const;

private:

    const Model::Data::Transcript* viewerTranscript = nullptr;

    QScrollArea* viewerScrollArea = nullptr;
    QWidget* viewerContainer = nullptr;
    QVBoxLayout* viewerLayout = nullptr;

    // Map: segment index -> row widget (for scrolling/highlighting)
    QHash<int, QWidget*> rowWidgets;

    // Cache: speaker ID -> color
    mutable QHash<QString, QColor> speakerColors;

    // Highlight state
    int currentSegmentIndex = -1;
    QSet<int> highlightedSegments;

    // Font size
    int baseFontPointSize  = -1;
    int minFontPointSize = 9;
    int maxFontPointSize = 24;

};

}
}

#endif // VIEW_UTILITY_TRANSCRIPT_VIEWER_WIDGET_H
