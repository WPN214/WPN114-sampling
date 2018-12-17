TARGET = WPN114-sampling
TEMPLATE = lib
CONFIG += c++11 dll
QT += quick

QMLDIR_FILES += $$PWD/qml/qmldir
QMLDIR_FILES += $$PWD/qml/sampling.qmltypes
OTHER_FILES = $$QMLDIR_FILES

localmod: DESTDIR = $$QML_MODULE_DESTDIR/WPN114/Audio/Sampling
else {
    DESTDIR = $$[QT_INSTALL_QML]/WPN114/Audio/Sampling
    QML_MODULE_DESTDIR = $$[QT_INSTALL_QML]
}

for(FILE,QMLDIR_FILES) {
    QMAKE_POST_LINK += $$quote(cp $${FILE} $${DESTDIR}$$escape_expand(\n\t))
}

WPN114_AUDIO_REPOSITORY = ../WPN114-audio
INCLUDEPATH += $$WPN114_AUDIO_REPOSITORY
LIBS += -L$$QML_MODULE_DESTDIR/WPN114/Audio -lWPN114-audio
QMAKE_RPATHDIR += $$QML_MODULE_DESTDIR/WPN114/Audio

HEADERS += $$PWD/source/sampler/sampler.hpp
HEADERS += $$PWD/source/multisampler/multisampler.hpp
SOURCES += $$PWD/source/sampler/sampler.cpp
SOURCES += $$PWD/source/multisampler/multisampler.cpp

SOURCES += $$PWD/qml_plugin.cpp
HEADERS += $$PWD/qml_plugin.hpp
