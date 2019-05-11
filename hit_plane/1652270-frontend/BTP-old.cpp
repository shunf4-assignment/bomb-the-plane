#define _CRT_SECURE_NO_WARNINGS
#include "BTP.h"
#include <QMetaEnum>
#include <QTextCodec>
#include <QFile>


#define memcpy_s(a,b,c,d) memcpy(a,c,d)

static QTextCodec *gbk = QTextCodec::codecForName("GBK");

BTP::BTP(QObject *parent) : QObject(parent)
{
    this->m_state = State::Logout;
    this->m_uiState = static_cast<UIState>(0);
    this->m_map = new BTPMapModel(this);
    this->m_oMap = new BTPMapModel(this);
    this->m_friends = new FriendsModel(this);

    this->m_sm = new SocketMan(this);
    this->m_nextStateAfterError = State::Logout;

    this->m_socketStateText.push_back(QStringLiteral(u"未初始化"));
    this->m_socketStateText.push_back(QStringLiteral(u"未知地址"));
    this->m_socketStateText.push_back(QStringLiteral(u""));

    emit socketStateTextChanged();

    QObject::connect(m_sm, SIGNAL(connected()), this, SLOT(onConnected()));
    QObject::connect(m_sm, SIGNAL(socketError(QAbstractSocket::SocketError)), this, SLOT(onSocketError(QAbstractSocket::SocketError)));
    QObject::connect(m_sm, SIGNAL(socketStateChanged(QAbstractSocket::SocketState)), this, SLOT(onSocketStateChanged(QAbstractSocket::SocketState)));
    QObject::connect(m_sm, SIGNAL(gramArrived(Gram *)), this, SLOT(onGramArrived(Gram *)));

    periodicalTimer = new QTimer(this);
    periodicalTimer->setSingleShot(false);

    connect(periodicalTimer, SIGNAL(timeout()), this, SIGNAL(periodicalRefresh()));

    connect(this, SIGNAL(periodicalRefresh()), this, SLOT(onPeriodicalRefresh()));

    periodicalTimer->start(1000);

    connect(this, SIGNAL(reconnRemainingTimeChanged()), this, SLOT(onReconnRemainingTimeChanged()));


}

BTP::~BTP()
{
}

void BTP::onConnected()
{
    m_socketStateText[1] = m_sm->getPeerAddress();
    emit socketStateTextChanged();

    if (+(m_state & State::Logout) || m_state == State::Null)
    {
        login();
    }
}

void BTP::onSocketError(QAbstractSocket::SocketError err)
{
    m_socketStateText[0] = QStringLiteral(u"出错: %1").arg(QMetaEnum::fromType<QAbstractSocket::SocketError>().valueToKey(err));
    emit socketStateTextChanged();

    this->setState(State::Logout);
}

void BTP::onSocketStateChanged(QAbstractSocket::SocketState state)
{
    m_socketStateText[0] = QStringLiteral(u"%1").arg(QMetaEnum::fromType<QAbstractSocket::SocketState>().valueToKey(state));
    emit socketStateTextChanged();
}

void BTP::fatalError()
{
    qDebug() << "致命错误";
    this->m_sm->errorAndDisconnect();
    this->m_sm->onError(QAbstractSocket::SocketError::UnknownSocketError);
}

void BTP::handleStatusGram(Gram *gram)
{
    const char *initial = gram->data.constData();
    const char *p = initial + sizeof(gram->type);
    // 读取我目前状态
    setState(*reinterpret_cast<const State *>(p));
    p += sizeof(State);

    // 如果状态不是 Idle, 还会给我发 GameInfo = {(对方名称),(对方部署)OH1,OT1,OH2,OT2,OH3,OT3,(我部署)H1,T1,H2,T2,H3,T3,BoardByHim[100],BoardByMe[100]}
    if (!(state() & State::Idle)) {
        const char *q = p;
        while (*q && q - initial < gram->header.length)
        {
            q++;
        }
        if (q - initial == gram->header.length)
        {
            fatalError();
            return;
        }
        m_opName = p;
        p = q+1;

        m_oh1 = *reinterpret_cast<const unsigned char *>(p);
        p++;
        m_ot1 = *reinterpret_cast<const unsigned char *>(p);
        p++;
        m_oh2 = *reinterpret_cast<const unsigned char *>(p);
        p++;
        m_ot2 = *reinterpret_cast<const unsigned char *>(p);
        p++;
        m_oh3 = *reinterpret_cast<const unsigned char *>(p);
        p++;
        m_ot3 = *reinterpret_cast<const unsigned char *>(p);
        p++;

        m_h1 = *reinterpret_cast<const unsigned char *>(p);
        p++;
        m_t1 = *reinterpret_cast<const unsigned char *>(p);
        p++;
        m_h2 = *reinterpret_cast<const unsigned char *>(p);
        p++;
        m_t2 = *reinterpret_cast<const unsigned char *>(p);
        p++;
        m_h3 = *reinterpret_cast<const unsigned char *>(p);
        p++;
        m_t3 = *reinterpret_cast<const unsigned char *>(p);
        p++;

        QVector<Grid> oMapVector(100);
        QVector<Grid> oMapVector(100);

        for (int i = 0; i < 100; i++) {
            oMapVector[i] = *reinterpret_cast<const Grid *>(p);
            p++;
        }
        m_oMap->setDataByBTPMapGridTypeList(oMapVector);

        for (int i = 0; i < 100; i++) {
            mapVector[i] = *reinterpret_cast<const Grid *>(p);
            p++;
        }
        m_map->setDataByBTPMapGridTypeList(mapVector);
    }

    QVector<Friend *> friends;

    QByteArray t;

    while (p - initial < gram->header.length)
    {
        const char *q = p;
        while (*q && q - initial < gram->header.length)
            q++;

        if (q - initial == gram->header.length)
        {
            fatalError();
            return;
        }
        q++;
        if (q + 3 - initial >= gram->header.length)
        {
            fatalError();
            return;
        }

        t = QByteArray(p);

        QString username = gbk->toUnicode(t);

        State hisState = *reinterpret_cast<const State *>(q);

        Friend *newFriend = new Friend(username, hisState);
        friends.push_back(newFriend);

        p = q + 4;
    }

    qDebug() << "Friends" << friends;
    this->friends()->setDataByFriendList(friends);
}


void BTP::onGramArrived(Gram *gram)
{
    qDebug() << "接收: (Len)" << gram->header.length << "(Type)" << QtEnumToString(gram->type) << gram->data.toHex();
    QByteArray ba = gram->toByteArray();
    qDebug() << "接收:" << gbk->toUnicode(ba);

    switch (gram->type)
    {



    default:
        setState(BTPState::ErrorState);
        setHint(QStringLiteral(u"收到的报文格式不正确. 将尝试重新连接服务器."));
        fatalError();
    }

}

void BTP::onPeriodicalRefresh()
{
    emit reconnRemainingTimeChanged();
}

void BTP::onReconnRemainingTimeChanged()
{
    qreal rt = this->reconnRemainingTime();
    if (rt == qreal(0))
        m_socketStateText[2] = "";
    else
        m_socketStateText[2] = QStringLiteral(u"重连时间:%1s").arg(QString::number(this->reconnRemainingTime(), 'g', 2));
    emit socketStateTextChanged();
}

BTP::BTPState BTP::state() const
{
    return this->m_state;
}

void BTP::setState(BTP::BTPState state)
{
    BTPState prevState = this->m_state;
    this->m_state = state;

    qDebug() << getStateText(state);

    if ((prevState == NotEverLoginState || prevState == LogFailedPasswordErrorState || prevState == LogFailedUsernameErrorState || prevState == ErrorState) && state == DisconnectedState && m_sm->getState() == QAbstractSocket::ConnectedState) {
        qDebug() << "现在登录";
        this->login();
    }

    if ((prevState == NotEverLoginState || prevState == ErrorState) && state == DisconnectedState) {
        this->setEverLogIn(true);
    }

    emit stateChanged();
}

bool BTP::everLogIn() const
{
    return this->m_everLogIn;
}

void BTP::setEverLogIn(bool everLogIn)
{
    this->m_everLogIn = everLogIn;
    emit everLogInChanged();
}

QString BTP::username() const
{
    return this->m_username;
}

void BTP::setUsername(const QString &username)
{
    this->m_username = username;
    emit usernameChanged();
}

QString BTP::password() const
{
    return this->m_password;
}

void BTP::setPassword(const QString &password)
{
    this->m_password = password;
}

void BTP::init()
{
    this->m_sm->ensureConnected();
}

FriendsModel *BTP::friends()
{
    return this->m_friends;
}

void BTP::setFriends(FriendsModel *friendsModel)
{
    this->m_friends->deleteLater();
    this->m_friends = friendsModel;
    this->m_friends->setParent(this);
    emit friendsChanged();
}

Chat *BTP::currentChat()
{
    return this->m_currentChat;
}

void BTP::setCurrentChat(Chat *currentChat)
{
    this->m_currentChat = currentChat;
    emit currentChatChanged();
}

void BTP::switchCurrentChat(int currentIndex)
{
    if (currentIndex >= 0)
        this->setCurrentChat(m_chats.value(m_friends->getData().at(currentIndex)->username()));
    else
        this->setCurrentChat(nullptr);
}

QString BTP::socketStateText() const
{
    return m_socketStateText.join(" ");
}

qreal BTP::reconnRemainingTime() const
{
    int rmt = m_sm->getReconnRemainingTime();
    if (rmt == -1)
        return qreal(0);

    return qreal(rmt) / 1000;
}

QHash<QString, Chat *> &BTP::chats()
{
    return this->m_chats;
}

QString BTP::getStateText(BTP::BTPState state)
{
    return QString(QMetaEnum::fromType<BTPState>().valueToKey(state));
}

void BTP::changePassword()
{
    qDebug() << "执行 changePassword()";
    this->setState(ChangingPasswordState);

    Gram changePasswordGram;
    changePasswordGram.header.type = MsgType::MSG_CHANGE_PASSWORD;
    changePasswordGram.header.seq = m_seq++;
    changePasswordGram.header.length = 32;

    changePasswordGram.data.resize(changePasswordGram.header.length);
    changePasswordGram.data.fill('\0');

    auto gbkPassword = gbk->fromUnicode(this->m_password);

    memcpy_s(changePasswordGram.data.data(), 32, gbkPassword.constData(), 32);

    qDebug() << "发送了修正密码报文";
    this->m_sm->sendGram(&changePasswordGram);
}

void BTP::nextStateAfterError()
{
    this->setState(m_nextStateAfterError);
}

void BTP::requestHistory()
{
    qDebug() << "执行 requestHistory()";

    Gram historyGram;
    historyGram.header.type = MsgType::MSG_REQUEST_RECORD;
    historyGram.header.seq = m_seq++;
    historyGram.header.length = 0;

    qDebug() << "发送了请求历史记录报文";
    this->m_sm->sendGram(&historyGram);
}

void BTP::handleMessage(Gram *gram)
{

    QByteArray t;

    t = gram->data.mid(0, 32);
    t = QByteArray(t.constData());
    QString sender = gbk->toUnicode(t);

    t = gram->data.mid(32);
    QString text = gbk->toUnicode(t);

    if (gram->header.type == GramType::MSG_SEND_MESSAGE)
    {
        // Private Convers.
        this->chats().value(sender)->msgModel()->appendChatMessage(new ChatMessage(this->m_chats.value(sender)->friendfriend(), ChatMessage::TextMessage, QDateTime::currentDateTime(), text, nullptr, 1.0));
    } else {
        // Group Chat.
        this->chats().value(QStringLiteral("all"))->msgModel()->appendChatMessage(new ChatMessage(this->m_chats.value(sender)->friendfriend(), ChatMessage::TextMessage, QDateTime::currentDateTime(), text, nullptr, 1.0));
    }

}

void BTP::handleLogInOutBroadcast(Gram *gram)
{
    QString username = gbk->toUnicode(QByteArray(gram->data.constData()));
    if (gram->header.type == GramType::MSG_USER_LOGIN)
    {
        this->m_chats.value(username)->friendfriend()->setIsOnline(true);
    }
    else {
        this->m_chats.value(username)->friendfriend()->setIsOnline(false);
    }
}

void BTP::handleFileInfo(Gram *gram)
{
    if (this->m_receiveFileState == ReceiveFileState::ReceivingFileState)
    {
        this->rejectFile(gbk->toUnicode(QByteArray(gram->data.mid(0, 32).constData())), gram->header.seq);
        return;
    }

    this->m_receiveFileSeq = gram->header.seq;
    QString username = gbk->toUnicode(QByteArray(gram->data.mid(0, 32).constData()));
    this->m_receiveFileName = gbk->toUnicode(QByteArray(gram->data.mid(32, 32).constData()));
    QDataStream qds(gram->data.mid(64, 4));
    qds >> this->m_receiveFileSize;
    this->m_receiveFileState = ReceivingFileState;
    this->setState(AskAcceptFileState);
    this->setHint(QStringLiteral(u"%1 想给你发送 [文件: %2](%3字节)").arg(username).arg(this->m_receiveFileName).arg(this->m_receiveFileSize));
    this->m_receiveFileContent.clear();
    this->m_receiveFileChatMessage = new ChatMessage(this->m_chats.value(username)->friendfriend(), ChatMessage::FileMessage, QDateTime::currentDateTime(), QStringLiteral(u"[文件: %1](%2字节)").arg(this->m_receiveFileName).arg(this->m_receiveFileSize), nullptr, 0.0);

    this->m_chats.value(username)->msgModel()->appendChatMessage(this->m_receiveFileChatMessage);
}

void BTP::receiveFileContent(Gram *gram)
{
    assert(this->m_receiveFileState == ReceivingFileState);
    assert(gbk->toUnicode(QByteArray(gram->data.mid(0, 32).constData())) == this->m_receiveFileChatMessage->friendfriend()->username());
    assert(this->m_receiveFileSeq == gram->header.seq);

    int fileSize = gram->data.size() - 32;
    this->m_receiveFileContent += gram->data.mid(32);
    this->m_receiveFileChatMessage->setProgress(this->m_receiveFileChatMessage->progress() + qreal(fileSize) / this->m_receiveFileSize);
}

void BTP::handleFileEnd(Gram *gram)
{
    assert(this->m_receiveFileState == ReceivingFileState);
    assert(gbk->toUnicode(QByteArray(gram->data.mid(0, 32).constData())) == this->m_receiveFileChatMessage->friendfriend()->username());
    assert(this->m_receiveFileSeq == gram->header.seq);

    this->m_receiveFileState = NotReceivingFileState;

    if(this->m_receiveFileContent.size() == this->m_receiveFileSize)
    {
        this->m_receiveFileChatMessage->setProgress(1.0);

        QFile file(this->m_receiveFileName);
        file.open(QIODevice::WriteOnly);
        file.write(this->m_receiveFileContent);
        file.waitForBytesWritten(-1);
        file.close();

        this->m_receiveFileChatMessage->setText(this->m_receiveFileChatMessage->text() + QStringLiteral(u" (接受完成)"));

        qDebug() << "已经写入文件.";
    } else {

        this->m_receiveFileChatMessage->setText(this->m_receiveFileChatMessage->text() + QStringLiteral(u" (主动中断)"));
        qDebug() << "已经中断.";
    }

}

void BTP::handleSendMessageSucceed(Gram *gram)
{
    this->m_seqToCM.value(gram->header.seq)->setProgress(1.0);
}

void BTP::handleFileInfoSucceed(Gram *gram)
{
    this->m_seqToCM.value(gram->header.seq)->setText(this->m_seqToCM.value(gram->header.seq)->text() + QStringLiteral(u" (对方在线)"));
}

void BTP::handleSendFileContentSucceed(Gram *gram)
{
    this->m_seqToCM.value(gram->header.seq)->setProgress(this->m_seqToCM.value(gram->header.seq)->progress() + qreal(1024) / this->m_sendFileContent.size());
}

void BTP::handleFileEndSucceed(Gram *gram)
{
    this->m_seqToCM.value(gram->header.seq)->setText(this->m_seqToCM.value(gram->header.seq)->text() + QStringLiteral(u" (成功发送/已经中断)"));
    this->m_sendFileState = NotSendingFileState;
}

void BTP::handleFileInfoNotConnected()
{
    this->m_seqToCM.value(this->m_sendFileSeq)->setText(this->m_seqToCM.value(this->m_sendFileSeq)->text() + QStringLiteral(u" (对方不在线,发送失败)"));
}

void BTP::handleSendFileNotConnected()
{
    this->m_seqToCM.value(this->m_sendFileSeq)->setText(this->m_seqToCM.value(this->m_sendFileSeq)->text() + QStringLiteral(u" (对方不在线,发送失败)"));
    this->m_sendFileState = NotSendingFileState;
    this->m_sendFileContent.clear();
}

void BTP::handleFileEndNotConnected()
{
    this->m_seqToCM.value(this->m_sendFileSeq)->setText(this->m_seqToCM.value(this->m_sendFileSeq)->text() + QStringLiteral(u" (发送过程中对方不在线,发送失败)"));
    this->m_sendFileState = NotSendingFileState;
    this->m_sendFileContent.clear();
}

void BTP::handleAcceptFile(Gram *gram)
{
    this->m_seqToCM.value(gram->header.seq)->setText(this->m_seqToCM.value(gram->header.seq)->text() + QStringLiteral(u"(开始发送)"));
    this->sendFileContent(this->m_sendFileUsername);
    this->sendFileEnd(this->m_sendFileUsername);
}

void BTP::handleRejectFile(Gram *gram)
{
    this->m_seqToCM.value(this->m_sendFileSeq)->setText(this->m_seqToCM.value(this->m_sendFileSeq)->text() + QStringLiteral(u" (对方拒绝,发送失败)"));
    this->m_sendFileState = NotSendingFileState;
    this->m_sendFileContent.clear();
}

void BTP::handleResponseAcceptFile(Gram *gram)
{
    this->setState(IdleState);
}

void BTP::handleResponseRejectFile(Gram *gram)
{
    this->setState(IdleState);
}

void BTP::handleSetConfigOk(Gram *gram)
{
    this->setState(BTPState::IdleState);
}

void BTP::sendMessage(const QString &username, const QString &text)
{
    qDebug() << "执行 sendMessage()";

    Gram messageGram;
    messageGram.header.type = MsgType::MSG_SEND_MESSAGE;
    messageGram.header.seq = m_seq++;

    ChatMessage *newCM = new ChatMessage(this->currentChat()->friendfriend(), ChatMessage::TextMessage, QDateTime::currentDateTime(), text, nullptr, 0.0);

    this->currentChat()->msgModel()->appendChatMessage(newCM);
    this->m_seqToCM.insert(m_seq - 1, newCM);

    auto gbkReceiver = gbk->fromUnicode(username);
    auto gbkText = gbk->fromUnicode(text);

    messageGram.header.length = 32 + gbkText.size();

    messageGram.data.resize(messageGram.header.length);
    messageGram.data.fill('\0');



    memcpy_s(messageGram.data.data(), messageGram.header.length, gbkReceiver.constData(), gbkReceiver.size());
    memcpy_s(messageGram.data.data() + 32, gbkText.size(), gbkText.constData(), gbkText.size());

    qDebug() << "发送了消息报文";
    this->m_sm->sendGram(&messageGram);
}

void BTP::requestSendFile(const QString &username, const QString &filename_, const QUrl &fileUrl, qint32 size)
{
    qDebug() << "执行 requestSendFile()";

    Gram rsGram;
    rsGram.header.type = MsgType::MSG_FILE_INFO;
    rsGram.header.seq = m_seq++;


    QFile file(fileUrl.toLocalFile());
    file.open(QIODevice::ReadOnly);
    QByteArray allQBA = file.readAll();

    size = allQBA.size();
    QString filename = fileUrl.fileName();

    ChatMessage *newCM = new ChatMessage(this->currentChat()->friendfriend(), ChatMessage::FileMessage, QDateTime::currentDateTime(), QStringLiteral(u"[文件: %1](%2字节)").arg(filename).arg(size), nullptr, 0.0);


    this->currentChat()->msgModel()->appendChatMessage(newCM);

    this->m_sendFileState = WantToSendFileState;
    this->m_sendFileSeq = rsGram.header.seq;
    this->m_sendFileUrl = fileUrl;
    this->m_seqToCM.insert(rsGram.header.seq, newCM);
    this->m_sendFileUsername = username;

    rsGram.header.length = 32 + 32 + 4;

    rsGram.data.resize(rsGram.header.length);
    rsGram.data.fill('\0');

    auto gbkReceiver = gbk->fromUnicode(username);
    auto gbkFilename = gbk->fromUnicode(filename);

    memcpy_s(rsGram.data.data(), rsGram.header.length, gbkReceiver.constData(), gbkReceiver.size());
    memcpy_s(rsGram.data.data() + 32, rsGram.header.length - 32, gbkFilename.constData(), gbkFilename.size());

    QByteArray filesizeQBA;
    QDataStream filesizeQDS(&filesizeQBA, QIODevice::OpenModeFlag::WriteOnly);

    filesizeQDS << size;

    memcpy_s(rsGram.data.data() + 64, rsGram.header.length - 64, filesizeQBA.constData(), filesizeQBA.size());

    qDebug() << "发送了请求发文件报文";
    this->m_sm->sendGram(&rsGram);
}

void BTP::sendFileContent(const QString &username)
{
    qDebug() << "执行 sendFileContent()";

    Gram contentGram;
    contentGram.header.type = MsgType::MSG_SEND_FILE;
    contentGram.header.seq = this->m_sendFileSeq;

    QFile file(this->m_sendFileUrl.toLocalFile());
    file.open(QIODevice::ReadOnly);
    QByteArray allQBA = file.readAll();
    auto gbkReceiver = gbk->fromUnicode(username);

    while (true)
    {
        if (allQBA.size() < 1024)
            contentGram.header.length = 32 + allQBA.size();
        else {
            contentGram.header.length = 32 + 1024;
        }

        contentGram.data.resize(contentGram.header.length);
        contentGram.data.fill('\0');

        memcpy_s(contentGram.data.data(), contentGram.header.length, gbkReceiver.constData(), gbkReceiver.size());
        memcpy_s(contentGram.data.data() + 32, contentGram.header.length - 32, allQBA.constData(), contentGram.header.length - 32);

        qDebug() << "发送了文件内容报文";
        this->m_sm->sendGram(&contentGram);

        allQBA.remove(0, contentGram.header.length - 32);
        if (allQBA.size() == 0)
            break;
    }
}

void BTP::sendFileEnd(const QString &username)
{
    qDebug() << "执行 sendFileEnd()";

    Gram efGram;
    efGram.header.type = MsgType::MSG_FILE_END;
    efGram.header.seq = this->m_sendFileSeq;

    this->m_sendFileState = EndSendingFileState;
    this->m_sendFileContent.clear();

    efGram.header.length = 32 ;

    efGram.data.resize(efGram.header.length);
    efGram.data.fill('\0');

    auto gbkReceiver = gbk->fromUnicode(username);

    memcpy_s(efGram.data.data(), efGram.header.length, gbkReceiver.constData(), gbkReceiver.size());

    qDebug() << "发送了结束发文件报文";
    this->m_sm->sendGram(&efGram);
}

void BTP::login()
{
    qDebug() << "执行 login()";
    this->setState(LoggingInState);

    Gram loginGram;
    loginGram.header.type = MsgType::MSG_LOGIN;
    loginGram.header.seq = m_seq++;
    loginGram.header.length = 64;

    loginGram.data.resize(loginGram.header.length);
    loginGram.data.fill('\0');

    auto gbkUsername = gbk->fromUnicode(this->m_username);
    auto gbkPassword = gbk->fromUnicode(this->m_password);

    memcpy_s(loginGram.data.data(), 64, gbkUsername.constData(), 32);
    memcpy_s(loginGram.data.data() + 32, 32, gbkPassword.constData(), 32);

    qDebug() << "发送了登录报文";
    this->m_sm->sendGram(&loginGram);
}

QString BTP::hint() const
{
    return this->m_hint;
}

void BTP::setHint(const QString &hint)
{
    this->m_hint = hint;
    emit hintChanged();
}

void BTP::rejectFile(const QString &username, quint32 seq)
{
    qDebug() << "执行 rejectFile()";

    Gram rjFile;
    rjFile.header.type = MsgType::MSG_R_REJECT_FILE;
    rjFile.header.seq = seq == -1 ? this->m_receiveFileSeq : seq;

    rjFile.header.length = 32;
    rjFile.data.resize(rjFile.header.length);
    rjFile.data.fill('\0');

    QString username_;
    if (username == "")
        username_ = this->m_receiveFileChatMessage->friendfriend()->username();
    else {
        username_ = username;
    }

    auto gbkSender = gbk->fromUnicode(username_);

    memcpy_s(rjFile.data.data(), rjFile.header.length, gbkSender.constData(), gbkSender.size());

    qDebug() << "发送了拒绝文件报文";
    this->m_sm->sendGram(&rjFile);
    this->setState(AcceptingRejectingFileState);

    this->handleResponseAcceptFile(nullptr);
}

void BTP::acceptFile(const QString &username, quint32 seq)
{
    qDebug() << "执行 acceptFile()";

    Gram rjFile;
    rjFile.header.type = MsgType::MSG_R_ACCEPT_FILE;
    rjFile.header.seq = seq == -1 ? this->m_receiveFileSeq : seq;

    rjFile.header.length = 32;
    rjFile.data.resize(rjFile.header.length);
    rjFile.data.fill('\0');

    QString username_;
    if (username == "")
        username_ = this->m_receiveFileChatMessage->friendfriend()->username();
    else {
        username_ = username;
    }

    auto gbkSender = gbk->fromUnicode(username_);

    memcpy_s(rjFile.data.data(), rjFile.header.length, gbkSender.constData(), gbkSender.size());

    qDebug() << "发送了接受文件报文";
    this->m_sm->sendGram(&rjFile);

    this->m_receiveFileState = ReceivingFileState;

    this->setState(AcceptingRejectingFileState);
    this->handleResponseAcceptFile(nullptr);
}

void BTP::setConfig(QString msgNum)
{
    qDebug() << "执行 setConfig()";
    this->setState(SettingState);

    Gram gram;
    gram.header.type = MsgType::MSG_SET_CONFIG;
    gram.header.seq = m_seq++;
    gram.header.length = 300;

    gram.data.resize(gram.header.length);
    gram.data.fill('\0');

    auto numStr = gbk->fromUnicode(msgNum);

    memcpy_s(gram.data.data(), 100, numStr.constData(), numStr.size());
    qDebug() << "发送了config报文";
    this->m_sm->sendGram(&gram);
}

