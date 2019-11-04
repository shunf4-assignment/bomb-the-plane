
win32 {
    QMAKE_CXXFLAGS += /utf-8 /wd4819
    QMAKE_CXXFLAGS_DEBUG  += /utf-8 /wd4819
    QMAKE_CXXFLAGS_RELEASE  += /utf-8 /wd4819
    QMAKE_CXXFLAGS_RELEASE_WITH_DEBUGINFO  += /utf-8 /wd4819
}

android {
    QT += androidextras
}

QT += quick network qml core gui
CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Refer to the documentation for the
# deprecated API to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += QT_BEING_USED

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        main.cpp \
    SocketMan.cpp \
    Grams.cpp \
    Friend.cpp \
    FriendsModel.cpp \
    BTPMapModel.cpp \
    BTP.cpp \
    JavaHelper.cpp

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    myimage.h \
    SocketMan.h \
    Grams.h \
    ../ConsoleApplication2/enum.h \
    Friend.h \
    FriendsModel.h \
    BTPMapModel.h \
    BTP.h \
    enum.h \
    JavaHelper.h

DISTFILES += \
    android/AndroidManifest.xml \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradlew \
    android/res/values/libs.xml \
    android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew.bat \
    android/src/com/friendlyarm/AndroidSDK/FileCtlEnum.java \
    android/src/com/friendlyarm/AndroidSDK/GPIOEnum.java \
    android/src/com/friendlyarm/AndroidSDK/HardwareControler.java \
    android/src/com/friendlyarm/AndroidSDK/SPI.java \
    android/src/com/friendlyarm/AndroidSDK/SPIEnum.java

contains(ANDROID_TARGET_ARCH,armeabi-v7a) {
    ANDROID_PACKAGE_SOURCE_DIR = \
        $$PWD/android
}
