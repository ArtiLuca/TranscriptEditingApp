#include "AppMainWindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QHBoxLayout>
#include <QListWidgetItem>
#include <QKeySequence>
#include <QInputDialog>

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
}


// === Setup ===

void AppMainWindow::setupMenus() {

    fileMenu = menuBar()->addMenu(tr("&File"));
    editMenu = menuBar()->addMenu(tr("&Edit"));
    audioMenu = menuBar()->addMenu(tr("&Audio"));
    viewMenu = menuBar()->addMenu(tr("&View"));

    actionChooseRootDirectory = new QAction(tr("Choose &Root Folder..."), this);
    actionReload = new QAction(tr("&Reload Transcripts"), this);
    actionImport = new QAction(tr("&Import Transcript Folder..."), this);
    actionSaveCurrent = new QAction(tr("&Save Current Transcript"), this);
    actionSaveAll = new QAction(tr("Save &All Transcripts"), this);
    actionExit = new QAction(tr("E&xit"), this);

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
    actionUndo = new QAction(tr("&Undo"), this);
    actionRedo = new QAction(tr("&Redo"), this);
    actionUndo->setShortcut(QKeySequence::Undo);
    actionRedo->setShortcut(QKeySequence::Redo);
    actionUndo->setEnabled(false);
    actionRedo->setEnabled(false);

    connect(actionUndo, &QAction::triggered, this, &AppMainWindow::onUndoRequested);
    connect(actionRedo, &QAction::triggered, this, &AppMainWindow::onRedoRequested);

    editMenu->addAction(actionUndo);
    editMenu->addAction(actionRedo);

    // Audio menu actions
    actionPlayPause = new QAction(tr("Play/Pause"), this);
    actionStop = new QAction(tr("Stop"), this);
    actionSeekBack = new QAction(tr("<< 5s"), this);
    actionSeekForward = new QAction(tr("5s >>"), this);

    connect(actionPlayPause, &QAction::triggered, this, &AppMainWindow::onPlayPauseRequested);
    connect(actionStop,      &QAction::triggered, this, &AppMainWindow::onStopRequested);
    connect(actionSeekBack,  &QAction::triggered, this, &AppMainWindow::onSeekBackward5s);
    connect(actionSeekForward,&QAction::triggered,this, &AppMainWindow::onSeekForward5s);

    audioMenu->addAction(actionPlayPause);
    audioMenu->addAction(actionStop);
    audioMenu->addSeparator();
    audioMenu->addAction(actionSeekBack);
    audioMenu->addAction(actionSeekForward);

    // View menu kept for future options (e.g., show/hide toolbars)

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
        tr("No transcript selected.\n\n"
           "Later this area will host:\n"
           " - TranscriptViewerWidget\n"
           " - TranscriptEditorWidget\n"
           " - TranscriptAudioWidget, etc."),
        placeholderPage);
    placeholderLabel->setAlignment(Qt::AlignCenter);

    transcriptPreview = new QTextBrowser(placeholderPage);
    transcriptPreview->setReadOnly(true);

    // Start empty
    transcriptPreview->setPlainText(QString());

    placeholderLayout->addWidget(placeholderLabel);
    placeholderLayout->addWidget(transcriptPreview, 1);

    rightStack->addWidget(placeholderPage);
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

    Q_UNUSED(transcript);
    syncCurrentIndexSpin();
    updateWindowTitleForCurrentTranscript();


    // Test
    if (!controller || !transcriptPreview)
        return;

    const auto* t = controller->currentTranscript();
    if (!t)
        return;

    // Read transcript.txt as-is
    QFile f(t->referencePath);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString text = QString::fromUtf8(f.readAll());
        transcriptPreview->setPlainText(text);
        rightStack->setCurrentWidget(placeholderPage);
        if (statusBar)
            statusBar->showMessage(tr("Loaded transcript: %1").arg(t->title), 4000);
        else {
            transcriptPreview->setPlainText(QString());
            if (statusBar)
                statusBar->showMessage(tr("Could not open transcript file: %1").arg(t->referencePath), 4000);
        }
    }

    // Later: switch rightStack to TranscriptViewerWidget/EditorWidget
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

}


