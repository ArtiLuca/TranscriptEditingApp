#include "EditableSegmentRowWidget.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QFont>
#include <QPalette>

namespace View {
namespace Widgets {
namespace Utility {

EditableSegmentRowWidget::EditableSegmentRowWidget(int segmentIndex,
                                                   const QStringList& speakers,
                                                   const QString& speakerID,
                                                   const QString& text,
                                                   int basePointSize,
                                                   QWidget* parent)
    : QFrame(parent),
    rowSegmentIndex(segmentIndex)
{
    setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
    setAutoFillBackground(true);

    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(8,4,8,4);
    mainLayout->setSpacing(8);

    // Left column: speaker selector + row-level buttons
    auto* leftColumn = new QVBoxLayout();
    leftColumn->setContentsMargins(0,0,0,0);
    leftColumn->setSpacing(4);

    speakerCombo = new QComboBox(this);
    speakerCombo->addItems(speakers);
    int ind = speakerCombo->findText(speakerID);
    if (ind >= 0)
        speakerCombo->setCurrentIndex(ind);
    else if (!speakerID.isEmpty()) {
        speakerCombo->addItem(speakerID);
        speakerCombo->setCurrentIndex(speakerCombo->count() - 1);
    }
    leftColumn->addWidget(speakerCombo);

    // Button column
    auto* buttonColumn = new QHBoxLayout();
    buttonColumn->setContentsMargins(0,0,0,0);
    buttonColumn->setSpacing(2);

    //buttonMerge    = new QToolButton(this);

    buttonSplit    = new QToolButton(this);
    buttonInsertBelow = new QToolButton(this);
    buttonDelete   = new QToolButton(this);

    buttonSplit->setText(QStringLiteral("Split"));
    buttonSplit->setToolTip(tr("Split this segment at the cursor position"));

    buttonInsertBelow->setText(QStringLiteral("+"));
    buttonInsertBelow->setToolTip(tr("Insert a new empty segment below"));

    buttonDelete->setText(QStringLiteral("✕"));
    buttonDelete->setToolTip(tr("Delete this segment"));

    // buttonMerge->setText(QStringLiteral("Merge"));
    // buttonColumn->addWidget(buttonMerge);

    buttonColumn->addWidget(buttonSplit);
    buttonColumn->addWidget(buttonInsertBelow);
    buttonColumn->addWidget(buttonDelete);

    leftColumn->addLayout(buttonColumn);

    // Right: text editor
    textEdit = new QPlainTextEdit(this);
    textEdit->setPlainText(text);
    applyBaseFontSize(basePointSize);
    updateMinimumHeightForText();

    mainLayout->addLayout(leftColumn);
    mainLayout->addWidget(textEdit, 1);

    // Connections
    connect(textEdit, &QPlainTextEdit::textChanged,
            this, &EditableSegmentRowWidget::handleTextChanged);
    connect(speakerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &EditableSegmentRowWidget::handleSpeakerChanged);

    connect(buttonSplit, &QToolButton::clicked, this, &EditableSegmentRowWidget::handleSplitClicked);
    connect(buttonInsertBelow, &QToolButton::clicked, this, &EditableSegmentRowWidget::handleInsertBelowClicked);
    connect(buttonDelete, &QToolButton::clicked, this, &EditableSegmentRowWidget::handleDeleteClicked);


    // connect(buttonMerge, &QToolButton::clicked, this, &EditableSegmentRowWidget::handleMergeClicked);

    updateVisualState();
}

int EditableSegmentRowWidget::segmentIndex() const {

    return rowSegmentIndex;
}

void EditableSegmentRowWidget::setSegmentIndex(int index) {

    rowSegmentIndex = index;
}

QString EditableSegmentRowWidget::speakerID() const {

    return speakerCombo ? speakerCombo->currentText() : QString();
}

void EditableSegmentRowWidget::setSpeakerID(const QString& id) {

    if (!speakerCombo)
        return;

    int ind = speakerCombo->findText(id);
    if (ind >= 0) {
        speakerCombo->setCurrentIndex(ind);
    }
    else if (!id.isEmpty()) {
        speakerCombo->addItem(id);
        speakerCombo->setCurrentIndex(speakerCombo->count() - 1);
    }
}

QString EditableSegmentRowWidget::text() const {

    return textEdit ? textEdit->toPlainText() : QString();
}

void EditableSegmentRowWidget::setText(const QString& t) {

    if (textEdit)
        textEdit->setPlainText(t);
}

void EditableSegmentRowWidget::applyBaseFontSize(int basePointSize) {

    if (basePointSize <= 0)
        return;

    if (textEdit) {
        QFont f = textEdit->font();
        f.setPointSize(basePointSize);
        textEdit->setFont(f);
    }

    if (speakerCombo) {
        QFont f = speakerCombo->font();
        f.setPointSize(basePointSize);
        f.setBold(true);
        speakerCombo->setFont(f);
    }
}

int EditableSegmentRowWidget::cursorPosition() const {

    if (!textEdit)
        return 0;

    return textEdit->textCursor().position();
}

void EditableSegmentRowWidget::setSpeakerColor(const QColor& color) {

    rowSpeakerColor = color;
    updateVisualState();
}

void EditableSegmentRowWidget::setActive(bool active) {

    if (rowIsActive == active)
        return;

    rowIsActive = active;
    updateVisualState();
}

void EditableSegmentRowWidget::updateVisualState() {

    // Background highlight for active row
    QPalette pal = palette();
    QColor baseBg = palette().color(QPalette::Window);
    QColor activeBg(255, 252, 220); // light warm highlight

    pal.setColor(QPalette::Window, rowIsActive ? activeBg : baseBg);
    setPalette(pal);

    // Speaker color as combo background (if provided)
    if (speakerCombo) {
        if (rowSpeakerColor.isValid()) {
            const QString style =
                QStringLiteral("QComboBox { background-color: %1; }").arg(rowSpeakerColor.name());
            speakerCombo->setStyleSheet(style);
        }
        else {
            speakerCombo->setStyleSheet(QString());
        }
    }
}

void EditableSegmentRowWidget::updateMinimumHeightForText() {

    if (!textEdit)
        return;

    QFontMetrics fm(textEdit->font());
    const int lineHeight = fm.lineSpacing();

    // Rough estimate of line count:
    const int lineCount = textEdit->document()->blockCount();

    // Keep between 3 and 8 visible lines per row
    const int minLines = 3;
    const int maxLines = 8;
    const int lines = qBound(minLines, lineCount, maxLines);

    const int height = lines * lineHeight + 12; // padding
    textEdit->setMinimumHeight(height);
}

void EditableSegmentRowWidget::mousePressEvent(QMouseEvent* event) {

    QFrame::mousePressEvent(event);
    emit rowClicked(rowSegmentIndex);
}



void EditableSegmentRowWidget::handleTextChanged() {

    updateMinimumHeightForText();
    emit textEdited(rowSegmentIndex, text());
}

void EditableSegmentRowWidget::handleSpeakerChanged(int) {

    emit speakerChanged(rowSegmentIndex, speakerID());
}

void EditableSegmentRowWidget::handleSplitClicked() {

    const int pos = cursorPosition();
    emit splitRequested(rowSegmentIndex, pos);
}


void EditableSegmentRowWidget::handleDeleteClicked() {

    emit deleteRequested(rowSegmentIndex);
}

void EditableSegmentRowWidget::handleInsertBelowClicked() {

    emit insertBelowRequested(rowSegmentIndex);
}


// void EditableSegmentRowWidget::handleMergeClicked() {

//     emit mergeWithNextRequested(rowSegmentIndex);
// }


}
}
}
