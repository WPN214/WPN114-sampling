QT += widgets
DEFINES += QT_DEPRECATED_WARNINGS

WPN114_AUDIO_REPOSITORY = ../WPN114-audio
INCLUDEPATH += $$WPN114_AUDIO_REPOSITORY
LIBS += -L$$[QT_INSTALL_QML]/WPN114/Audio -lWPN114-audio

macx {
    QMAKE_MAC_SDK = macosx10.14
}

HEADERS += $$PWD/source/sampler/sampler.hpp
HEADERS += $$PWD/source/multisampler/multisampler.hpp
SOURCES += $$PWD/source/sampler/sampler.cpp
SOURCES += $$PWD/source/multisampler/multisampler.cpp

SOURCES += $$PWD/qml_plugin.cpp
HEADERS += $$PWD/qml_plugin.hpp
