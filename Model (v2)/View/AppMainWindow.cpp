#include "AppMainWindow.h"

#include "View/Widgets/TranscriptViewerWidget.h"
#include "View/Widgets/TranscriptEditorWidget.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QFileDialog>
#include <QInputDialog>
#include <QDialog>
#include <QDialogButtonBox>
#include <QListWidgetItem>
#include <QKeySequence>
#include <QMessageBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QRadioButton>
#include <QActionGroup>
#include <QIcon>
#include <QDir>

namespace View {

AppMainWindow::AppMainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("Transcript Editor"));
    resize(1200, 700);

    setupMenus();
    setupToolbars();
    setupPanels();
    setupStatusBar();
}


void AppMainWindow::setController(Controller::AppController* ctrl) {

    controller = ctrl;
    if (!controller)
        return;

    connectControllerSignals();

    // Try to refresh UI from controller state
    refreshTranscriptList();
    syncCurrentIndexSpin();
    updateWindowTitleForCurrentTranscript();

    if (transcriptEditor)
        transcriptEditor->setController(controller);
}


// === Setup ===

void AppMainWindow::setupMenus() {

    fileMenu = menuBar()->addMenu(tr("&File"));
    editMenu = menuBar()->addMenu(tr("&Edit"));
    audioMenu = menuBar()->addMenu(tr("&Audio"));
    viewMenu = menuBar()->addMenu(tr("&View"));

    actionChooseRootDirectory = new QAction(QIcon(":/icons/icons/actionSetRootDirectory.png"), "Choose &Directory", this);
    actionReload = new QAction(QIcon(":/icons/icons/actionReloadAll.png"), "&Reload All", this);
    actionImport = new QAction(QIcon(":/icons/icons/actionImport.png"), "&Import Transcript", this);
    actionSaveCurrent = new QAction(QIcon(":/icons/icons/actionSaveCurrent.png"), "&Save Transcript", this);
    actionSaveAll = new QAction(QIcon(":/icons/icons/actionSaveAll.png"), "Save &All Transcripts", this);
    actionExit = new QAction(QIcon(":/icons/icons/actionExit.png"), "E&xit", this);

    actionChooseRootDirectory->setToolTip(tr("Select root directory for transcript folders"));
    actionReload->setToolTip(tr("Reload all transcripts from root directory"));
    actionImport->setToolTip(tr("Import new transcript"));
    actionSaveCurrent->setToolTip(tr("Save current transcript to file"));
    actionSaveAll->setToolTip(tr("Save all transcripts to file"));
    actionExit->setToolTip(tr("Close application"));

    actionChooseRootDirectory->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_O));
    actionSaveCurrent->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_S));

    connect(actionChooseRootDirectory, &QAction::triggered, this, &AppMainWindow::onChooseRootDirectory);
    connect(actionReload, &QAction::triggered, this, &AppMainWindow::onReloadTranscripts);
    connect(actionImport, &QAction::triggered, this, &AppMainWindow::onImportTranscript);
    connect(actionSaveCurrent, &QAction::triggered, this, &AppMainWindow::onSaveCurrent);
    connect(actionSaveAll, &QAction::triggered, this, &AppMainWindow::onSaveAll);
    connect(actionExit, &QAction::triggered, this, &AppMainWindow::onExitRequested);

    fileMenu->addAction(actionChooseRootDirectory);
    fileMenu->addAction(actionReload);
    fileMenu->addSeparator();
    fileMenu->addAction(actionImport);
    fileMenu->addSeparator();
    fileMenu->addAction(actionSaveCurrent);
    fileMenu->addAction(actionSaveAll);
    fileMenu->addSeparator();

    // Edit menu actions
    actionUndo = new QAction(QIcon(":/icons/icons/actionUndo.png"), "&Undo", this);
    actionRedo = new QAction(QIcon(":/icons/icons/actionRedo.png"), "&Redo", this);
    actionUndo->setShortcut(QKeySequence::Undo);
    actionRedo->setShortcut(QKeySequence::Redo);
    actionUndo->setEnabled(false);
    actionRedo->setEnabled(false);

    actionFontSmaller = new QAction(QIcon(":/icons/icons/actionDecreaseFontSize.png"),"A-", this);
    actionFontLarger  = new QAction(QIcon(":/icons/icons/actionIncreaseFontSize.png"), "A+", this);
    actionFontReset   = new QAction(QIcon(":/icons/icons/actionResetFontSize.png"), "A", this);

    actionChangeSpeaker = new QAction(QIcon(":/icons/icons/actionChangeSpeaker.png"), "Change segment speaker", this);
    actionMergeWithNext = new QAction(QIcon(":/icons/icons/actionMergeWithNext.png"), "Merge current with &next segment", this);
    actionNormalizeWhitespace = new QAction(QIcon(":/icons/icons/actionNormalizeWhitespace.png"), "Normalize whitespace", this);
    actionReplaceText = new QAction(QIcon(":/icons/icons/actionReplaceText.png"), "Replace text", this);
    actionSplitSameSpeaker = new QAction(QIcon(":/icons/icons/actionSplitSegmentSameSpeaker.png"), "Split at cursor (same speaker)", this);
    actionSplitTwoSpeakers = new QAction(QIcon(":/icons/icons/actionSplitSegment.png"), "Split at cursor (two speakers)", this);
    actionInsertBelow = new QAction(QIcon(":/icons/icons/actionInsertBelow.png"), "Inser new segment below", this);
    actionDeleteSegment = new QAction(QIcon(":/icons/icons/actionRemoveSegment.png"), "Delete current segment", this);

    actionMergeWithNext->setEnabled(true); // editor itself will guard invalid cases

    actionUndo->setToolTip(tr("Undo last operation"));
    actionRedo->setToolTip(tr("Redo last operation"));
    actionFontSmaller->setToolTip(tr("Decrease font size"));
    actionFontLarger->setToolTip(tr("Increase font size"));
    actionFontReset->setToolTip(tr("Reset default font size"));

    actionChangeSpeaker->setToolTip(tr("Change current segment's speaker with another one present in transcript"));
    actionMergeWithNext->setToolTip(tr("Merge the currently selected segment with the one below it"));
    actionNormalizeWhitespace->setToolTip(tr("Normalizes whitespace in segment being edited"));
    actionReplaceText->setToolTip(tr("Replace text of segment being currently edited"));
    actionSplitSameSpeaker->setToolTip(tr("Split current segment at cursor, keeping same speaker"));
    actionSplitTwoSpeakers->setToolTip(tr("Split current segment at cursor into two speakers"));
    actionInsertBelow->setToolTip(tr("Insert an empty segment below the one being edited"));
    actionDeleteSegment->setToolTip(tr("Delete the segment being edited"));

    connect(actionUndo, &QAction::triggered, this, &AppMainWindow::onUndoRequested);
    connect(actionRedo, &QAction::triggered, this, &AppMainWindow::onRedoRequested);

    connect(actionFontSmaller, &QAction::triggered, this, &AppMainWindow::onFontSmaller);
    connect(actionFontLarger, &QAction::triggered, this, &AppMainWindow::onFontLarger);
    connect(actionFontReset, &QAction::triggered, this, &AppMainWindow::onFontReset);

    connect(actionChangeSpeaker, &QAction::triggered, this, &AppMainWindow::onChangeSegmentSpeakerTriggered);
    connect(actionMergeWithNext, &QAction::triggered, this, &AppMainWindow::onMergeWithNextTriggered);
    connect(actionNormalizeWhitespace, &QAction::triggered, this, &AppMainWindow::onNormalizeWhitespaceRequested);
    connect(actionReplaceText, &QAction::triggered, this, &AppMainWindow::onReplaceTextRequested);
    connect(actionSplitSameSpeaker, &QAction::triggered, this, &AppMainWindow::onSplitSameSpeakerTriggered);
    connect(actionSplitTwoSpeakers, &QAction::triggered, this, &AppMainWindow::onSplitTwoSpeakersTriggered);
    connect(actionInsertBelow, &QAction::triggered, this, &AppMainWindow::onInsertSegmentBelowTriggered);
    connect(actionDeleteSegment, &QAction::triggered, this, &AppMainWindow::onDeleteCurrentSegmentTriggered);

    editMenu->addAction(actionUndo);
    editMenu->addAction(actionRedo);
    editMenu->addSeparator();
    editMenu->addAction(actionFontSmaller);
    editMenu->addAction(actionFontLarger);
    editMenu->addAction(actionFontReset);
    editMenu->addSeparator();
    editMenu->addAction(actionMergeWithNext);
    editMenu->addAction(actionNormalizeWhitespace);
    editMenu->addAction(actionReplaceText);
    editMenu->addSeparator();
    editMenu->addAction(actionChangeSpeaker);
    editMenu->addAction(actionSplitSameSpeaker);
    editMenu->addAction(actionSplitTwoSpeakers);
    editMenu->addAction(actionInsertBelow);
    editMenu->addAction(actionDeleteSegment);


    // Audio menu actions
    actionPlayPause = new QAction(QIcon(":/icons/icons/actionPlayPause.png"), "Play/Pause", this);
    actionStop = new QAction(QIcon(":/icons/icons/actionStopAudio.png"), "Stop", this);
    actionSeekBack = new QAction(QIcon(":/icons/icons/actionSeekBack.png"), "<< 5s", this);
    actionSeekForward = new QAction(QIcon(":/icons/icons/actionSeekForward.png"), "5s >>", this);

    actionPlayPause->setToolTip(tr("Play/Pause audio (if available)"));
    actionStop->setToolTip(tr("Stop audio (if available)"));
    actionSeekBack->setToolTip(tr("Rewind audio by 5s (if available)"));
    actionSeekForward->setToolTip(tr("Fast-forward audio 5s (if available)"));

    connect(actionPlayPause, &QAction::triggered, this, &AppMainWindow::onPlayPauseRequested);
    connect(actionStop,      &QAction::triggered, this, &AppMainWindow::onStopRequested);
    connect(actionSeekBack,  &QAction::triggered, this, &AppMainWindow::onSeekBackward5s);
    connect(actionSeekForward,&QAction::triggered,this, &AppMainWindow::onSeekForward5s);

    audioMenu->addAction(actionPlayPause);
    audioMenu->addAction(actionStop);
    audioMenu->addSeparator();
    audioMenu->addAction(actionSeekBack);
    audioMenu->addAction(actionSeekForward);

    actionShowViewer = new QAction(QIcon(":/icons/icons/actionShowViewer.png"), "&View Transcript", this);
    actionShowEditor = new QAction(QIcon(":/icons/icons/actionShowEditor.png"), "Edit Transcript", this);
    actionShowViewer->setCheckable(true);
    actionShowEditor->setCheckable(true);

    auto* viewModeGroup = new QActionGroup(this);
    viewModeGroup->addAction(actionShowViewer);
    viewModeGroup->addAction(actionShowEditor);
    actionShowViewer->setChecked(true);
    actionShowEditor->setEnabled(false);

    connect(actionShowViewer, &QAction::triggered,
            this, &AppMainWindow::onShowViewerRequested);
    connect(actionShowEditor, &QAction::triggered,
            this, &AppMainWindow::onShowEditorRequested);

    viewMenu->addAction(actionShowViewer);
    viewMenu->addAction(actionShowEditor);

}

void AppMainWindow::setupToolbars() {

    // File toolbar
    fileToolbar = addToolBar(tr("File"));
    fileToolbar->addAction(actionChooseRootDirectory);
    fileToolbar->addAction(actionReload);
    fileToolbar->addSeparator();
    fileToolbar->addAction(actionImport);
    fileToolbar->addSeparator();
    fileToolbar->addAction(actionSaveCurrent);
    fileToolbar->addAction(actionSaveAll);

    // Edit toolbar
    editToolbar = addToolBar(tr("Edit"));
    editToolbar->addAction(actionUndo);
    editToolbar->addAction(actionRedo);
    editToolbar->addSeparator();
    editToolbar->addAction(actionFontSmaller);
    editToolbar->addAction(actionFontLarger);
    editToolbar->addAction(actionFontReset);
    editToolbar->addSeparator();
    editToolbar->addAction(actionMergeWithNext);
    editToolbar->addAction(actionNormalizeWhitespace);
    editToolbar->addAction(actionReplaceText);
    editToolbar->addSeparator();
    editToolbar->addAction(actionChangeSpeaker);
    editToolbar->addAction(actionSplitSameSpeaker);
    editToolbar->addAction(actionSplitTwoSpeakers);
    editToolbar->addAction(actionInsertBelow);
    editToolbar->addAction(actionDeleteSegment);

    // Audio toolbar
    audioToolbar = addToolBar(tr("Audio"));

    audioToolbar->addAction(actionPlayPause);
    audioToolbar->addAction(actionStop);
    audioToolbar->addSeparator();
    audioToolbar->addAction(actionSeekBack);
    audioToolbar->addAction(actionSeekForward);

    audioSlider = new QSlider(Qt::Horizontal, this);
    audioSlider->setRange(0, 0);
    audioSlider->setEnabled(false);

    connect(audioSlider, &QSlider::sliderMoved,
            this, &AppMainWindow::onAudioSliderMoved);

    audioToolbar->addSeparator();
    audioToolbar->addWidget(audioSlider);
}

void AppMainWindow::setupPanels() {

    mainSplitter = new QSplitter(this);
    setCentralWidget(mainSplitter);

    // Left Panel
    leftPanel = new QWidget(this);
    leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(4,4,4,4);
    leftLayout->setSpacing(4);

    rootDirectoryLabel = new QLabel(tr("Root: (not set)"), leftPanel);
    rootDirectoryLabel->setWordWrap(true);

    transcriptList = new QListWidget(leftPanel);
    transcriptList->setSelectionMode(QAbstractItemView::SingleSelection);

    connect(transcriptList, &QListWidget::itemClicked,
            this, &AppMainWindow::onTranscriptListItemClicked);

    QWidget* indexRow = new QWidget(leftPanel);
    auto* indexLayout = new QHBoxLayout(indexRow);
    indexLayout->setContentsMargins(0,0,0,0);
    indexLayout->addWidget(new QLabel(tr("Current index:"), indexRow));
    indexSpinBox = new QSpinBox(indexRow);
    indexSpinBox->setMinimum(0);
    indexSpinBox->setMaximum(0);
    indexSpinBox->setEnabled(false);

    // This line was missing, I think it was needed right?
    indexLayout->addWidget(indexSpinBox);

    connect(indexSpinBox, qOverload<int>(&QSpinBox::valueChanged),
            this, &AppMainWindow::onCurrentIndexSpinChanged);

    leftLayout->addWidget(rootDirectoryLabel);
    leftLayout->addWidget(transcriptList, 1);
    leftLayout->addWidget(indexRow);

    // Right Panel
    rightPanel = new QWidget(this);
    rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(4,4,4,4);
    rightLayout->setSpacing(4);

    rightStack = new QStackedWidget(rightPanel);

    // Placeholder page with label + text preview
    placeholderPage = new QWidget(rightStack);
    auto* placeholderLayout = new QVBoxLayout(placeholderPage);
    auto* placeholderLabel = new QLabel(
        tr("Please upload a transcript using Import.\n"),
        placeholderPage);
    placeholderLabel->setAlignment(Qt::AlignCenter);

    transcriptPreview = new QTextBrowser(placeholderPage);
    transcriptPreview->setReadOnly(true);
    transcriptPreview->setPlainText(QString());

    placeholderLayout->addWidget(placeholderLabel);
    placeholderLayout->addWidget(transcriptPreview, 1);

    rightStack->addWidget(placeholderPage);

    transcriptViewer = new Widgets::TranscriptViewerWidget(rightStack);
    rightStack->addWidget(transcriptViewer);

    transcriptEditor = new Widgets::TranscriptEditorWidget(rightStack);
    rightStack->addWidget(transcriptEditor);

    rightStack->setCurrentWidget(placeholderPage);
    currentRightPage = RightPage::Placeholder;

    rightLayout->addWidget(rightStack, 1);

    mainSplitter->addWidget(leftPanel);
    mainSplitter->addWidget(rightPanel);
    mainSplitter->setStretchFactor(0,0);
    mainSplitter->setStretchFactor(1,1);
    mainSplitter->setSizes({250,950});

}

void AppMainWindow::setupStatusBar() {

    statusBar = new QStatusBar(this);
    setStatusBar(statusBar);

    audioStatusLabel = new QLabel(tr("Audio: stopped"), this);
    statusBar->addPermanentWidget(audioStatusLabel);
}


void AppMainWindow::connectControllerSignals() {

    if (!controller)
        return;

    // Error and general feedback
    connect(controller, &Controller::AppController::errorOccurred,
            this, &AppMainWindow::onErrorOccurred);
    connect(controller, &Controller::AppController::transcriptsReloaded,
            this, &AppMainWindow::onTranscriptsReloaded);
    connect(controller, &Controller::AppController::currentTranscriptChanged,
            this, &AppMainWindow::onCurrentTranscriptChanged);
    connect(controller, &Controller::AppController::transcriptContentChanged,
            this, &AppMainWindow::onTranscriptContentChanged);
    connect(controller, &Controller::AppController::saveCompleted,
            this, &AppMainWindow::onSaveCompleted);
    connect(controller, &Controller::AppController::importCompleted,
            this, &AppMainWindow::onImportCompleted);

    // Undo/Redo
    connect(controller, &Controller::AppController::undoRedoAvailabilityChanged,
            this, &AppMainWindow::onUndoRedoAvailabilityChanged);

    // Audio
    connect(controller, &Controller::AppController::audioPositionChanged,
            this, &AppMainWindow::onAudioPositionChanged);
    connect(controller, &Controller::AppController::audioPlaybackStateChanged,
            this, &AppMainWindow::onAudioPlaybackStateChanged);

    // TranscriptViewerWidget
    if (transcriptViewer) {
        connect(controller, &Controller::AppController::currentTranscriptChanged,
                transcriptViewer, &Widgets::TranscriptViewerWidget::setTranscript);

        connect(controller, &Controller::AppController::transcriptContentChanged,
                transcriptViewer, &Widgets::TranscriptViewerWidget::onTranscriptContentChanged);
    }

    // TranscriptEditorWidget
    if (transcriptEditor) {
        connect(controller, &Controller::AppController::currentTranscriptChanged,
                transcriptEditor, &Widgets::TranscriptEditorWidget::setTranscript);
        connect(controller, &Controller::AppController::transcriptContentChanged,
                transcriptEditor, &Widgets::TranscriptEditorWidget::onTranscriptContentChanged);
    }

}


// === Slots: File / root directory ===

void AppMainWindow::onChooseRootDirectory() {

    const QString dir = QFileDialog::getExistingDirectory(
        this, tr("Select transcripts root directory folder"),
        QString(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (dir.isEmpty()) {
        if (statusBar) statusBar->showMessage(tr("Please select a valid directory for transcripts"), 4000);
        return;
    }

    if (!controller)
        return;

    controller->setRootDirectory(dir);
    rootDirectoryLabel->setText(tr("Root: %1").arg(dir));

    QString err;
    if (!controller->loadTranscripts(&err)) {
        QMessageBox::warning(this, tr("Error loading transcripts"), err);
        return;
    }
}

void AppMainWindow::onReloadTranscripts() {

    if (!controller) {
        if (statusBar) statusBar->showMessage(tr("Controller not yet initialized!"), 4000);
        return;
    }

    QString err;
    if (!controller->loadTranscripts(&err)) {
        QMessageBox::warning(this, tr("Error reloading transcripts"), err);
    }
}

void AppMainWindow::onImportTranscript() {

    if (!controller) {
        if (statusBar) statusBar->showMessage(tr("Controller not yet initialized!"), 4000);
        return;
    }

    const QString dir = QFileDialog::getExistingDirectory(
        this, tr("Select transcript folder to import"),
        QString(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (dir.isEmpty()) {
        if (statusBar) statusBar->showMessage(tr("Please select a valid directory for transcripts"), 4000);
        return;
    }

    // Simple dialog: ask for speakers as comma-separated list
    bool ok = false;
    const QString rawSpeakers = QInputDialog::getText(
        this, tr("Speakers"), tr("Enter speaker names separated by commas:"),
        QLineEdit::Normal, QStringLiteral("Stephen"), &ok);

    if (!ok || rawSpeakers.trimmed().isEmpty()) {
        if (statusBar) statusBar->showMessage(tr("Please input valid speaker name list"), 4000);
        return;
    }

    QStringList speakerNames;
    for (const QString& s : rawSpeakers.split(',', Qt::SkipEmptyParts)) {
        speakerNames << s.trimmed();
    }

    QString err;
    if (!controller->requestImportTranscript(dir, speakerNames, &err)) {
        QMessageBox::warning(this, tr("Error importing transcript"), err);
    }

}

void AppMainWindow::onSaveCurrent() {

    if (!controller) {
        if (statusBar) statusBar->showMessage(tr("Controller not yet initialized!"), 4000);
        return;
    }

    Model::Data::Transcript* t = controller->currentTranscript();
    if (!t) {
        if (statusBar) statusBar->showMessage(tr("No transcript selected to save."), 4000);
        return;
    }

    const QString title = t->title.isEmpty()
        ? tr("untitled") : t->title;

    const auto reply = QMessageBox::question(
        this, tr("Save transcript"),
        tr("Save current transcript:\n\"%1\" ?").arg(title),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

    if (reply != QMessageBox::Yes)
        return;

    controller->requestSaveCurrent(false);
}

void AppMainWindow::onSaveAll() {

    if (!controller) {
        if (statusBar) statusBar->showMessage(tr("Controller not yet initialized!"), 4000);
        return;
    }

    controller->requestSaveAll();
}

void AppMainWindow::onExitRequested() {

    close();
}

// === Selection & updates ===

void AppMainWindow::onTranscriptListItemClicked(QListWidgetItem* item) {

    if (!controller || !item) {
        if (statusBar) statusBar->showMessage(tr("Controller not yet initialized or no item clicked"), 4000);
        return;
    }

    const int index = item->data(Qt::UserRole).toInt();
    controller->selectTranscript(index);
}

void AppMainWindow::onCurrentIndexSpinChanged(int value) {

    if (!controller) {
        if (statusBar) statusBar->showMessage(tr("Controller not yet initialized or no item clicked"), 4000);
        return;
    }

    if (value < 0 || value >= controller->transcriptCount()) {
        if (statusBar) statusBar->showMessage(tr("Index %1 is out-of-bounds").arg(value));
        return;
    }

    controller->selectTranscript(value);
}

void AppMainWindow::onTranscriptsReloaded() {

    refreshTranscriptList();
    syncCurrentIndexSpin();
}

void AppMainWindow::onCurrentTranscriptChanged(Model::Data::Transcript* transcript) {

    syncCurrentIndexSpin();
    updateWindowTitleForCurrentTranscript();

    if (!rightStack)
        return;

    if (transcript) {
        // Enable editor mode now that we have something to edit
        if (actionShowEditor)
            actionShowEditor->setEnabled(true);

        // If user was in editor mode and it's still checked, stay there.
        if (currentRightPage == RightPage::Editor &&
            actionShowEditor && actionShowEditor->isChecked()) {
            showRightPage(RightPage::Editor);
        }
        else {
            showRightPage(RightPage::Viewer);
            if (actionShowViewer) actionShowViewer->setChecked(true);
            if (actionShowEditor) actionShowEditor->setChecked(false);
        }

        if (statusBar)
            statusBar->showMessage(tr("Loaded transcript: %1").arg(transcript->title), 4000);
    }
    else {
        // No transcript: back to placeholder and disable editor
        showRightPage(RightPage::Placeholder);
        if (actionShowViewer) actionShowViewer->setChecked(false);
        if (actionShowEditor) {
            actionShowEditor->setChecked(false);
            actionShowEditor->setEnabled(false);
        }
        if (statusBar)
            statusBar->showMessage(tr("No transcript selected"), 3000);
    }
}

void AppMainWindow::onTranscriptContentChanged(Model::Data::Transcript* transcript) {

    Q_UNUSED(transcript);
    // For now do nothing; later you’ll refresh the viewer/editor widgets
}


// === Editing ===

void AppMainWindow::onUndoRequested() {

    if (!controller) {
        if (statusBar) statusBar->showMessage(tr("Controller not yet initialized"), 4000);
        return;
    }

    controller->requestUndo();
}

void AppMainWindow::onRedoRequested() {

    if (!controller) {
        if (statusBar) statusBar->showMessage(tr("Controller not yet initialized"), 4000);
        return;
    }

    controller->requestRedo();
}


// === Audio ===

void AppMainWindow::onPlayPauseRequested() {

    if (!controller) {
        if (statusBar) statusBar->showMessage(tr("Controller not yet initialized"), 4000);
        return;
    }

    controller->requestPlayPause();
}

void AppMainWindow::onStopRequested() {

    if (!controller) {
        if (statusBar) statusBar->showMessage(tr("Controller not yet initialized"), 4000);
        return;
    }

    controller->requestStop();
}

void AppMainWindow::onSeekBackward5s() {

    if (!controller) {
        if (statusBar) statusBar->showMessage(tr("Controller not yet initialized"), 4000);
        return;
    }

    controller->requestJumpRelativeMs(-5000);
}

void AppMainWindow::onSeekForward5s() {

    if (!controller) {
        if (statusBar) statusBar->showMessage(tr("Controller not yet initialized"), 4000);
        return;
    }

    controller->requestJumpRelativeMs(5000);
}

void AppMainWindow::onAudioSliderMoved(int value) {

    if (!controller) {
        if (statusBar) statusBar->showMessage(tr("Controller not yet initialized"), 4000);
        return;
    }

    controller->requestSeek(static_cast<qint64>(value));

}


// === Controller feedback ===

void AppMainWindow::onErrorOccurred(const QString& message) {

    QMessageBox::warning(this, tr("Error"), message);
}

void AppMainWindow::onSaveCompleted(Model::Data::Transcript* transcript) {

    if (!transcript) {
        if (statusBar) statusBar->showMessage(tr("Could not save transcript (invalid/null"), 4000);
        return;
    }

    statusBar->showMessage(tr("Saved transcript: %1").arg(transcript->title), 4000);
}

void AppMainWindow::onImportCompleted(int newIndex, Model::Data::Transcript* transcript) {

    Q_UNUSED(transcript);
    refreshTranscriptList();
    syncCurrentIndexSpin();

    if (controller && newIndex >= 0 && newIndex < controller->transcriptCount()) {
        controller->selectTranscript(newIndex);
    }
}

void AppMainWindow::onUndoRedoAvailabilityChanged(bool canUndo, bool canRedo) {

    actionUndo->setEnabled(canUndo);
    actionRedo->setEnabled(canRedo);
}

void AppMainWindow::onAudioPositionChanged(qint64 positionMs, qint64 durationMs) {

    updateAudioStatus(positionMs, durationMs);

    if (!audioSlider)
        return;

    if (durationMs > 0) {
        audioSlider->setEnabled(true);
        audioSlider->setMaximum(static_cast<int>(durationMs));
        audioSlider->setValue(static_cast<int>(positionMs));
    }
    else {
        audioSlider->setEnabled(false);
        audioSlider->setMaximum(0);
        audioSlider->setValue(0);
    }
}

void AppMainWindow::onAudioPlaybackStateChanged(QMediaPlayer::PlaybackState state) {

    QString stateStr;
    switch (state) {
    case QMediaPlayer::PlayingState: stateStr = tr("playing"); break;
    case QMediaPlayer::PausedState:  stateStr = tr("paused");  break;
    case QMediaPlayer::StoppedState: stateStr = tr("stopped"); break;
    }
    audioStatusLabel->setText(tr("Audio: %1").arg(stateStr));
}


// === UI helpers ===

void AppMainWindow::refreshTranscriptList() {

    transcriptList->clear();

    if (!controller) {
        if (statusBar) statusBar->showMessage(tr("Controller not yet initialized"), 4000);
        return;
    }

    const int count = controller->transcriptCount();

    const QStringList titles = controller->transcriptTitles();
    for (int i = 0; i < titles.size(); ++i) {
        auto* item = new QListWidgetItem(titles.at(i), transcriptList);
        item->setData(Qt::UserRole, i);
    }
}

void AppMainWindow::syncSelectionToController() {

    // Placeholder for future use (e.g. syncing list/spinbox to controller)
}

void AppMainWindow::syncCurrentIndexSpin() {

    if (!controller) {
        if (statusBar) statusBar->showMessage(tr("Controller not yet initialized"), 4000);
        return;
    }

    const int count = controller->transcriptCount();
    indexSpinBox->setMinimum(count > 0 ? 0 : 0);
    indexSpinBox->setMaximum(count > 0 ? count - 1 : 0);
    indexSpinBox->setEnabled(count > 0);

    const int cur = controller->currentTranscriptIndex();
    if (cur >= 0 && cur < count)
        indexSpinBox->setValue(cur);
}

void AppMainWindow::updateWindowTitleForCurrentTranscript() {

    if (!controller) {
        setWindowTitle(tr("Transcript Editor"));
        return;
    }

    const auto* t = controller->currentTranscript();
    if (!t) {
        setWindowTitle(tr("Transcript Editor"));
        return;
    }

    setWindowTitle(tr("Transcript Editor - %1").arg(t->title));
}

void AppMainWindow::updateAudioStatus(qint64 positionMs, qint64 durationMs) {

    if (durationMs <= 0) {
        audioStatusLabel->setText(tr("Audio: 0:00 / 0:00"));
        return;
    }

    auto fmt = [](qint64 ms) -> QString {
        const qint64 totalSeconds = ms / 1000;
        const qint64 minutes = totalSeconds / 60;
        const qint64 seconds = totalSeconds % 60;
        return QStringLiteral("%1:%2")
            .arg(minutes, 2, 10, QLatin1Char('0'))
            .arg(seconds, 2, 10, QLatin1Char('0'));
    };

    audioStatusLabel->setText(tr("Audio: %1 / %2")
        .arg(fmt(positionMs), fmt(durationMs)));
}

void AppMainWindow::showRightPage(RightPage page) {

    if (!rightStack)
        return;

    if (page == currentRightPage)
        return;

    currentRightPage = page;


    switch (page) {
    case RightPage::Placeholder:
        rightStack->setCurrentWidget(placeholderPage);
        break;
    case RightPage::Viewer:
        if (transcriptViewer)
            rightStack->setCurrentWidget(transcriptViewer);
        break;
    case RightPage::Editor:
        if (transcriptEditor)
            rightStack->setCurrentWidget(transcriptEditor);
        break;
    }

    updateEditorActionsEnabled();
}

void AppMainWindow::updateEditorActionsEnabled() {

    const bool inEditor = (currentRightPage == RightPage::Editor);

    if (actionChangeSpeaker) actionChangeSpeaker->setEnabled(inEditor);
    if (actionMergeWithNext) actionMergeWithNext->setEnabled(inEditor);
    if (actionNormalizeWhitespace) actionNormalizeWhitespace->setEnabled(inEditor);
    if (actionReplaceText) actionReplaceText->setEnabled(inEditor);
    if (actionSplitSameSpeaker) actionSplitSameSpeaker->setEnabled(inEditor);
    if (actionSplitTwoSpeakers) actionSplitTwoSpeakers->setEnabled(inEditor);
    if (actionInsertBelow) actionInsertBelow->setEnabled(inEditor);
    if (actionDeleteSegment) actionDeleteSegment->setEnabled(inEditor);


}


// === Font actions ===

void AppMainWindow::onFontSmaller() {

    switch (currentRightPage) {
    case RightPage::Viewer:
        if (transcriptViewer)
            transcriptViewer->decreaseFontSize();
        break;
    case RightPage::Editor:
        if (transcriptEditor)
            transcriptEditor->decreaseFontSize();
        break;
    case RightPage::Placeholder:
        //...
        break;
    }
}

void AppMainWindow::onFontLarger() {

    switch (currentRightPage) {
    case RightPage::Viewer:
        if (transcriptViewer)
            transcriptViewer->increaseFontSize();
        break;
    case RightPage::Editor:
        if (transcriptEditor)
            transcriptEditor->increaseFontSize();
        break;
    case RightPage::Placeholder:
        //...
        break;
    }
}

void AppMainWindow::onFontReset() {

    switch (currentRightPage) {
    case RightPage::Viewer:
        if (transcriptViewer)
            transcriptViewer->resetFontSize();
        break;
    case RightPage::Editor:
        if (transcriptEditor)
            transcriptEditor->resetFontSize();
        break;
    case RightPage::Placeholder:
        //...
        break;
    }
}

// === Editor actions ===

void AppMainWindow::onMergeWithNextTriggered() {

    if (!transcriptEditor)
        return;
    transcriptEditor->requestMergeCurrentWithNext();
}

void AppMainWindow::onSplitSameSpeakerTriggered()
{

}

void AppMainWindow::onSplitTwoSpeakersTriggered()
{

}

void AppMainWindow::onInsertSegmentBelowTriggered()
{

}

void AppMainWindow::onDeleteCurrentSegmentTriggered()
{

}

void AppMainWindow::onChangeSegmentSpeakerTriggered()
{

}

void AppMainWindow::onNormalizeWhitespaceRequested() {

    if (!transcriptEditor || currentRightPage != RightPage::Editor) {
        if (statusBar)
            statusBar->showMessage(tr("Switch to the editor to normalize whitespace."), 4000);
        return;
    }
    transcriptEditor->requestNormalizeWhitespaceAll();

}

void AppMainWindow::onReplaceTextRequested() {

    if (!transcriptEditor || currentRightPage != RightPage::Editor) {
        if (statusBar)
            statusBar->showMessage(tr("Switch to the editor to use Replace."), 4000);
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(tr("Replace text"));

    QVBoxLayout* dialogLayout = new QVBoxLayout(&dialog);

    QLineEdit* fromEdit = new QLineEdit(&dialog);
    QLineEdit* toEdit = new QLineEdit(&dialog);
    QCheckBox* caseSensitiveBox = new QCheckBox(tr("Case sensitive"), &dialog);

    QRadioButton* currentSegmentRadio = new QRadioButton(tr("Current segment only"), &dialog);
    QRadioButton* allSegmentsRadio = new QRadioButton(tr("All segments"), &dialog);
    allSegmentsRadio->setChecked(true);

    QFormLayout* dialogForm = new QFormLayout();
    dialogForm->addRow(tr("Find:"), fromEdit);
    dialogForm->addRow(tr("Replace with:"), toEdit);

    dialogLayout->addLayout(dialogForm);
    dialogLayout->addWidget(caseSensitiveBox);
    dialogLayout->addWidget(currentSegmentRadio);
    dialogLayout->addWidget(allSegmentsRadio);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    dialogLayout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted)
        return;

    const QString from = fromEdit->text();
    const QString to = toEdit->text();

    if (from.isEmpty())
        return;

    const Qt::CaseSensitivity cs = caseSensitiveBox->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;

    if (currentSegmentRadio->isChecked()) {
        transcriptEditor->requestReplaceInCurrentSegment(from, to, cs);
    }
    else {
        transcriptEditor->requestReplaceAll(from, to, cs);
    }
}


void AppMainWindow::onShowViewerRequested() {

    if (controller && controller->currentTranscript()) {
        showRightPage(RightPage::Viewer);
    }
    else {
        showRightPage(RightPage::Placeholder);
        if (actionShowViewer) actionShowViewer->setChecked(false);
    }
}

void AppMainWindow::onShowEditorRequested() {

    if (controller && controller->currentTranscript()) {
        showRightPage(RightPage::Editor);
    }
    else {
        // No transcript: can't edit, revert to viewer/placeholder
        if (actionShowEditor) actionShowEditor->setChecked(false);
        if (actionShowViewer) actionShowViewer->setChecked(true);
        showRightPage(RightPage::Placeholder);
    }
}


}


