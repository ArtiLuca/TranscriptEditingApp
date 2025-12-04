#include "TranscriptEditorWidget.h"

#include "View/Widgets/Utility/EditableSegmentRowWidget.h"
#include "Controller/AppController.h"
#include "Model/Data/Transcript.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QComboBox>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QFont>
#include <QSet>

namespace View {
namespace Widgets {

using Model::Data::Transcript;
using View::Widgets::Utility::EditableSegmentRowWidget;

TranscriptEditorWidget::TranscriptEditorWidget(QWidget* parent)
    : QWidget(parent),
    controller(nullptr),
    editorTranscript(nullptr),
    editorScrollArea(new QScrollArea(this)),
    editorContainer(new QWidget(this)),
    editorLayout(new QVBoxLayout(editorContainer))
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0,0,0,0);
    rootLayout->setSpacing(0);

    editorScrollArea->setWidgetResizable(true);
    editorScrollArea->setFrameShape(QFrame::NoFrame);

    editorLayout->setContentsMargins(8,8,8,8);
    editorLayout->setSpacing(8);
    editorContainer->setLayout(editorLayout);

    editorScrollArea->setWidget(editorContainer);
    rootLayout->addWidget(editorScrollArea);

    // Base font from widget font
    QFont f = font();
    int pt = f.pointSize();
    if (pt <= 0)
        pt = 11;
    baseFontPointSize = pt + 1;
}


void TranscriptEditorWidget::setController(Controller::AppController* ctrl) {

    controller = ctrl;
}

const Transcript* TranscriptEditorWidget::transcript() const {

    return editorTranscript;
}

void TranscriptEditorWidget::setTranscript(Transcript* transcript) {

    if (editorTranscript == transcript)
        return;

    editorTranscript = transcript;
    reloadSpeakerList();
    rebuildView();
}

void TranscriptEditorWidget::onTranscriptContentChanged(Transcript* transcript) {

    if (transcript != editorTranscript)
        return;

    if (!editorTranscript)
        return;

    const int modelCount = editorTranscript->segments.size();
    const int rowCount = rows.size();

    // If segment count changed (split, merge, insert, delete),
    // rebuild the rows.
    if (modelCount != rowCount) {
        reloadSpeakerList();
        rebuildView();
        return;
    }

    // Same number of segments: most likely pure text edits or
    // things like replace/rename. Our row widgets already show
    // the user-edited text, so we avoid rebuilding to preserve
    // focus and cursor position.
    //
    // If you later add global speaker rename, you can refresh
    // speaker combos/colors here if needed.
    reloadSpeakerList();

    // Optionally, refresh colors in case speakers changed:
    for (int i = 0; i < modelCount; ++i) {
        auto it = rows.find(i);
        if (it == rows.end() || !it.value())
            continue;

        const QString speakerID = editorTranscript->segments.at(i).speakerID;
        const QColor c = colorForSpeaker(speakerID);
        it.value()->setSpeakerID(speakerID);
        it.value()->setSpeakerColor(c);
    }
}


void TranscriptEditorWidget::scrollToSegment(int segmentIndex) {

    auto it = rows.find(segmentIndex);
    if (it == rows.end())
        return;

    if (QWidget* w = it.value()) {
        editorScrollArea->ensureWidgetVisible(w);
    }
}

void TranscriptEditorWidget::setCurrentSegmentIndex(int segmentIndex, bool scrollTo) {

    if (!editorTranscript)
        return;

    const int maxIndex = editorTranscript->segments.size() - 1;
    if (segmentIndex < 0 || segmentIndex > maxIndex)
        return;

    currentSegmentIndex = segmentIndex;
    updateRowHighlights();

    if (scrollTo)
        scrollToSegment(segmentIndex);

    emit currentSegmentChanged(segmentIndex);
}


void TranscriptEditorWidget::requestUndo() {

    if (!controller)
        return;
    controller->requestUndo();
}

void TranscriptEditorWidget::requestRedo() {

    if (!controller)
        return;
    controller->requestRedo();
}


void TranscriptEditorWidget::requestNormalizeWhitespaceAll() {

    if (!controller)
        return;
    controller->requestNormalizeWhitespaceAll();
}

void TranscriptEditorWidget::requestReplaceInCurrentSegment(const QString& from,
                                                            const QString& to,
                                                            Qt::CaseSensitivity cs) {

    if (!controller || currentSegmentIndex < 0)
        return;

    controller->requestReplaceInSegment(currentSegmentIndex, from, to, cs);
}

void TranscriptEditorWidget::requestReplaceAll(const QString& from,
                                               const QString& to,
                                               Qt::CaseSensitivity cs) {

    if (!controller)
        return;
    controller->requestReplaceAll(from, to, cs);
}

void TranscriptEditorWidget::requestSplitCurrentSegmentSameSpeaker() {

    if (!controller || currentSegmentIndex < 0)
        return;

    auto it = rows.find(currentSegmentIndex);
    if (it == rows.end())
        return;

    EditableSegmentRowWidget* row = it.value();
    if (!row)
        return;

    const int cursorPos = row->cursorPosition();
    controller->requestSplitSegment(currentSegmentIndex, cursorPos);
}

void TranscriptEditorWidget::requestSplitCurrentSegmentWithSpeakers(const QString& speakerFirst,
                                                                    const QString& speakerSecond) {

    if (!controller || currentSegmentIndex < 0)
        return;

    auto it = rows.find(currentSegmentIndex);
    if (it == rows.end())
        return;

    EditableSegmentRowWidget* row = it.value();
    if (!row)
        return;

    const int cursorPos = row->cursorPosition();
    controller->requestSplitSegmentWithSpeakers(
        currentSegmentIndex, cursorPos, speakerFirst, speakerSecond);
}

void TranscriptEditorWidget::requestInsertSegmentAt(int index,
                                                    const QString& speakerID,
                                                    const QString& text) {

    if (!controller)
        return;
    controller->requestInsertSegment(index, speakerID, text);
}


// === Private helpers ===

void TranscriptEditorWidget::reloadSpeakerList() {

    speakers.clear();

    if (!editorTranscript)
        return;

    if (controller) {
        // Use controller helper – returns display names (which we also treat as IDs).
        speakers = controller->currentTranscriptSpeakers();
    }

    // If you ever want to fall back to Transcript::speakers directly, you can
    // add code here to introspect the model (id/displayName, etc.).
}

void TranscriptEditorWidget::updateRowHighlights() {

    for (auto it = rows.begin(); it != rows.end(); ++it) {
        const int idx = it.key();
        EditableSegmentRowWidget* row = it.value();
        if (!row)
            continue;
        row->setActive(idx == currentSegmentIndex);
    }
}

QColor TranscriptEditorWidget::colorForSpeaker(const QString& speakerID) const {

    if (speakerID.isEmpty())
        return QColor(Qt::darkGray);

    // Cached?
    auto it = speakerColors.find(speakerID);
    if (it != speakerColors.end())
        return it.value();

    QColor c;

    // If you later want to respect model-defined colors, you can query
    // editorTranscript->speakerFromID(speakerID) here, like the viewer does.

    // For now we use the same deterministic HSV as TranscriptViewerWidget:
    const uint h = qHash(speakerID) % 360u;
    c = QColor::fromHsv(static_cast<int>(h), 160, 220);

    // Example hard-coded override (same as you did for the viewer):
    // if (speakerID == QStringLiteral("Stephen"))
    //     c = QColor(70, 130, 180); // a soft steel-blue

    speakerColors.insert(speakerID, c);
    return c;
}

void TranscriptEditorWidget::clearRows() {

    QHash<int, EditableSegmentRowWidget*>::iterator it = rows.begin();
    while (it != rows.end()) {
        EditableSegmentRowWidget* row = it.value();
        if (row)
            row->deleteLater();
        ++it;
    }
    rows.clear();

    QLayoutItem* child = nullptr;
    while ((child = editorLayout->takeAt(0)) != nullptr) {
        if (child->widget())
            child->widget()->deleteLater();
        delete child;
    }
}

void TranscriptEditorWidget::rebuildView() {

    clearRows();

    if (!editorTranscript || !editorLayout)
        return;

    const int count = editorTranscript->segments.size();

    for (int i = 0; i < count; ++i) {
        const auto& seg = editorTranscript->segments.at(i);
        const QString speakerID = seg.speakerID;

        auto* row = new EditableSegmentRowWidget(
            i,
            speakers,
            speakerID,
            seg.text,
            baseFontPointSize,
            editorContainer);

        const QColor speakerColor = colorForSpeaker(speakerID);
        row->setSpeakerColor(speakerColor);

        connect(row, &EditableSegmentRowWidget::textEdited,
                this, &TranscriptEditorWidget::handleRowTextEdited);
        connect(row, &EditableSegmentRowWidget::speakerChanged,
                this, &TranscriptEditorWidget::handleRowSpeakerChanged);
        connect(row, &EditableSegmentRowWidget::splitRequested,
                this, &TranscriptEditorWidget::handleRowSplitRequested);
        connect(row, &EditableSegmentRowWidget::deleteRequested,
                this, &TranscriptEditorWidget::handleRowDeleteRequested);
        connect(row, &EditableSegmentRowWidget::insertBelowRequested,
                this, &TranscriptEditorWidget::handleRowInsertBelowRequested);
        connect(row, &EditableSegmentRowWidget::rowClicked,
                this, &TranscriptEditorWidget::handleRowClicked);

        editorLayout->addWidget(row);
        rows.insert(i, row);
    }

    editorLayout->addStretch(1);
    updateRowHighlights();
}


// === Row-level slots ===

void TranscriptEditorWidget::handleRowTextEdited(int segmentIndex, const QString& newText) {

    if (!controller)
        return;

    setCurrentSegmentIndex(segmentIndex, false);
    controller->requestSetSegmentText(segmentIndex, newText);
}

void TranscriptEditorWidget::handleRowSpeakerChanged(int segmentIndex, const QString& newSpeakerID) {

    if (!controller)
        return;

    setCurrentSegmentIndex(segmentIndex, false);
    controller->requestChangeSegmentSpeaker(segmentIndex, newSpeakerID);

    // Update row color to match new speaker
    auto it = rows.find(segmentIndex);
    if (it != rows.end() && it.value()) {
        const QColor c = colorForSpeaker(newSpeakerID);
        it.value()->setSpeakerColor(c);
    }
}

void TranscriptEditorWidget::handleRowSplitRequested(int segmentIndex, int cursorPosition)
{
    if (!controller || !editorTranscript)
        return;

    // Tiny dialog to pick speakers
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Split segment into two speakers"));

    QVBoxLayout* vbox = new QVBoxLayout(&dialog);
    QLabel* label = new QLabel(tr("Choose speakers for the first and second part:"), &dialog);
    vbox->addWidget(label);

    QComboBox* firstCombo = new QComboBox(&dialog);
    QComboBox* secondCombo = new QComboBox(&dialog);
    firstCombo->addItems(speakers);
    secondCombo->addItems(speakers);

    // Preselect current row's speaker
    QString currentSpeaker;
    auto itRow = rows.find(segmentIndex);
    if (itRow != rows.end() && itRow.value())
        currentSpeaker = itRow.value()->speakerID();

    int idx1 = firstCombo->findText(currentSpeaker);
    if (idx1 >= 0) firstCombo->setCurrentIndex(idx1);
    int idx2 = secondCombo->findText(currentSpeaker);
    if (idx2 >= 0) secondCombo->setCurrentIndex(idx2);

    QFormLayout* form = new QFormLayout();
    form->addRow(tr("First part:"), firstCombo);
    form->addRow(tr("Second part:"), secondCombo);
    vbox->addLayout(form);

    QDialogButtonBox* buttons =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    vbox->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted)
        return;

    const QString speakerFirst  = firstCombo->currentText();
    const QString speakerSecond = secondCombo->currentText();

    setCurrentSegmentIndex(segmentIndex, false);

    // Use a new helper on AppController (see below)
    controller->requestSplitSegmentWithSpeakers(
        segmentIndex, cursorPosition, speakerFirst, speakerSecond);
}

void TranscriptEditorWidget::handleRowDeleteRequested(int segmentIndex) {

    if (!controller)
        return;
    controller->requestDeleteSegment(segmentIndex);
    // After deletion, segment indices shift; we'll get a contentChanged signal
    // and rebuild, so we don't try to manually adjust row indices here.
}

void TranscriptEditorWidget::handleRowInsertBelowRequested(int segmentIndex)
{
    if (!controller)
        return;

    QString speakerID;
    auto it = rows.find(segmentIndex);
    if (it != rows.end() && it.value())
        speakerID = it.value()->speakerID();

    const int newIndex = segmentIndex + 1;

    controller->requestInsertSegment(newIndex, speakerID, QString());
    setCurrentSegmentIndex(newIndex, true);
}

void TranscriptEditorWidget::handleRowClicked(int segmentIndex) {

    setCurrentSegmentIndex(segmentIndex, false);
}

void TranscriptEditorWidget::requestMergeCurrentWithNext()
{
    if (!controller || !editorTranscript)
        return;

    if (currentSegmentIndex < 0 ||
        currentSegmentIndex >= editorTranscript->segments.size() - 1)
        return; // no "next" to merge

    controller->requestMergeWithNext(currentSegmentIndex);
    // After contentChanged, rebuildView() runs and highlight will remain on the same index.
}


void TranscriptEditorWidget::increaseFontSize() {

    if (baseFontPointSize >= maxFontPointSize)
        return;

    ++baseFontPointSize;
    for (auto it = rows.begin(); it != rows.end(); ++it) {
        if (auto* row = it.value())
            row->applyBaseFontSize(baseFontPointSize);
    }
}

void TranscriptEditorWidget::decreaseFontSize() {

    if (baseFontPointSize <= minFontPointSize)
        return;

    --baseFontPointSize;
    for (auto it = rows.begin(); it != rows.end(); ++it) {
        if (auto* row = it.value())
            row->applyBaseFontSize(baseFontPointSize);
    }
}

void TranscriptEditorWidget::resetFontSize() {

    QFont f = font();
    int pt = f.pointSize();
    if (pt <= 0)
        pt = 11;
    baseFontPointSize = pt + 1;

    for (auto it = rows.begin(); it != rows.end(); ++it) {
        if (auto* row = it.value())
            row->applyBaseFontSize(baseFontPointSize);
    }
}

}
}
