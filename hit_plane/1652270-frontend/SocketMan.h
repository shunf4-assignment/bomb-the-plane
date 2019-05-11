#ifndef SOCKETMAN_H
#define SOCKETMAN_H

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include "Grams.h"
#include <QQueue>
#include <QTimer>
#include <QNetworkSession>
#include <QNetworkConfigurationManager>
#include <QNetworkConfiguration>

#include <QMetaEnum>

template<typename QEnum>
QString QtEnumToString (const QEnum value)
{
    return (QMetaEnum::fromType<QEnum>().valueToKey(static_cast<typename std::underlying_type<QEnum>::type>(value)));
}

#define HEARTBEAT 0

enum SocketManReadType
{
    ReadingHeader,
    ReadingContent
};

enum SocketManWriteState
{
    NotWriting,
    Writing
};

class SocketMan : public QObject
{
    Q_OBJECT

private:
    QTcpSocket *m_socket;

    int m_timerReconnTime;

    quint32 seqNum;

    unsigned int m_readRemainLen;
    SocketManReadType m_readType;
    QByteArray m_readData;
    GramHeader m_tempHeader;

    QByteArray m_writeData;
    SocketManWriteState m_writeState;
    QQueue<Gram *> m_writeQueue;

    QTimer *m_reconnTimer;
    QTimer *m_connTimer;
    QTimer *m_heartbeatTimer;

    QNetworkConfigurationManager ncm;
    QNetworkConfiguration nc;
    QNetworkSession ns;


public:
    explicit SocketMan(QObject *parent = nullptr);
    SocketMan(QTcpSocket *socket, QObject *parent = nullptr);
    ~SocketMan();

    QAbstractSocket::SocketState getState();
    void ensureConnected();
    void sendGram(Gram *gram);

    QString getPeerAddress() const;

    int getReconnRemainingTime() const;

    void errorAndDisconnect();

signals:
    void connected();
    void socketError(QAbstractSocket::SocketError err);
    void socketStateChanged(QAbstractSocket::SocketState state);
    void gramArrived(Gram *gram);

public slots:
    void onError(QAbstractSocket::SocketError err);

protected slots:
    void onConnected();
    void onReadyRead();

    void onSocketStateChanged(QAbstractSocket::SocketState state);
    void onBytesWritten(qint64 bytes);

    void onConnTimeout();
    void onReconnTimeout();

    void onNetworkChanged();
    void onHeartbeatTimeout();
protected:
    void restartConnTimer();
    void stopConnTimer();
    void restartReconnTimer();
    void stopReconnTimer();
    void sockConnect();
    void readCompleted();
    void startWrite();
    void continueWrite();

    void reconnLater();
};

#endif // SOCKETMAN_H
