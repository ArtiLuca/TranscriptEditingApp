#include "TranscriptViewerWidget.h"

#include "View/Widgets/Utility/SegmentRowWidget.h"
#include "Model/Data/Transcript.h"
#include "Model/Data/Segment.h"
#include "Model/Data/Speaker.h"

#include <QScrollArea>
#include <QVBoxLayout>
#include <QLayoutItem>
#include <QFrame>

using Model::Data::Transcript;
using Model::Data::Segment;
using Model::Data::Speaker;
using View::Widgets::Utility::SegmentRowWidget;

namespace View {
namespace Widgets {

TranscriptViewerWidget::TranscriptViewerWidget(QWidget* parent)
    : QWidget(parent),
    viewerTranscript(nullptr),
    viewerScrollArea(new QScrollArea(this)),
    viewerContainer(new QWidget(this)),
    viewerLayout(new QVBoxLayout(viewerContainer))
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0,0,0,0);
    rootLayout->setSpacing(0);

    viewerScrollArea->setWidgetResizable(true);
    viewerScrollArea->setFrameShape(QFrame::NoFrame);

    viewerLayout->setContentsMargins(8,8,8,8);
    viewerLayout->setSpacing(8);

    viewerScrollArea->setWidget(viewerContainer);
    rootLayout->addWidget(viewerScrollArea);

    // Base font size: slightly larger than app default
    QFont f = font();
    int pt = f.pointSize();
    if (pt <= 0)
        pt = 11;
    baseFontPointSize = pt + 1;
}


const Transcript* TranscriptViewerWidget::transcript() const {

    return viewerTranscript;
}

void TranscriptViewerWidget::setTranscript(Transcript* transcript) {

    if (viewerTranscript == transcript)
        return;

    viewerTranscript = transcript;
    currentSegmentIndex = -1;
    highlightedSegments.clear();
    speakerColors.clear();
    rebuildView();
}

void TranscriptViewerWidget::onTranscriptContentChanged(Transcript* transcript) {

    if (transcript != viewerTranscript)
        return;

    rebuildView();
}

void TranscriptViewerWidget::scrollToSegment(int segmentIndex) {

    if (!rowWidgets.contains(segmentIndex))
        return;

    QWidget* row = rowWidgets.value(segmentIndex);
    if (!row)
        return;

    viewerScrollArea->ensureWidgetVisible(row, 0, 50);
}

void TranscriptViewerWidget::setCurrentSegmentIndex(int segmentIndex, bool scrollTo) {

    if (currentSegmentIndex == segmentIndex)
        return;

    currentSegmentIndex = segmentIndex;
    updateRowHighlights();

    if (scrollTo && segmentIndex >= 0)
        scrollToSegment(segmentIndex);
}

void TranscriptViewerWidget::setHighlightedSegments(const QVector<int>& segmentIndices) {

    highlightedSegments.clear();
    for (int ind : segmentIndices) {
        highlightedSegments.insert(ind);
    }
    updateRowHighlights();
}

void TranscriptViewerWidget::clearHighlights() {

    highlightedSegments.clear();
    currentSegmentIndex = -1;
    updateRowHighlights();
}

void TranscriptViewerWidget::increaseFontSize() {

    if (baseFontPointSize < maxFontPointSize) {
        ++baseFontPointSize;
        for (auto it = rowWidgets.begin(); it != rowWidgets.end(); ++it) {
            if (auto* row = qobject_cast<SegmentRowWidget*>(it.value())) {
                row->applyBaseFontSize(baseFontPointSize);
            }
        }
    }
}

void TranscriptViewerWidget::decreaseFontSize() {

    if (baseFontPointSize > minFontPointSize) {
        --baseFontPointSize;
        for (auto it = rowWidgets.begin(); it != rowWidgets.end(); ++it) {
            if (auto* row = qobject_cast<SegmentRowWidget*>(it.value())) {
                row->applyBaseFontSize(baseFontPointSize);
            }
        }
    }
}

void TranscriptViewerWidget::resetFontSize() {

    QFont f = font();
    int pt = f.pointSize();
    if (pt <= 0)
        pt = 11;
    baseFontPointSize = pt + 1;

    for (auto it = rowWidgets.begin(); it != rowWidgets.end(); ++it) {
        if (auto* row = qobject_cast<SegmentRowWidget*>(it.value())) {
            row->applyBaseFontSize(baseFontPointSize);
        }
    }
}

void TranscriptViewerWidget::rebuildView() {

    setUpdatesEnabled(false);

    clearRows();

    if (!viewerTranscript) {
        setUpdatesEnabled(true);
        return;
    }

    rowWidgets.reserve(viewerTranscript->segments.size());

    for (int i = 0; i < viewerTranscript->segments.size(); ++i) {
        const Segment& seg = viewerTranscript->segments.at(i);

        // Resolve speaker display name
        QString speakerText = seg.speakerID;
        if (!seg.speakerID.isEmpty()) {
            const Speaker* sp = viewerTranscript->speakerFromID(seg.speakerID);
            if (sp && !sp->displayName.isEmpty())
                speakerText = sp->displayName;
        }

        const QColor speakerColor = colorForSpeaker(seg.speakerID);

        auto* row = new SegmentRowWidget(i, speakerText, seg.text,
            speakerColor, baseFontPointSize, viewerContainer);

        connect(row, &SegmentRowWidget::clicked,
                this, &TranscriptViewerWidget::segmentActivated);

        viewerLayout->addWidget(row);
        rowWidgets.insert(i, row);
    }

    // Add stretch so last row doesn't expand unnecessarily
    viewerLayout->addStretch(1);
    updateRowHighlights();

    setUpdatesEnabled(true);
}

void TranscriptViewerWidget::clearRows() {

    // Remove widgets from layout and delete them
    QLayoutItem* item = nullptr;
    while ((item = viewerLayout->takeAt(0)) != nullptr) {
        if (QWidget* w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }
    rowWidgets.clear();
}

void TranscriptViewerWidget::updateRowHighlights() {

    for (auto it = rowWidgets.begin(); it != rowWidgets.end(); ++it) {
        const int ind = it.key();
        auto* row = qobject_cast<SegmentRowWidget*>(it.value());
        if (!row)
            continue;

        const bool isCurrent = (ind == currentSegmentIndex);
        const bool isInSet = highlightedSegments.contains(ind);
        row->setHighlighted(isCurrent || isInSet);
    }
}

QColor TranscriptViewerWidget::colorForSpeaker(const QString& speakerID) const {

    if (speakerID.isEmpty())
        return QColor(Qt::darkGray);

    // Hard-coded override for specific speakers
    if (speakerID == QStringLiteral("Stephen")) {
        // nice medium blue
        return QColor(25, 118, 210);
    }

    // If we already have a cached color, return it
    auto it = speakerColors.find(speakerID);
    if (it != speakerColors.end())
        return it.value();

    QColor c;

    // Try to use speaker's own color if available
    if (viewerTranscript) {
        const Speaker* sp = viewerTranscript->speakerFromID(speakerID);
        if (sp && sp->color.isValid())
            c = sp->color;
    }

    // If still invalid, generate a deterministic pseudo-random color
    if (!c.isValid()) {
        const uint h = qHash(speakerID) % 360u;
        c = QColor::fromHsv(static_cast<int>(h), 160, 220);
    }

    speakerColors.insert(speakerID, c);
    return c;
}

}
}
