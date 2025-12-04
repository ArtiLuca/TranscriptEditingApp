QT += core gui widgets
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets multimedia multimediawidgets

CONFIG += c++17
CONFIG += gui

HEADERS += \
    Controller/AppController.h \
    Model/Data/Segment.h \
    Model/Data/Speaker.h \
    Model/Data/Transcript.h \
    Model/Service/TranscriptEditor.h \
    Model/Service/TranscriptEditorAlt.h \
    Model/Service/TranscriptExporter.h \
    Model/Service/TranscriptImporter.h \
    Model/Service/TranscriptManager.h \
    Model/Service/TranscriptParser.h \
    Model/Service/TranscriptSearch.h \
    View/AppMainWindow.h \
    View/Widgets/TranscriptEditorWidget.h \
    View/Widgets/TranscriptViewerWidget.h \
    View/Widgets/Utility/EditableSegmentRowWidget.h \
    View/Widgets/Utility/SegmentRowWidget.h

SOURCES += \
    Controller/AppController.cpp \
    Model/Data/Segment.cpp \
    Model/Data/Speaker.cpp \
    Model/Data/Transcript.cpp \
    Model/Service/TranscriptEditor.cpp \
    Model/Service/TranscriptEditorAlt.cpp \
    Model/Service/TranscriptExporter.cpp \
    Model/Service/TranscriptImporter.cpp \
    Model/Service/TranscriptManager.cpp \
    Model/Service/TranscriptParser.cpp \
    Model/Service/TranscriptSearch.cpp \
    View/AppMainWindow.cpp \
    View/Widgets/TranscriptEditorWidget.cpp \
    View/Widgets/TranscriptViewerWidget.cpp \
    View/Widgets/Utility/EditableSegmentRowWidget.cpp \
    View/Widgets/Utility/SegmentRowWidget.cpp \
    main.cpp

RESOURCES += \
    Resource.qrc
