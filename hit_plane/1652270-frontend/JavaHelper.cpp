#include "JavaHelper.h"
#ifdef Q_OS_ANDROID
#include <QAndroidJniObject>
#include <QAndroidJniEnvironment>
#endif
#include <QTimer>

//extern template jint QAndroidJniObject::callStaticMethod<jint>(jclass clazz, const char *methodName, const char *sig, ...);

JavaHelper::JavaHelper(QObject *parent) : QObject(parent)
{
    ledTimer = nullptr;
    ledOn = false;
}

int JavaHelper::setLedState(int ledID, int ledState)
{
#ifdef Q_OS_ANDROID
    jint result = 0;
    result = QAndroidJniObject::callStaticMethod<jint>("com/friendlyarm/AndroidSDK/HardwareControler", "setLedState", "(II)I", jint(ledID), jint(ledState));

    QAndroidJniEnvironment env;
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
    }

    return result;
#else
    return 0;
#endif
}

int JavaHelper::PWMPlay(int frequency)
{
#ifdef Q_OS_ANDROID
    jint result = 0;
    result = QAndroidJniObject::callStaticMethod<jint>("com/friendlyarm/AndroidSDK/HardwareControler", "PWMPlay", "(I)I", jint(frequency));

    QAndroidJniEnvironment env;
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
    }

    return result;
#else
    return 0;
#endif
}

int JavaHelper::PWMStop()
{
#ifdef Q_OS_ANDROID
    jint result = 0;
    result = QAndroidJniObject::callStaticMethod<jint>("com/friendlyarm/AndroidSDK/HardwareControler", "PWMStop", "()I");

    QAndroidJniEnvironment env;
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
    }

    return result;
#else
    return 0;
#endif
}

void JavaHelper::playPWMfor(int frequency, int msec)
{
#ifdef Q_OS_ANDROID
    PWMPlay(frequency);

    QTimer::singleShot(msec, this, SLOT(onPWMTimeout()));
#else
    return;
#endif
}

void JavaHelper::flashLedfor(int msecInterval)
{
#ifdef Q_OS_ANDROID
    if (ledTimer == nullptr) {
        ledTimer = new QTimer(this);
        connect(ledTimer, SIGNAL(timeout()), this, SLOT(onLedTimeout()));
    }

    ledTimer->setSingleShot(false);
    ledOn = true;
    for (int i = 0; i < 4; i++) {
        setLedState(i, 1);
    }
    ledTimer->start(msecInterval);
#else
    return;
#endif
}

void JavaHelper::stopFlashLed()
{
#ifdef Q_OS_ANDROID
    if (ledTimer) {
        ledTimer->stop();
    }
    ledOn = false;
    for (int i = 0; i < 4; i++) {
        setLedState(i, 0);
    }
#else
    return;
#endif
}

void JavaHelper::onPWMTimeout()
{
#ifdef Q_OS_ANDROID
    PWMStop();
#endif
}

void JavaHelper::onLedTimeout()
{
#ifdef Q_OS_ANDROID
    if (ledOn == false) {
        ledOn = true;
        for (int i = 0; i < 4; i++) {
            setLedState(i, 1);
        }
    } else {
        ledOn = false;
        for (int i = 0; i < 4; i++) {
            setLedState(i, 0);
        }
    }
#else
#endif
}

