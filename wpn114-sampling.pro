TARGET = WPN114-sampling
TEMPLATE = lib
CONFIG += c++11 dll
QT += quick

QMLDIR_FILES += $$PWD/qml/qmldir
DESTDIR = $$[QT_INSTALL_QML]/WPN114/Audio/Sampling

for(FILE,QMLDIR_FILES) {
    QMAKE_POST_LINK += $$quote(cp $${FILE} $${DESTDIR}$$escape_expand(\n\t))
}

QMAKE_LFLAGS += -Wl,-rpath,$$[QT_INSTALL_QML]/WPN114/Audio

include($$PWD/wpn114-sampling.pri)

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
