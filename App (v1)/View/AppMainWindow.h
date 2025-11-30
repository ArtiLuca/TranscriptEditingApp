#ifndef VIEW_MAIN_WINDOW_H
#define VIEW_MAIN_WINDOW_H

#include "Controller/AppController.h"

#include <QMainWindow>
#include <QSplitter>
#include <QDockWidget>
#include <QListWidget>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextBrowser>
#include <QToolBar>
#include <QMenuBar>
#include <QStatusBar>
#include <QAction>
#include <QSpinBox>
#include <QLabel>
#include <QSlider>

namespace View {

class AppMainWindow : public QMainWindow {

    Q_OBJECT

public:

    explicit AppMainWindow(QWidget* parent = nullptr);

    void setController(Controller::AppController* ctrl);

signals:

    /** @brief Emitted when the user selects a transcript from the list. */
    void transcriptSelected(int index);

private slots:

    // === File / root directory ===
    void onChooseRootDirectory();
    void onReloadTranscripts();
    void onImportTranscript();
    void onSaveCurrent();
    void onSaveAll();
    void onExitRequested();

    // === Selection & updates ===
    void onTranscriptListItemClicked(QListWidgetItem* item);
    void onCurrentIndexSpinChanged(int value);

    void onTranscriptsReloaded();
    void onCurrentTranscriptChanged(Model::Data::Transcript* transcript);
    void onTranscriptContentChanged(Model::Data::Transcript* transcript);

    // === Editing actions (forwarded to controller) ===
    void onUndoRequested();
    void onRedoRequested();

    // === Audio actions ===
    void onPlayPauseRequested();
    void onStopRequested();
    void onSeekBackward5s();
    void onSeekForward5s();
    void onAudioSliderMoved(int value);

    // === Controller feedback ===
    void onErrorOccurred(const QString& message);
    void onSaveCompleted(Model::Data::Transcript* transcript);
    void onImportCompleted(int newIndex, Model::Data::Transcript* transcript);
    void onUndoRedoAvailabilityChanged(bool canUndo, bool canRedo);
    void onAudioPositionChanged(qint64 positionMs, qint64 durationMs);
    void onAudioPlaybackStateChanged(QMediaPlayer::PlaybackState state);

private:

    // === Setup helpers ===
    void setupMenus();
    void setupToolbars();
    void setupPanels();
    void setupStatusBar();
    void connectControllerSignals();

    // === UI helpers ===
    void refreshTranscriptList();
    void syncSelectionToController();
    void syncCurrentIndexSpin();
    void updateWindowTitleForCurrentTranscript();
    void updateAudioStatus(qint64 positionMs, qint64 durationMs);

private:

    // Controller
    Controller::AppController* controller = nullptr;

    // Core layout
    QSplitter* mainSplitter = nullptr;
    QWidget* leftPanel = nullptr;
    QWidget* rightPanel = nullptr;
    QVBoxLayout* leftLayout = nullptr;
    QVBoxLayout* rightLayout = nullptr;

    // Left-side widgets
    QListWidget* transcriptList = nullptr;
    QSpinBox* indexSpinBox = nullptr;
    QLabel* rootDirectoryLabel = nullptr;

    // Right-side widgets
    QStackedWidget* rightStack = nullptr;
    QWidget* placeholderPage = nullptr;
    QTextBrowser* transcriptPreview = nullptr;

    // Menus & Toolbars
    QMenu* fileMenu = nullptr;
    QMenu* editMenu = nullptr;
    QMenu* audioMenu = nullptr;
    QMenu* viewMenu = nullptr;
    QToolBar* fileToolbar = nullptr;
    QToolBar* editToolbar = nullptr;
    QToolBar* audioToolbar = nullptr;
    QToolBar* viewToolbar = nullptr;

    // Actions
    QAction* actionChooseRootDirectory = nullptr;
    QAction* actionReload = nullptr;
    QAction* actionImport = nullptr;
    QAction* actionSaveCurrent = nullptr;
    QAction* actionSaveAll = nullptr;
    QAction* actionExit = nullptr;

    QAction* actionUndo = nullptr;
    QAction* actionRedo = nullptr;

    QAction* actionPlayPause = nullptr;
    QAction* actionStop = nullptr;
    QAction* actionSeekBack = nullptr;
    QAction* actionSeekForward = nullptr;

    // Status bar elements
    QStatusBar* statusBar = nullptr;
    QLabel* audioStatusLabel = nullptr;
    QSlider* audioSlider = nullptr;
};

}


#endif // VIEW_MAIN_WINDOW_H
