#ifndef VIEW_WIDGETS_UTILITY_SEGMENT_ROW_WIDGET_H
#define VIEW_WIDGETS_UTILITY_SEGMENT_ROW_WIDGET_H

#include <QFrame>
#include <QString>
#include <QColor>
#include <QLabel>

namespace View {
namespace Widgets {
namespace Utility {

/**
 * @brief Simple row widget representing a transcript segment.
 *
 * Displays a bold colored speaker label on the left and the segment text on the right.
 * Emits clicked(segmentIndex) when the user clicks anywhere on the row.
 */

class SegmentRowWidget : public QFrame {

    Q_OBJECT

public:

    /**
     * @brief Constructs a row widget for a single segment.
     *
     * @param segmentIndex Index of the segment this row represents.
     * @param speakerText  Display text for the speaker (e.g. displayName).
     * @param segmentText  Full segment text to show in the row.
     * @param speakerColor Color used to render the speaker label.
     * @param parent       Parent widget.
     */
    explicit SegmentRowWidget(int segmentIndex,
                              const QString& speakerText,
                              const QString& segmentText,
                              const QColor& speakerColor,
                              int basePointSize,
                              QWidget* parent = nullptr);

    /** @brief Returns the segment index this row represents. */
    int segmentIndex() const;

    /** @brief Enables or disables a visual highlight background. */
    void setHighlighted(bool on);

    /** @brief Apply a new base font size to both labels. */
    void applyBaseFontSize(int basePointSize);

Q_SIGNALS:

    /** @brief Emitted when the user clicks on this row. */
    void clicked(int segmentIndex);

protected:

    /** @brief Handles mouse clicks to emit the clicked() signal. */
    void mousePressEvent(QMouseEvent* event) override;

private:

    int rowSegmentIndex = -1;
    QLabel* rowSpeakerLabel = nullptr;
    QLabel* rowTextLabel = nullptr;

};

}
}
}

#endif // VIEW_WIDGETS_UTILITY_SEGMENT_ROW_WIDGET_H
