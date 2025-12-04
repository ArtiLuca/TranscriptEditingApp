#ifndef VIEW_WIDGETS_UTILITY_EDITABLE_SEGMENT_ROW_WIDGET
#define VIEW_WIDGETS_UTILITY_EDITABLE_SEGMENT_ROW_WIDGET

#include <QFrame>
#include <QString>
#include <QStringList>
#include <QComboBox>
#include <QPlainTextEdit>
#include <QToolButton>
#include <QColor>

namespace View {
namespace Widgets {
namespace Utility {


/**
 * @brief Editable row widget representing a transcript segment.
 *
 * Shows a speaker combo and multi-line text editor, plus small buttons for:
 * - Split at cursor,
 * - Insert a new empty segment below,
 * - Delete this segment.
 *
 * The row does not modify the model directly. Instead, it emits signals that
 * TranscriptEditorWidget connects to Controller::AppController.
 *
 * It can also:
 * - Visually highlight itself as the "current" row,
 * - Display a speaker-specific color on the speaker combo.
 */

class EditableSegmentRowWidget : public QFrame {

    Q_OBJECT

public:

    /**
     * @brief Construct an editable row for a single segment.
     *
     * @param segmentIndex Index of the segment this row represents.
     * @param speakers List of available speaker IDs/names for the combo box.
     * @param speakerID Speaker ID/name for this segment.
     * @param text Text content of the segment.
     * @param basePointSize Base font size to apply to controls.
     * @param parent Parent widget.
     */
    explicit EditableSegmentRowWidget(int segmentIndex,
                                      const QStringList& speakers,
                                      const QString& speakerID,
                                      const QString& text,
                                      int basePointSize,
                                      QWidget* parent = nullptr);

    /** @brief Returns the segment index represented by this row. */
    int segmentIndex() const;

    /** @brief Sets the segment index (used after reordering). */
    void setSegmentIndex(int index);

    /** @brief Returns the current speaker ID/name. */
    QString speakerID() const;

    /** @brief Sets the current speaker combo to the given ID/name. */
    void setSpeakerID(const QString& id);

    /** @brief Returns the current text from the editor. */
    QString text() const;

    /** @brief Sets the text in the editor. */
    void setText(const QString& text);

    /** @brief Apply a new base font size to the speaker combo and text edit. */
    void applyBaseFontSize(int basePointSize);

    /** @brief Returns the current cursor position inside the text editor. */
    int cursorPosition() const;

    /** @brief Set the color associated with this row's speaker (for the combo). */
    void setSpeakerColor(const QColor& color);

    /** @brief Mark this row as the active/current one, updating its background. */
    void setActive(bool active);

Q_SIGNALS:

    /** @brief Emitted when the text changes. */
    void textEdited(int segmentIndex, const QString& newText);

    /** @brief Emitted when the speaker selection changes. */
    void speakerChanged(int segmentIndex, const QString& newSpeakerID);

    /** @brief User requested a split at the current cursor position. */
    void splitRequested(int segmentIndex, int cursorPosition);

    /** @brief User requested to delete this segment. */
    void deleteRequested(int segmentIndex);

    /** @brief User requested to insert a new segment below this one. */
    void insertBelowRequested(int segmentIndex);

    /** @brief Emitted when the row is clicked (used to mark it as current). */
    void rowClicked(int segmentIndex);


    // TO REMOVE
    /** @brief User requested to merge this segment with the next one. */
    //void mergeWithNextRequested(int segmentIndex);

protected:

    void mousePressEvent(QMouseEvent* event) override;

private Q_SLOTS:

    void handleTextChanged();
    void handleSpeakerChanged(int index);
    void handleSplitClicked();
    void handleDeleteClicked();
    void handleInsertBelowClicked();

    // TO REMOVE
    //void handleMergeClicked();

private:

    /** @brief Refresh background + speaker combo color based on state. */
    void updateVisualState();

    void updateMinimumHeightForText();

    int rowSegmentIndex = -1;
    bool rowIsActive = false;
    QColor rowSpeakerColor;

    QComboBox* speakerCombo = nullptr;
    QPlainTextEdit* textEdit = nullptr;
    QToolButton* buttonSplit = nullptr;
    QToolButton* buttonInsertBelow = nullptr;
    QToolButton* buttonDelete = nullptr;

    // TO REMOVE
    // QToolButton* buttonMerge = nullptr;

};

}
}
}

#endif // VIEW_WIDGETS_UTILITY_EDITABLE_SEGMENT_ROW_WIDGET
