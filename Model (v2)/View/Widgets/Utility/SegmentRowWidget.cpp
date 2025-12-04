#include "SegmentRowWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPalette>
#include <QFont>

namespace View {
namespace Widgets {
namespace Utility {

SegmentRowWidget::SegmentRowWidget(int segmentIndex,
                                   const QString& speakerText,
                                   const QString& segmentText,
                                   const QColor& speakerColor,
                                   int basePointSize,
                                   QWidget* parent)
    : QFrame(parent),
    rowSegmentIndex(segmentIndex)
{
    setFrameStyle(QFrame::StyledPanel | QFrame::Plain);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(8);

    rowSpeakerLabel = new QLabel(speakerText, this);
    rowSpeakerLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    rowTextLabel = new QLabel(segmentText, this);
    rowTextLabel->setWordWrap(true);
    rowTextLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    applyBaseFontSize(basePointSize);

    QPalette spPal = rowSpeakerLabel->palette();
    spPal.setColor(QPalette::WindowText, speakerColor);
    rowSpeakerLabel->setPalette(spPal);

    layout->addWidget(rowSpeakerLabel);
    layout->addWidget(rowTextLabel, 1);

    setAutoFillBackground(true);
}


int SegmentRowWidget::segmentIndex() const {

    return rowSegmentIndex;
}

void SegmentRowWidget::setHighlighted(bool on) {

    QPalette pal = palette();
    if (on) {
        pal.setColor(QPalette::Window,QColor(QStringLiteral("#FFF9C4"))); // light yellow
    }
    else {
        // Transparent-ish background (inherit from parent)
        pal.setColor(QPalette::Window, Qt::transparent);
    }

    setPalette(pal);
}

void SegmentRowWidget::mousePressEvent(QMouseEvent* event) {

    if (event->button() == Qt::LeftButton) {
        emit clicked(rowSegmentIndex);
    }
    QFrame::mousePressEvent(event);
}

void SegmentRowWidget::applyBaseFontSize(int basePointSize) {

    QFont speakerFont = rowSpeakerLabel->font();
    QFont textFont = rowTextLabel->font();

    if (basePointSize > 0) {
        textFont.setPointSize(basePointSize);
        speakerFont.setPointSize(basePointSize + 1);
    }

    speakerFont.setBold(true);

    rowSpeakerLabel->setFont(speakerFont);
    rowTextLabel->setFont(textFont);
}

}
}
}
