#ifndef JAVAHELPER_H
#define JAVAHELPER_H

#include <QObject>
#include <QTimer>

class JavaHelper : public QObject
{
    Q_OBJECT
public:
    QTimer *ledTimer;
    bool ledOn;

    explicit JavaHelper(QObject *parent = nullptr);

    Q_INVOKABLE int setLedState(int ledID, int ledState);
    Q_INVOKABLE int PWMPlay(int frequency);
    Q_INVOKABLE int PWMStop();

    Q_INVOKABLE void playPWMfor(int frequency, int msec);
    Q_INVOKABLE void flashLedfor(int msecInterval);
    Q_INVOKABLE void stopFlashLed();

signals:

public slots:
    void onPWMTimeout();
    void onLedTimeout();
};

#endif // JAVAHELPER_H
