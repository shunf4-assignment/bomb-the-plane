#include "SocketMan.h"
#include <QTimerEvent>
#include <QHostAddress>
#include <QNetworkProxy>
#include <QTextCodec>

#define SERVER_HOST "10.60.102.252"
#define SERVER_PORT 20270
#define TIMERCONN_TIME 2000
#define TIMERHEARTBEAT_TIME 2000
#define TIMERRECONN_DEFAULT_TIME 1000

#include <QNetworkConfiguration>

static QTextCodec *gbk = QTextCodec::codecForName("GBK");

SocketMan::SocketMan(QObject *parent) : QObject(parent), nc(ncm.defaultConfiguration()), ns(nc)
{
    m_socket = nullptr;

    m_connTimer = new QTimer(this);
    m_reconnTimer = new QTimer(this);
    m_heartbeatTimer = new QTimer(this);
    m_connTimer->setSingleShot(true);
    m_reconnTimer->setSingleShot(true);
    m_heartbeatTimer->setSingleShot(true);

    connect(m_connTimer, SIGNAL(timeout()), this, SLOT(onConnTimeout()));
    connect(m_reconnTimer, SIGNAL(timeout()), this, SLOT(onReconnTimeout()));
    connect(m_heartbeatTimer, SIGNAL(timeout()), this, SLOT(onHeartbeatTimeout()));

    connect(&ncm, SIGNAL(onlineStateChanged(bool)), this, SLOT(onNetworkChanged()));
    connect(&ns, SIGNAL(stateChanged(QNetworkSession::State)), this, SLOT(onNetworkChanged()));

    m_timerReconnTime = TIMERRECONN_DEFAULT_TIME;
}

SocketMan::SocketMan(QTcpSocket *socket, QObject *parent) : SocketMan(parent)
{
    m_socket = socket;
}

SocketMan::~SocketMan()
{
    if (m_socket)
    {
        m_socket->deleteLater();
        m_socket = nullptr;
    }
}

QAbstractSocket::SocketState SocketMan::getState()
{
    if (m_socket == nullptr)
        return QAbstractSocket::SocketState::UnconnectedState;
    return m_socket->state();
}

void SocketMan::restartConnTimer()
{
    m_connTimer->start(TIMERCONN_TIME);
}

void SocketMan::stopConnTimer()
{
    m_connTimer->stop();
}

void SocketMan::restartReconnTimer()
{
    qDebug() << "重连定时器开启";
    m_timerReconnTime += 1000;
    m_reconnTimer->start(m_timerReconnTime);
}

void SocketMan::stopReconnTimer()
{
    qDebug() << "重连定时器取消";
    m_reconnTimer->stop();
}


void SocketMan::sockConnect()
{
    stopReconnTimer();
    restartConnTimer();

    qDebug() << "socket 开始连接";
    if (m_socket == nullptr) {
        qDebug() << "错误：socket 为 nullptr";
    }

    m_socket->setProxy(QNetworkProxy::NoProxy);
    m_socket->setSocketOption(QAbstractSocket::SocketOption::KeepAliveOption, true);
    m_socket->connectToHost(SERVER_HOST, SERVER_PORT);
}

void SocketMan::reconnLater()
{
    if (m_socket) {
        m_socket->disconnect();
        m_socket->abort();
        m_socket->deleteLater();
        m_socket = nullptr;
    }

    restartReconnTimer();
}

void SocketMan::onConnTimeout()
{
    if (!m_socket)
        return;
    if (m_socket->state() != QAbstractSocket::ConnectedState)
    {
        qDebug() << "socket 连接超时:" << m_socket->state() << m_socket->peerAddress();
        emit socketError(QAbstractSocket::SocketError::SocketTimeoutError);

        reconnLater();
    }
}

void SocketMan::onReconnTimeout()
{
    qDebug() << "重连等待时间到, 开始重连";
    this->ensureConnected();
}

void SocketMan::onNetworkChanged()
{
    qDebug() << "网络情况发生变化, 开始重连";
    this->ensureConnected();
}

void SocketMan::ensureConnected()
{
    if (m_socket)
    {
        if (m_socket->state() == QAbstractSocket::UnconnectedState)
        {
            if (!this->m_connTimer->isActive()) {
                qDebug() << "检测到 socket 处于 UnconnectedState, 重连";
                this->sockConnect();
            }
        }
    }
    else
    {
        qDebug() << "检测到 m_socket 为空指针, 重新创建 socket";
        m_socket = new QTcpSocket;
        QObject::connect(m_socket, SIGNAL(connected()), this, SLOT(onConnected()));
        QObject::connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onError(QAbstractSocket::SocketError)));
        QObject::connect(m_socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(onSocketStateChanged(QAbstractSocket::SocketState)));

        this->sockConnect();
    }
}

void SocketMan::sendGram(Gram *gram)
{
    Gram *newGram = new Gram(*gram, this);
    QDataStream(newGram->data) >> underlie(newGram->type);

    this->m_writeQueue.enqueue(newGram);
    if (this->m_writeState == NotWriting)
    {
        qDebug() << "Start Writing to Socket";
        this->startWrite();
    }
}

QString SocketMan::getPeerAddress() const
{
    return this->m_socket->peerAddress().toString() + QStringLiteral(u":") + QString::number(this->m_socket->peerPort());
}

int SocketMan::getReconnRemainingTime() const
{
    return m_reconnTimer->remainingTime();
}

void SocketMan::onConnected()
{
    qDebug() << "已连接上!" << m_socket->state() << m_socket->peerAddress() << ":" << m_socket->peerPort() << m_socket->peerAddress().isNull();
    stopConnTimer();
    stopReconnTimer();
    m_timerReconnTime = TIMERRECONN_DEFAULT_TIME;

    m_socket->setSocketOption(QAbstractSocket::SocketOption::KeepAliveOption, true);



    qDebug() << "连接开始, 所有量重置";
    this->m_writeState = NotWriting;
    this->m_readData.clear();
    this->m_readType = ReadingHeader;
    this->m_readRemainLen = GramHeader::size();
    this->m_writeData.clear();

    for (auto g : this->m_writeQueue) {
        g->deleteLater();
    }

    this->m_writeQueue.clear();

    this->m_heartbeatTimer->start(TIMERHEARTBEAT_TIME);

    QObject::connect(m_socket, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    QObject::connect(m_socket, SIGNAL(bytesWritten(qint64)), this, SLOT(onBytesWritten(qint64)));

    emit connected();
}

void SocketMan::onReadyRead()
{
    QByteArray qba;
    qba.resize(static_cast<int>(this->m_readRemainLen));
    qint64 readRes = m_socket->read(qba.data(), this->m_readRemainLen);

    qDebug() << "从 socket 读取:" << qba.toPercentEncoding() << "(" << readRes << "/" << this->m_readRemainLen << ")";

    if (readRes == -1)
    {
        qDebug() << "read() 返回 -1，可能出错";
        this->errorAndDisconnect();
        return;
    }
    this->m_readRemainLen -= static_cast<unsigned int>(qba.length());
    this->m_readData += qba;

    if (this->m_readRemainLen == 0) {
        this->readCompleted();
    }
}

void SocketMan::onError(QAbstractSocket::SocketError err)
{
    qDebug() << "连接出错" << err;
    emit socketError(err);
    qDebug() << "停心跳计时";
    this->m_heartbeatTimer->stop();
    this->stopConnTimer();
    this->reconnLater();
}

void SocketMan::onSocketStateChanged(QAbstractSocket::SocketState state)
{
    qDebug() << "SocketStateChanged" << state;
    emit socketStateChanged(state);
}

void SocketMan::onBytesWritten(qint64)
{
    if (this->m_writeState == Writing)
    {
        if (this->m_socket->bytesToWrite() == 0) {
            if (this->m_writeData.size() == 0)
            {
                if (this->m_writeQueue.size() == 0)
                {
                    qDebug() << "队列和待发字节空";
                    qDebug() << "开启心跳计时";
                    this->m_heartbeatTimer->start();
                    this->m_writeState = NotWriting;
                }
                else
                {
                    this->startWrite();
                }
            }
            else
            {
                this->continueWrite();
            }
        } else {

        }
    }
    else
    {
        qDebug() << "奇怪: 处于 NotWriting 状态时, 接收到 bytesWritten";
    }
}

void SocketMan::onHeartbeatTimeout()
{
    if (!m_socket)
    {
        qDebug() << "奇怪: 连接不存在时, 产生心跳";
        return;
    }
    if (m_socket->state() == QAbstractSocket::ConnectedState)
    {
        if (this->m_writeState == NotWriting)
        {
#if (HEARTBEAT)
            qDebug() << "发送一个心跳包";

            Gram g;
            g.header.type = GramType::MSG_HEARTBEAT;
            g.header.seq = 0;
            g.header.length = 0;

            this->sendGram(&g);
#endif
        }
    }
    else {
        qDebug() << "奇怪: 连接未建立时, 产生心跳";
        return;
    }
}

void SocketMan::readCompleted()
{
    qDebug() << "readCompleted" << this->m_readType;
    if (this->m_readType == ReadingHeader)
    {
        this->m_readType = ReadingContent;
        GramHeader gh(this->m_readData);   // QDataStream 自动转字节序
        this->m_readRemainLen = gh.length;

        if (this->m_readRemainLen == 0)
        {   // 不可能
            this->m_readType = ReadingHeader;
            this->m_readRemainLen = GramHeader::size();

            Gram g(this->m_readData);
            emit gramArrived(&g);
            this->m_readData.clear();
        }
    }
    else if (this->m_readType == ReadingContent)
    {
        this->m_readType = ReadingHeader;
        this->m_readRemainLen = GramHeader::size();

        Gram g(this->m_readData);
        emit gramArrived(&g);
        this->m_readData.clear();
    }

    if (this->m_socket && this->m_socket->bytesAvailable())
    {
        this->onReadyRead();
    }
}

void SocketMan::startWrite()
{
    this->m_writeState = Writing;
    Gram *g = this->m_writeQueue.dequeue();
    this->m_writeData.clear();
    this->m_writeData = g->toByteArray();
    qDebug() << "发送: " << g->header.length << "(Type:)" << QtEnumToString(g->type) << "(Data:)" << g->data.toPercentEncoding();
    g->deleteLater();
    qDebug() << "停心跳计时";
    this->m_heartbeatTimer->stop();
    this->continueWrite();
}

void SocketMan::continueWrite()
{
    qint64 writeLen = this->m_socket->write(this->m_writeData.constData(), this->m_writeData.size());
    if (writeLen == -1)
    {
        qDebug() << "写遇到错误，断开连接";
        this->errorAndDisconnect();
        return;
    }

    this->m_writeData.remove(0, static_cast<int>(writeLen));
}

void SocketMan::errorAndDisconnect()
{
    qDebug() << "连接过程中遇到问题, 断开";
    this->m_socket->close();
    this->onError(QAbstractSocket::SocketError::UnknownSocketError);
}

