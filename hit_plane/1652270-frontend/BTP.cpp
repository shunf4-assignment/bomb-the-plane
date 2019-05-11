#define _CRT_SECURE_NO_WARNINGS
#include "BTP.h"
#include <QMetaEnum>
#include <QTextCodec>
#include <QFile>
#include <QJSValue>
#include <QtMath>
#include <QHash>

#define memcpy_s(a,b,c,d) memcpy(a,c,d)

static QTextCodec *gbk = QTextCodec::codecForName("GBK");

template <typename T>
static QString enumToQStr(T e)
{
    return QString::number(static_cast<typename std::underlying_type<T>::type>(e));
}

BTP::BTP(QObject *parent) : QObject(parent)
{
    this->m_state = State::Logout;
    this->m_uiState = static_cast<UIState>(0);
    this->m_map = new BTPMapModel(this);
    this->m_oMap = new BTPMapModel(this);
    this->m_currMap = new BTPMapModel(this);
    this->m_friends = new FriendsModel(this);

    this->m_popupHint[QString::number(underlie(UIState::TPopupInitiaInvit))] = QStringLiteral(u"");
    this->m_popupHint[QString::number(underlie(UIState::TPopupOpAction))] = QStringLiteral(u"");
    this->m_popupHint[QString::number(underlie(UIState::TPopupChat))] = QStringLiteral(u"");
    this->m_popupHint[QString::number(underlie(UIState::TPopupWinLose))] = QStringLiteral(u"");
    this->m_popupHint[QString::number(underlie(UIState::TPopupInvitResult))] = QStringLiteral(u"");
    this->m_popupHint[QString::number(underlie(UIState::TPopupDeployError))] = QStringLiteral(u"");
    this->m_popupHint[QString::number(underlie(UIState::TPopupActionResult))] = QStringLiteral(u"");
    this->m_popupHint[QString::number(underlie(UIState::CPopupAccInvit))] = QStringLiteral(u"");
    this->m_popupHint[QString::number(underlie(UIState::CPopupLogin))] = QStringLiteral(u"");
    this->m_popupHint[QString::number(underlie(UIState::CPopupWaitRespBusy))] = QStringLiteral(u"");
    this->m_popupHint[QString::number(underlie(UIState::CPopupWaitDeployBusy))] = QStringLiteral(u"");
    this->m_popupHint[QString::number(underlie(UIState::TPopupError))] = QStringLiteral(u"");

    setUIState(UIState::MapCampOurSide | UIState::MapModeBomb);
    connect((m_map), SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &, const QVector<int> &)), (m_currMap), SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &, const QVector<int> &)));

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

    periodicalTimer->start(2000);

    connect(this, SIGNAL(reconnRemainingTimeChanged()), this, SLOT(onReconnRemainingTimeChanged()));


}

BTP::~BTP()
{
}


// Slots

void BTP::onConnected()
{
    m_socketStateText[1] = m_sm->getPeerAddress();
    emit socketStateTextChanged();

    if (+(m_state & (State::Logout | State::CliWaitLogin)))
    {
        triggerEvent(Event::EvLogin, QVariant::fromValue(nullptr));
    }
}

void BTP::onSocketError(QAbstractSocket::SocketError err)
{
    m_socketStateText[0] = QStringLiteral(u"出错: %1").arg(QMetaEnum::fromType<QAbstractSocket::SocketError>().valueToKey(err));
    emit socketStateTextChanged();

    if (!(state() & State::CliNotLogin))
        this->setState(State::Logout);
}

void BTP::onSocketStateChanged(QAbstractSocket::SocketState state)
{
    m_socketStateText[0] = QStringLiteral(u"%1").arg(QMetaEnum::fromType<QAbstractSocket::SocketState>().valueToKey(state));
    emit socketStateTextChanged();
}

void BTP::onGramArrived(Gram *gram)
{
    qDebug() << "接收: (Len)" << gram->header.length << "(Type)" << QtEnumToString(gram->type) << gram->data.toPercentEncoding();
    QByteArray ba = gram->toByteArray();

    switch (gram->type)
    {
    case MsgType::Null:
    case MsgType::Reset:
    case MsgType::Login:
    case MsgType::Invite:
    case MsgType::End:
    case MsgType::Accept:
    case MsgType::Refuse:
    case MsgType::Deploy:
    case MsgType::Bomb:
    case MsgType::Guess:
    case MsgType::Win:
    case MsgType::Lose:
    case MsgType::Chat:
    case MsgType::Status:
    case MsgType::RLoginOk:
    case MsgType::RInviteOk:
    case MsgType::REnd:
    case MsgType::RAccept:
    case MsgType::RRefuse:
    case MsgType::RDeployOk:
    case MsgType::RDeployError:
    case MsgType::RBomb:
    case MsgType::RGuess:

    case MsgType::RLoginUsernameError:
    case MsgType::RLoginPasswordError:
    case MsgType::RInviteErrorLogout:
    case MsgType::RInviteErrorBusy:
    case MsgType::RStatus:
        triggerEvent(Event::EvGramArrived, QVariant::fromValue(gram));
        break;
    default:

        m_popupHint[QString::number(static_cast<quint32>(UIState::TPopupError))] = QStringLiteral(u"收到的报文格式不正确. 将尝试重新连接服务器.");
        popupHintChanged();
        reset(true);
    }

}

void BTP::onPeriodicalRefresh()
{
    emit reconnRemainingTimeChanged();

    if (!(state() & (State::CliNotLogin))) {
        Gram g;
        g.header.length = 4;

        QDataStream dataStream(&g.data, QIODevice::OpenModeFlag::WriteOnly);
        dataStream << underlie(MsgType::Status);
        Q_ASSERT(static_cast<quint32>(g.data.size()) == g.header.length);

        qDebug() << "发送了Status报文";
        this->m_sm->sendGram(&g);
    }

}

void BTP::onReconnRemainingTimeChanged()
{
    qreal rt = this->reconnRemainingTime();
    if (qAbs(rt) < 1e-5)
        m_socketStateText[2] = "";
    else
        m_socketStateText[2] = QStringLiteral(u"重连时间:%1s").arg(QString::number(this->reconnRemainingTime(), 'g', 2));
    emit socketStateTextChanged();
}


// Methods


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

State BTP::state() const
{
    return this->m_state;
}

void BTP::setState(State state)
{
    this->m_state = state;
    qDebug() << "状态变为 " << getStateText(state);
    emit stateChanged();
}

BTP::UIState BTP::uiState() const
{
    return this->m_uiState;
}

void BTP::setUIState(UIState uiState)
{
    qDebug() << "UI状态变为 " << uiState;
    this->m_uiState = uiState;
    emit uiStateChanged();
}

BTPMapModel *BTP::map() const
{
    return this->m_map;
}

BTPMapModel *BTP::oMap() const
{
    return this->m_oMap;
}

BTPMapModel *BTP::currMap() const
{
    return this->m_currMap;
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


QString BTP::username() const
{
    return this->m_username;
}

void BTP::setUsername(const QString &username)
{
    this->m_username = username;
    emit usernameChanged();
}

QString BTP::opName() const
{
    return this->m_opName;
}

void BTP::setOpName(const QString &opName)
{
    this->m_opName = opName;
    emit opNameChanged();
}

QString BTP::password() const
{
    return this->m_password;
}

void BTP::setPassword(const QString &password)
{
    this->m_password = password;
    emit passwordChanged();
}

const QVariantMap &BTP::popupHint() const
{
    return this->m_popupHint;
}

void BTP::setPopupHint(const QVariantMap &popupHint)
{
    this->m_popupHint = popupHint;
    emit popupHintChanged();
}

QString BTP::mapHint1() const
{
    return this->m_mapHint1;
}

void BTP::setMapHint1(const QString &mapHint1)
{
    this->m_mapHint1 = mapHint1;
    emit mapHint1Changed();
}

QString BTP::mapHint2() const
{
    return this->m_mapHint2;
}

void BTP::setMapHint2(const QString &mapHint2)
{
    this->m_mapHint2 = mapHint2;
    emit mapHint2Changed();
}

quint8 BTP::mapPos1() const
{
    return this->m_mapPos1;
}

void BTP::setMapPos1(quint8 mapPos1)
{
    this->m_mapPos1 = mapPos1;
    emit mapPos1Changed();
}

quint8 BTP::mapPos2() const
{
    return this->m_mapPos2;
}

void BTP::setMapPos2(quint8 mapPos2)
{
    this->m_mapPos2 = mapPos2;
    emit mapPos2Changed();
}

QVariantList BTP::headTails() const
{
     return QVariantList{m_oh1, m_ot1, m_oh2, m_ot2, m_oh3, m_ot3, m_h1, m_t1, m_h2, m_t2, m_h3, m_t3};
}

void BTP::setHeadTails(int i, const QVariant &q)
{
    (i ==  0) && (m_oh1 = q.value<quint8>());
    (i ==  1) && (m_ot1 = q.value<quint8>());
    (i ==  2) && (m_oh2 = q.value<quint8>());
    (i ==  3) && (m_ot2 = q.value<quint8>());
    (i ==  4) && (m_oh3 = q.value<quint8>());
    (i ==  5) && (m_ot3 = q.value<quint8>());
    (i ==  6) && (m_h1 = q.value<quint8>());
    (i ==  7) && (m_t1 = q.value<quint8>());
    (i ==  8) && (m_h2 = q.value<quint8>());
    (i ==  9) && (m_t2 = q.value<quint8>());
    (i == 10) && (m_h3 = q.value<quint8>());
    (i == 11) && (m_t3 = q.value<quint8>());

    emit headTailsChanged();
}

void BTP::setHeadTails(const QVariantList &qv)
{
    m_oh1 = qv.at(0).value<quint8>();
    m_ot1 = qv.at(1).value<quint8>();
    m_oh2 = qv.at(2).value<quint8>();
    m_ot2 = qv.at(3).value<quint8>();
    m_oh3 = qv.at(4).value<quint8>();
    m_ot3 = qv.at(5).value<quint8>();
    m_h1 =  qv.at(6).value<quint8>();
    m_t1 =  qv.at(7).value<quint8>();
    m_h2 =  qv.at(8).value<quint8>();
    m_t2 =  qv.at(9).value<quint8>();
    m_h3 =  qv.at(10).value<quint8>();
    m_t3 =  qv.at(11).value<quint8>();

    emit headTailsChanged();
}




void BTP::init()
{
    this->m_sm->ensureConnected();
}

void BTP::reset(bool isError)
{
    if (isError) {
        qDebug() << "致命错误";
        this->setUIState((uiState() & ~UIState::TPopupMask) | UIState::TPopupError);
    }
    this->m_sm->errorAndDisconnect();
    this->m_sm->onError(QAbstractSocket::SocketError::UnknownSocketError);
}

void BTP::afterError()
{
    // this->setState(State::Logout);
}

void BTP::switchMap()
{
    UIState nus;
    if (+(this->uiState() & UIState::MapCampOurSide)) {
        switchMapTo(UIState::MapCampTheirSide);
    } else {
        switchMapTo(UIState::MapCampOurSide);
    }
}

void BTP::switchMapTo(UIState side)
{
    UIState nus;
    nus = this->uiState() & ~UIState::MapCampMask | side;

    if (+(side & UIState::MapCampOurSide)) {
        disconnect(static_cast<QAbstractItemModel *>(m_oMap), SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &, const QVector<int> &)), m_currMap, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &, const QVector<int> &)));
        connect(static_cast<QAbstractItemModel *>(m_map), SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &, const QVector<int> &)), m_currMap, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &, const QVector<int> &)));
    } else {
        disconnect(static_cast<QAbstractItemModel *>(m_map), SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &, const QVector<int> &)), m_currMap, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &, const QVector<int> &)));
        connect(static_cast<QAbstractItemModel *>(m_oMap), SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &, const QVector<int> &)), m_currMap, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &, const QVector<int> &)));
    }

    this->setUIState(nus);
    this->syncMap();
}

void BTP::switchToBomb()
{
    UIState nus;
    nus = this->uiState() & ~UIState::MapModeMask | UIState::MapModeBomb;
    this->setUIState(nus);
    setMapPos1(Coord_None);
    setMapPos2(Coord_None);
}

void BTP::switchToGuess()
{
    UIState nus;
    nus = this->uiState() & ~UIState::MapModeMask | UIState::MapModeGuess;
    this->setUIState(nus);
    setMapPos1(Coord_None);
    setMapPos2(Coord_None);
}

QString BTP::getStateText(State state)
{
    return QString(QMetaEnum::fromType<State>().valueToKey(static_cast<int>(state)));
}

static QHash<quint32, QString> stateText = {
    {static_cast<quint32>(State::Logout), QStringLiteral(u"离线")},
    {static_cast<quint32>(State::Idle), QStringLiteral(u"空闲")},
    {static_cast<quint32>(State::P1WaitAcc), QStringLiteral(u"等待别人同意邀请")},
    {static_cast<quint32>(State::P2WaitAcc), QStringLiteral(u"正在考虑是否接受邀请")},
    {static_cast<quint32>(State::Deploy), QStringLiteral(u"游戏中")},
    {static_cast<quint32>(State::WaitDeploy), QStringLiteral(u"游戏中")},
    {static_cast<quint32>(State::MyTurn), QStringLiteral(u"游戏中")},
    {static_cast<quint32>(State::WaitOp), QStringLiteral(u"游戏中")},
    {static_cast<quint32>(State::OpDeployed), QStringLiteral(u"游戏中")},
};

QString BTP::getStateText_friendly_int(quint32 state)
{
    return stateText.value(state);
}






/*
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
}*/


static QString getQStrFromCStr(Gram *g, const char **pp, bool mustNullTerminate)
{
    const char *&p = *pp;
    const char *q = p;
    while (*q && q - g->data.constData() < g->header.length)
        q++;

    if (q - g->data.constData() == g->header.length && mustNullTerminate) {
        return QString::Null();
    }
    QString ret = QString::fromLocal8Bit(p, static_cast<int>(q - p));
    *pp = q + 1;
    return ret;
}


// Core logic
void BTP::triggerEvent(Event ev, const QVariant &param)
{

    QVariant temp;
    const QVariant *unpackedP;

    if (param.userType() == qMetaTypeId<QJSValue>()) {
        temp = param.value<QJSValue>().toVariant();
        unpackedP = &temp;
    } else {
        unpackedP = &param;
    }

    const QVariant &unpacked = *unpackedP;
    Gram *g = nullptr;
    if (unpacked.userType() == qMetaTypeId<Gram *>()) {
        g = unpacked.value<Gram *>();
    }

    if (state() == State::Null) {
        // ???
        qDebug() << "错误: 状态为空";
        goto out;
    }


    if (ev == Event::EvLogout)
    {
        if (+(state() & (State::CliNotLogin)))
            goto out;

        if (+(state() & (State::InGame))) {
            Gram g;
            g.header.length = 4;

            QDataStream dataStream(&g.data, QIODevice::OpenModeFlag::WriteOnly);

            dataStream << underlie(MsgType::End);

            Q_ASSERT(static_cast<quint32>(g.data.size()) == g.header.length);

            qDebug() << "发送了End报文";
            this->m_sm->sendGram(&g);
        }

        this->setUsername("");
        setState(State::Logout);
        reset(false);
        goto out;
    }

    if (ev == Event::EvQuitGame)
    {
        if (+(state() & (State::CliNotLogin)))
            goto out;
        if (!+(state() & (State::InGame)))
            goto out;

        Gram g;
        g.header.length = 4;

        QDataStream dataStream(&g.data, QIODevice::OpenModeFlag::WriteOnly);

        dataStream << underlie(MsgType::End);

        Q_ASSERT(static_cast<quint32>(g.data.size()) == g.header.length);

        qDebug() << "发送了End报文";
        this->m_sm->sendGram(&g);

        setState(State::CliWaitEnd);
    }

    if (ev == Event::EvGramArrived)
    {
        if (g->type == MsgType::Reset)
        {
            // 等同于注销
            this->setUsername("");
            m_popupHint[enumToQStr(UIState::TPopupError)] = QStringLiteral(u"你被强制下线。");
            popupHintChanged();
            reset(true);
            goto out;
        }

        if (g->type == MsgType::RStatus)
        {
            if (+(state() & (State::CliNotLogin)))
                goto out;
            handleStatusGram(g);
            goto out;
        }
    }

/////////

    if (+(state() & (State::Logout | State::CliUsernameError | State::CliPasswordError))) {
        switch (ev) {
        case Event::EvGramArrived:
            switch(g->type) {
            default:
                qDebug() << "错误: 状态为" << QtEnumToString(state()) << "但却接收到了" << QtEnumToString(g->type);
                goto out;
            };

        case Event::EvLogin:
        loginEntry:
            if (unpacked.userType() != QMetaType::Nullptr) {
                setUsername(unpacked.value<QVariantList>().at(0).value<QString>());
                setPassword(unpacked.value<QVariantList>().at(1).value<QString>());
            }

            if (this->m_username.isEmpty() || this->m_username.isNull()) {
                qDebug() << "登录名还未设置，不会登录";
                goto out;
            } else {
                qDebug() << "登录";
                this->setState(State::CliWaitLogin);

                if (m_sm->getState() != QAbstractSocket::ConnectedState)
                    goto out;

                Gram loginGram;
                loginGram.header.length = 68;

                QDataStream dataStream(&loginGram.data, QIODevice::OpenModeFlag::WriteOnly);

                dataStream << underlie(MsgType::Login);



                auto gbkUsername = gbk->fromUnicode(this->m_username).leftJustified(32, '\0', true);
                auto gbkPassword = gbk->fromUnicode(this->m_password).leftJustified(32, '\0', true);

                dataStream.writeRawData(gbkUsername.constData(), gbkUsername.length());
                dataStream.writeRawData(gbkPassword.constData(), gbkPassword.length());

                qDebug() << loginGram.data.size() << loginGram.data;

                Q_ASSERT(static_cast<quint32>(loginGram.data.size()) == loginGram.header.length);

                qDebug() << "发送了登录报文";
                this->m_sm->sendGram(&loginGram);
                goto out;
            }

        case Event::EvLogout:
            this->setUsername("");
            reset(false);
            setState(State::Logout);
            goto out;

        default:
            qDebug() << "错误: 状态为" << QtEnumToString(state()) << "但却接收到了" << QtEnumToString(ev);
            goto out;
        }
    }


//////////



    if (state() == State::Idle) {
        switch (ev) {
        case Event::EvGramArrived:
            switch(g->type) {

            case MsgType::Invite:
                // 被邀请
            {
                const char *p = g->data.constData() + sizeof(MsgType);

                QString fromUser = getQStrFromCStr(g, &p, false);
                setOpName(fromUser);
                m_popupHint[enumToQStr(UIState::CPopupAccInvit)] = QVariant::fromValue(fromUser + QStringLiteral(u" 邀请你玩游戏。"));
                popupHintChanged();
                setState(State::P2WaitAcc);
                goto out;
            }

            default:
                qDebug() << "错误: 状态为" << QtEnumToString(state()) << "但却接收到了" << QtEnumToString(g->type);
                goto out;
            };

        case Event::EvInvite:
        {
            Gram g;

            QDataStream dataStream(&g.data, QIODevice::OpenModeFlag::WriteOnly);

            dataStream << underlie(MsgType::Invite);

            dataStream.writeRawData(friends()->getData().at(unpacked.value<QVariantList>().at(0).toInt())->username().toLocal8Bit().constData(), friends()->getData().at(unpacked.value<QVariantList>().at(0).toInt())->username().toLocal8Bit().length());

            setOpName(friends()->getData().at(unpacked.value<QVariantList>().at(0).toInt())->username());

            g.header.length = static_cast<quint32>(g.data.length());

            qDebug() << "发送了 Invite 报文";
            this->m_sm->sendGram(&g);

            setState(State::CliWaitRInvit);
            goto out;
        }

        default:
            qDebug() << "错误: 状态为" << QtEnumToString(state()) << "但却接收到了" << QtEnumToString(ev);
            goto out;
        }
    }

////////////

    if (state() == State::P1WaitAcc) {
        switch (ev) {
        case Event::EvGramArrived:
            switch(g->type) {


            case MsgType::Accept:
                // 对方接受
            {
                m_popupHint[enumToQStr(UIState::TPopupInvitResult)] = opName() + QStringLiteral(u" 接受了你的邀请。");
                popupHintChanged();
                setHeadTails({Coord_None,Coord_None,Coord_None,Coord_None,Coord_None,Coord_None,Coord_None,Coord_None,Coord_None,Coord_None,Coord_None,Coord_None});
                QVector<Grid> emptyMap(100);
                gameLog = "======" + username() + " 和 " + opName() + " 的游戏 ======\n";
                gameLogChanged();

                map()->setDataByBTPMapGridTypeList(emptyMap);
                setState(State::Deploy);
                switchMapTo(UIState::MapCampOurSide);

                setUIState(uiState() & ~UIState::TPopupMask | UIState::TPopupInvitResult);
                goto out;
            }

            case MsgType::Refuse:
                // 对方拒绝
            {
                m_popupHint[enumToQStr(UIState::TPopupInvitResult)] = opName() + QStringLiteral(u" 拒绝了你的邀请。");
                popupHintChanged();
                setState(State::Idle);
                setUIState(uiState() & ~UIState::TPopupMask | UIState::TPopupInvitResult);
                goto out;
            }

            case MsgType::End:
                // 出状况了
            {
                m_popupHint[enumToQStr(UIState::TPopupWinLose)] = opName() + QStringLiteral(u" 逃走了。");
                popupHintChanged();
                setState(State::Idle);
                setUIState(uiState() & ~UIState::TPopupMask | UIState::TPopupWinLose);
                goto out;
            }


            case MsgType::RInviteErrorLogout:

            {
                m_popupHint[enumToQStr(UIState::TPopupInvitResult)] = opName() + QStringLiteral(u" 不在线。");
                popupHintChanged();
                setState(State::Idle);
                setUIState(uiState() & ~UIState::TPopupMask | UIState::TPopupInvitResult);
                goto out;
            }

            case MsgType::RInviteErrorBusy:

            {
                m_popupHint[enumToQStr(UIState::TPopupInvitResult)] = opName() + QStringLiteral(u" 正忙。");
                popupHintChanged();
                setState(State::Idle);
                setUIState(uiState() & ~UIState::TPopupMask | UIState::TPopupInvitResult);
                goto out;
            }


            default:
                qDebug() << "错误: 状态为" << QtEnumToString(state()) << "但却接收到了" << QtEnumToString(g->type);
                goto out;
            };

        case Event::EvCancelInvit:
        case Event::EvQuitGame:
        {
            Gram g;
            g.header.length = 4;

            QDataStream dataStream(&g.data, QIODevice::OpenModeFlag::WriteOnly);

            dataStream << underlie(MsgType::End);

            Q_ASSERT(static_cast<quint32>(g.data.size()) == g.header.length);

            qDebug() << "发送了End报文";
            this->m_sm->sendGram(&g);

            setState(State::CliWaitEnd);
            goto out;
        }

        default:
            qDebug() << "错误: 状态为" << QtEnumToString(state()) << "但却接收到了" << QtEnumToString(ev);
            goto out;
        }
    }


    ////////////

    if (state() == State::P2WaitAcc) {
        switch (ev) {
        case Event::EvGramArrived:
            switch(g->type) {

            case MsgType::End:
                // 出状况了
            {
                m_popupHint[enumToQStr(UIState::TPopupWinLose)] = opName() + QStringLiteral(u" 逃走了。");
                popupHintChanged();
                setState(State::Idle);
                setUIState(uiState() & ~UIState::TPopupMask | UIState::TPopupWinLose);
                goto out;
            }


            default:
                qDebug() << "错误: 状态为" << QtEnumToString(state()) << "但却接收到了" << QtEnumToString(g->type);
                goto out;
            };

        case Event::EvAcceptInvit:
        {
            Gram g;
            g.header.length = 4;
            QDataStream dataStream(&g.data, QIODevice::OpenModeFlag::WriteOnly);

            dataStream << underlie(MsgType::Accept);

            Q_ASSERT(static_cast<quint32>(g.data.size()) == g.header.length);

            qDebug() << "发送了Accept报文";
            this->m_sm->sendGram(&g);

            setState(State::CliWaitRAcc);
            goto out;
        }

        case Event::EvRefuseInvit:
        {
            Gram g;
            g.header.length = 4;
            QDataStream dataStream(&g.data, QIODevice::OpenModeFlag::WriteOnly);

            dataStream << underlie(MsgType::Refuse);

            Q_ASSERT(static_cast<quint32>(g.data.size()) == g.header.length);

            qDebug() << "Refuse";
            this->m_sm->sendGram(&g);

            setState(State::CliWaitRAcc);
            goto out;
        }

        case Event::EvCancelInvit:
        case Event::EvQuitGame:
        {
            Gram g;
            g.header.length = 4;

            QDataStream dataStream(&g.data, QIODevice::OpenModeFlag::WriteOnly);

            dataStream << underlie(MsgType::End);

            Q_ASSERT(static_cast<quint32>(g.data.size()) == g.header.length);

            qDebug() << "发送了End报文";
            this->m_sm->sendGram(&g);

            setState(State::CliWaitEnd);
            goto out;
        }

        default:
            qDebug() << "错误: 状态为" << QtEnumToString(state()) << "但却接收到了" << QtEnumToString(ev);
            goto out;
        }
    }

    ////////////

    if (state() == State::Deploy) {
        switch (ev) {
        case Event::EvGramArrived:
            switch(g->type) {

            case MsgType::Deploy:
            {
                // 对方部署完毕
                setState(State::OpDeployed);
                goto out;
            }

            case MsgType::End:
                // 出状况了
            {
                m_popupHint[enumToQStr(UIState::TPopupWinLose)] = opName() + QStringLiteral(u" 逃走了。");
                popupHintChanged();
                setState(State::Idle);
                setUIState(uiState() & ~UIState::TPopupMask | UIState::TPopupWinLose);
                goto out;
            }


            default:
                qDebug() << "错误: 状态为" << QtEnumToString(state()) << "但却接收到了" << QtEnumToString(g->type);
                goto out;
            };

        case Event::EvDeploy:
        {
            Gram g;
            g.header.length = 10;
            QDataStream dataStream(&g.data, QIODevice::OpenModeFlag::WriteOnly);

            dataStream << underlie(MsgType::Deploy);

            dataStream << m_h1 << m_t1 << m_h2 << m_t2 <<m_h3 <<m_t3;

            Q_ASSERT(static_cast<quint32>(g.data.size()) == g.header.length);

            qDebug() << "发送了 Deploy 报文";
            this->m_sm->sendGram(&g);

            setState(State::CliWaitRDeploy);
            goto out;
        }

        case Event::EvQuitGame:
        {
            Gram g;
            g.header.length = 4;

            QDataStream dataStream(&g.data, QIODevice::OpenModeFlag::WriteOnly);

            dataStream << underlie(MsgType::End);

            Q_ASSERT(static_cast<quint32>(g.data.size()) == g.header.length);

            qDebug() << "发送了End报文";
            this->m_sm->sendGram(&g);

            setState(State::CliWaitEnd);
            goto out;
        }

        default:
            qDebug() << "错误: 状态为" << QtEnumToString(state()) << "但却接收到了" << QtEnumToString(ev);
            goto out;
        }
    }

    ////////////

    if (state() == State::WaitDeploy) {
        switch (ev) {
        case Event::EvGramArrived:
            switch(g->type) {

            case MsgType::Deploy:
            {
                // 对方部署完毕
                // 开始游戏
                switchMapTo(UIState::MapCampTheirSide);
                setMapPos1(Coord_None);
                setMapPos2(Coord_None);
                setState(State::MyTurn);
                goto out;
            }

            case MsgType::End:
                // 出状况了
            {
                m_popupHint[enumToQStr(UIState::TPopupWinLose)] = opName() + QStringLiteral(u" 逃走了。");
                popupHintChanged();
                setState(State::Idle);
                setUIState(uiState() & ~UIState::TPopupMask | UIState::TPopupWinLose);
                goto out;
            }


            default:
                qDebug() << "错误: 状态为" << QtEnumToString(state()) << "但却接收到了" << QtEnumToString(g->type);
                goto out;
            };


        case Event::EvQuitGame:
        {
            Gram g;
            g.header.length = 4;

            QDataStream dataStream(&g.data, QIODevice::OpenModeFlag::WriteOnly);

            dataStream << underlie(MsgType::End);

            Q_ASSERT(static_cast<quint32>(g.data.size()) == g.header.length);

            qDebug() << "发送了End报文";
            this->m_sm->sendGram(&g);

            setState(State::CliWaitEnd);
            goto out;
        }

        default:
            qDebug() << "错误: 状态为" << QtEnumToString(state()) << "但却接收到了" << QtEnumToString(ev);
            goto out;
        }
    }


    ////////////

    if (state() == State::MyTurn) {
        switch (ev) {
        case Event::EvGramArrived:
            switch(g->type) {

            case MsgType::End:
                // 出状况了
            {
                m_popupHint[enumToQStr(UIState::TPopupWinLose)] = opName() + QStringLiteral(u" 逃走了。");
                popupHintChanged();
                setState(State::Idle);
                setUIState(uiState() & ~UIState::TPopupMask | UIState::TPopupWinLose);
                goto out;
            }


            default:
                qDebug() << "错误: 状态为" << QtEnumToString(state()) << "但却接收到了" << QtEnumToString(g->type);
                goto out;
            };

        case Event::EvBomb:
        {
            Gram g;
            g.header.length = 5;
            QDataStream dataStream(&g.data, QIODevice::OpenModeFlag::WriteOnly);

            dataStream << underlie(MsgType::Bomb);

            dataStream << m_mapPos1;

            Q_ASSERT(static_cast<quint32>(g.data.size()) == g.header.length);

            qDebug() << "发送了 Bomb 报文";
            this->m_sm->sendGram(&g);

            setState(State::CliWaitRBomb);
            goto out;
        }

        case Event::EvGuess:
        {
            Gram g;
            g.header.length = 6;
            QDataStream dataStream(&g.data, QIODevice::OpenModeFlag::WriteOnly);

            dataStream << underlie(MsgType::Guess);

            dataStream << m_mapPos1;

            dataStream << m_mapPos2;

            Q_ASSERT(static_cast<quint32>(g.data.size()) == g.header.length);

            qDebug() << "发送了 Guess 报文";
            this->m_sm->sendGram(&g);
            setState(State::CliWaitRGuess);
            goto out;
        }

        case Event::EvQuitGame:
        {
            Gram g;
            g.header.length = 4;

            QDataStream dataStream(&g.data, QIODevice::OpenModeFlag::WriteOnly);

            dataStream << underlie(MsgType::End);

            Q_ASSERT(static_cast<quint32>(g.data.size()) == g.header.length);

            qDebug() << "发送了End报文";
            this->m_sm->sendGram(&g);

            setState(State::CliWaitEnd);
            goto out;
        }

        default:
            qDebug() << "错误: 状态为" << QtEnumToString(state()) << "但却接收到了" << QtEnumToString(ev);
            goto out;
        }
    }

    ////////////

    if (state() == State::OpDeployed) {
        switch (ev) {
        case Event::EvGramArrived:
            switch(g->type) {

            case MsgType::End:
                // 出状况了
            {
                m_popupHint[enumToQStr(UIState::TPopupWinLose)] = opName() + QStringLiteral(u" 逃走了。");
                popupHintChanged();
                setState(State::Idle);
                setUIState(uiState() & ~UIState::TPopupMask | UIState::TPopupWinLose);
                goto out;
            }


            default:
                qDebug() << "错误: 状态为" << QtEnumToString(state()) << "但却接收到了" << QtEnumToString(g->type);
                goto out;
            };

        case Event::EvDeploy:
        {
            Gram g;
            g.header.length = 10;
            QDataStream dataStream(&g.data, QIODevice::OpenModeFlag::WriteOnly);

            dataStream << underlie(MsgType::Deploy);

            dataStream << m_h1 << m_t1 << m_h2 << m_t2 <<m_h3 <<m_t3;

            Q_ASSERT(static_cast<quint32>(g.data.size()) == g.header.length);

            qDebug() << "发送了 Deploy 报文";
            this->m_sm->sendGram(&g);

            setState(State::CliWaitRDeployWhenOpDeployed);
            goto out;
        }

        case Event::EvQuitGame:
        {
            Gram g;
            g.header.length = 4;

            QDataStream dataStream(&g.data, QIODevice::OpenModeFlag::WriteOnly);

            dataStream << underlie(MsgType::End);

            Q_ASSERT(static_cast<quint32>(g.data.size()) == g.header.length);

            qDebug() << "发送了End报文";
            this->m_sm->sendGram(&g);

            setState(State::CliWaitEnd);
            goto out;
        }

        default:
            qDebug() << "错误: 状态为" << QtEnumToString(state()) << "但却接收到了" << QtEnumToString(ev);
            goto out;
        }
    }


    ////////////

    if (state() == State::WaitOp) {
        switch (ev) {
        case Event::EvGramArrived:
            switch(g->type) {

            case MsgType::End:
                // 出状况了
            {
                m_popupHint[enumToQStr(UIState::TPopupWinLose)] = opName() + QStringLiteral(u" 逃走了。");
                popupHintChanged();
                setState(State::Idle);
                setUIState(uiState() & ~UIState::TPopupMask | UIState::TPopupWinLose);
                goto out;
            }

            case MsgType::Bomb:
            {
                if (g->header.length != 5) {
                    qDebug() << "错误: 收到一个长为" << g->header.length << "的Bomb";
                    goto out;
                }

                m_popupHint[enumToQStr(UIState::TPopupOpAction)] = opName() + QStringLiteral(u" 轰炸了你的：") + m_map->indexToCoordStr(*reinterpret_cast<const unsigned char *>(g->data.constData() + 4));


                Grid hitG;
                hitG = m_map->bomb(*reinterpret_cast<const unsigned char *>(g->data.constData() + 4));
                m_popupHint[enumToQStr(UIState::TPopupOpAction)] = m_popupHint.value(enumToQStr(UIState::TPopupOpAction)).toString() + (bool(hitG & (Grid::Body | Grid::Tail)) ? QStringLiteral(u" 命中机身") : bool(hitG & Grid::Head) ? QStringLiteral(u" 命中机头") : QStringLiteral(u" 未中"));
                popupHintChanged();

                gameLog += opName() + QStringLiteral(u" 轰炸了 ") + username() + QStringLiteral(u" 的 ") + m_map->indexToCoordStr(*reinterpret_cast<const unsigned char *>(g->data.constData() + 4)) + (bool(hitG & (Grid::Body | Grid::Tail)) ? QStringLiteral(u" 命中机身") : bool(hitG & Grid::Head) ? QStringLiteral(u" 命中机头") : QStringLiteral(u" 未中")) + "\n";
                gameLogChanged();

                setMapPos1(Coord_None);
                setMapPos2(Coord_None);
                switchMapTo(UIState::MapCampTheirSide);
                setState(State::MyTurn);
                setUIState(uiState() & ~UIState::TPopupMask | UIState::TPopupOpAction);
                goto out;
            }

            case MsgType::Guess:
            {
                if (g->header.length != 6) {
                    qDebug() << "错误: 收到一个长为" << g->header.length << "的Guess";
                    goto out;
                }

                m_popupHint[enumToQStr(UIState::TPopupOpAction)] = opName() + QStringLiteral(u" 猜测了你某架飞机的机头和机尾：") + m_map->indexToCoordStr(*reinterpret_cast<const unsigned char *>(g->data.constData() + 4)) + QStringLiteral(u",") + m_map->indexToCoordStr(*reinterpret_cast<const unsigned char *>(g->data.constData() + 5));


                bool right = false;
                unsigned char guessh, guesst;
                guessh = *reinterpret_cast<const unsigned char *>(g->data.constData() + 4);
                guesst = *reinterpret_cast<const unsigned char *>(g->data.constData() + 5);

                (m_h1 & 0x7f) == guessh && (m_t1 & 0x7f) == guesst &&
                        (right = true);
                (m_h2 & 0x7f) == guessh && (m_t2 & 0x7f) == guesst &&
                        (right = true);
                (m_h3 & 0x7f) == guessh && (m_t3 & 0x7f) == guesst &&
                        (right = true);

                m_popupHint[enumToQStr(UIState::TPopupOpAction)] = m_popupHint.value(enumToQStr(UIState::TPopupOpAction)).toString() + (right ? QStringLiteral(u" 猜中") : QStringLiteral(u" 未猜中"));

                gameLog += opName() + QStringLiteral(u" 猜测了 ") + username() + QStringLiteral(u" 某架飞机的机头和机尾：") + m_map->indexToCoordStr(*reinterpret_cast<const unsigned char *>(g->data.constData() + 4)) + QStringLiteral(u",") + m_map->indexToCoordStr(*reinterpret_cast<const unsigned char *>(g->data.constData() + 5)) + (right ? QStringLiteral(u" 猜中") : QStringLiteral(u" 未猜中")) + "\n";
                gameLogChanged();

                popupHintChanged();
                setMapPos1(Coord_None);
                setMapPos2(Coord_None);
                setState(State::MyTurn);
                switchMapTo(UIState::MapCampTheirSide);
                setUIState(uiState() & ~UIState::TPopupMask | UIState::TPopupOpAction);
                goto out;
            }

            case MsgType::Lose:
            {
                m_popupHint[enumToQStr(UIState::TPopupOpAction)] = opName() + QStringLiteral(u" 猜中了你的最后一架飞机，你败了。");

                gameLog += opName() + QStringLiteral(u" 猜中了") + username() + QStringLiteral(u" 的最后一架飞机，") + username() + QStringLiteral(u" 败了。") + "\n";
                gameLogChanged();

                popupHintChanged();
                setState(State::Idle);
                setUIState(uiState() & ~UIState::TPopupMask | UIState::TPopupOpAction);
                goto out;
            }

            default:
                qDebug() << "错误: 状态为" << QtEnumToString(state()) << "但却接收到了" << QtEnumToString(g->type);
                goto out;
            };

        case Event::EvQuitGame:
        {
            Gram g;
            g.header.length = 4;

            QDataStream dataStream(&g.data, QIODevice::OpenModeFlag::WriteOnly);

            dataStream << underlie(MsgType::End);

            Q_ASSERT(static_cast<quint32>(g.data.size()) == g.header.length);

            qDebug() << "发送了End报文";
            this->m_sm->sendGram(&g);

            setState(State::CliWaitEnd);
            goto out;
        }

        default:
            qDebug() << "错误: 状态为" << QtEnumToString(state()) << "但却接收到了" << QtEnumToString(ev);
            goto out;
        }
    }

    ////////////

    if (state() == State::CliWaitLogin) {
        switch (ev) {
        case Event::EvGramArrived:
            switch(g->type) {

            case MsgType::RLoginOk:
            {
                setState(State::Idle);
                handleStatusGram(g);
                goto out;
            }

            case MsgType::RLoginPasswordError:
            {
                setState(State::CliPasswordError);
                goto out;
            }

            case MsgType::RLoginUsernameError:
            {
                setState(State::CliUsernameError);
                goto out;
            }


            default:
                qDebug() << "错误: 状态为" << QtEnumToString(state()) << "但却接收到了" << QtEnumToString(g->type);
                goto out;
            };

        case Event::EvLogin:
            goto loginEntry;

        default:
            qDebug() << "错误: 状态为" << QtEnumToString(state()) << "但却接收到了" << QtEnumToString(ev);
            goto out;
        }
    }

    ////////////

    if (state() == State::CliWaitEnd) {
        switch (ev) {
        case Event::EvGramArrived:
            switch(g->type) {

            case MsgType::REnd:
            {
                setState(State::Idle);
                goto out;
            }

            default:
                qDebug() << "错误: 状态为" << QtEnumToString(state()) << "但却接收到了" << QtEnumToString(g->type);
                goto out;
            };

        default:
            qDebug() << "错误: 状态为" << QtEnumToString(state()) << "但却接收到了" << QtEnumToString(ev);
            goto out;
        }
    }

    ////////////

    if (state() == State::CliWaitRAcc) {
        switch (ev) {
        case Event::EvGramArrived:
            switch(g->type) {

            case MsgType::RAccept:
            // 接受，开始部署
            {
                setHeadTails({Coord_None,Coord_None,Coord_None,Coord_None,Coord_None,Coord_None,Coord_None,Coord_None,Coord_None,Coord_None,Coord_None,Coord_None});
                gameLog = "======" + username() + " 和 " + opName() + "的游戏 ======\n";
                gameLogChanged();
                QVector<Grid> emptyMap(100);
                map()->setDataByBTPMapGridTypeList(emptyMap);
                syncMap();

                setState(State::Deploy);
                switchMapTo(UIState::MapCampOurSide);
                goto out;
            }

            case MsgType::RRefuse:
            {
                setState(State::Idle);
                goto out;
            }

            default:
                qDebug() << "错误: 状态为" << QtEnumToString(state()) << "但却接收到了" << QtEnumToString(g->type);
                goto out;
            };

        default:
            qDebug() << "错误: 状态为" << QtEnumToString(state()) << "但却接收到了" << QtEnumToString(ev);
            goto out;
        }
    }

    ////////////

    if (state() == State::CliWaitRInvit) {
        switch (ev) {
        case Event::EvGramArrived:
            switch(g->type) {

            case MsgType::RInviteOk:
            {
                setState(State::P1WaitAcc);
                goto out;
            }

            case MsgType::RInviteErrorLogout:

            {
                m_popupHint[enumToQStr(UIState::TPopupInvitResult)] = opName() + QStringLiteral(u" 不在线。");
                popupHintChanged();
                setState(State::Idle);
                setUIState(uiState() & ~UIState::TPopupMask | UIState::TPopupInvitResult);
                goto out;
            }

            case MsgType::RInviteErrorBusy:

            {
                m_popupHint[enumToQStr(UIState::TPopupInvitResult)] = opName() + QStringLiteral(u" 正忙。");
                popupHintChanged();
                setState(State::Idle);
                setUIState(uiState() & ~UIState::TPopupMask | UIState::TPopupInvitResult);
                goto out;
            }

            default:
                qDebug() << "错误: 状态为" << QtEnumToString(state()) << "但却接收到了" << QtEnumToString(g->type);
                goto out;
            };

        default:
            qDebug() << "错误: 状态为" << QtEnumToString(state()) << "但却接收到了" << QtEnumToString(ev);
            goto out;
        }
    }

    ////////////

    if (state() == State::CliWaitRDeploy) {
        switch (ev) {
        case Event::EvGramArrived:
            switch(g->type) {

            case MsgType::RDeployOk:
            {
                setState(State::WaitDeploy);
                goto out;
            }

            case MsgType::RDeployError:

            {
                setHeadTails({Coord_None,Coord_None,Coord_None,Coord_None,Coord_None,Coord_None,Coord_None,Coord_None,Coord_None,Coord_None,Coord_None,Coord_None});
                QVector<Grid> emptyMap(100);
                map()->setDataByBTPMapGridTypeList(emptyMap);
                syncMap();

                setState(State::Deploy);
                setUIState(uiState() & ~UIState::TPopupMask | UIState::TPopupDeployError);
                goto out;
            }

            default:
                qDebug() << "错误: 状态为" << QtEnumToString(state()) << "但却接收到了" << QtEnumToString(g->type);
                goto out;
            };

        default:
            qDebug() << "错误: 状态为" << QtEnumToString(state()) << "但却接收到了" << QtEnumToString(ev);
            goto out;
        }
    }

    ////////////

    if (state() == State::CliWaitRBomb) {
        switch (ev) {
        case Event::EvGramArrived:
            switch(g->type) {

            case MsgType::RBomb:
            {
                if (g->header.length != 5) {
                    qDebug() << "错误: 收到一个长为" << g->header.length << "的RBomb";
                    goto out;
                }

                Grid gg = *reinterpret_cast<const Grid *>(g->data.constData() + 4);
                oMap()->setGrid_serverBitsOnly(mapPos1(), gg);
                syncMap();

                m_popupHint[enumToQStr(UIState::TPopupActionResult)] = m_map->indexToCoordStr(mapPos1()) + (bool(gg & (Grid::Body | Grid::Tail)) ? QStringLiteral(u"：命中机身") : bool(gg & Grid::Head) ? QStringLiteral(u"：命中机头") : QStringLiteral(u"：未中"));

                popupHintChanged();
                setUIState(uiState() & ~UIState::TPopupMask | UIState::TPopupActionResult);

                gameLog += username() + QStringLiteral(u" 轰炸了 ") + opName() + QStringLiteral(u" 的 ") + m_map->indexToCoordStr(mapPos1()) + (bool(gg & (Grid::Body | Grid::Tail)) ? QStringLiteral(u" 命中机身") : bool(gg & Grid::Head) ? QStringLiteral(u" 命中机头") : QStringLiteral(u" 未中")) + "\n";
                gameLogChanged();

                setState(State::WaitOp);
                setMapPos1(Coord_None);
                setMapPos2(Coord_None);
                switchMapTo(UIState::MapCampOurSide);
                goto out;
            }




            default:
                qDebug() << "错误: 状态为" << QtEnumToString(state()) << "但却接收到了" << QtEnumToString(g->type);
                goto out;
            };

        default:
            qDebug() << "错误: 状态为" << QtEnumToString(state()) << "但却接收到了" << QtEnumToString(ev);
            goto out;
        }
    }

    ////////////

    if (state() == State::CliWaitRDeployWhenOpDeployed) {
        switch (ev) {
        case Event::EvGramArrived:
            switch(g->type) {

            case MsgType::RDeployOk:
            {
                setState(State::WaitOp);
                setMapPos1(Coord_None);
                setMapPos2(Coord_None);
                switchMapTo(UIState::MapCampOurSide);
                goto out;
            }

            case MsgType::RDeployError:

            {
                setState(State::OpDeployed);
                setUIState(uiState() & ~UIState::TPopupMask | UIState::TPopupDeployError);
                goto out;
            }

            default:
                qDebug() << "错误: 状态为" << QtEnumToString(state()) << "但却接收到了" << QtEnumToString(g->type);
                goto out;
            };

        default:
            qDebug() << "错误: 状态为" << QtEnumToString(state()) << "但却接收到了" << QtEnumToString(ev);
            goto out;
        }
    }

    ////////////

    if (state() == State::CliWaitRGuess) {
        switch (ev) {
        case Event::EvGramArrived:
            switch(g->type) {

            case MsgType::RGuess:
            {
                if (g->header.length != 5) {
                    qDebug() << "错误: 收到一个长为" << g->header.length << "的RGuess";
                    goto out;
                }

                char good = *reinterpret_cast<const char *>(g->data.constData() + 4);

                m_popupHint[enumToQStr(UIState::TPopupActionResult)] = (good ? QStringLiteral(u"猜中") : QStringLiteral(u"未猜中"));

                gameLog += username() + QStringLiteral(u" 猜测了 ") + opName() + QStringLiteral(u" 某架飞机的机头和机尾：") + m_map->indexToCoordStr(mapPos1()) + QStringLiteral(u",") + m_map->indexToCoordStr(mapPos2()) + (good ? QStringLiteral(u" 猜中") : QStringLiteral(u" 未猜中")) + "\n";
                gameLogChanged();

                popupHintChanged();
                setUIState(uiState() & ~UIState::TPopupMask | UIState::TPopupActionResult);

                setState(State::WaitOp);
                setMapPos1(Coord_None);
                setMapPos2(Coord_None);
                switchMapTo(UIState::MapCampOurSide);
                goto out;
            }

            case MsgType::Win:
            {
                m_popupHint[enumToQStr(UIState::TPopupOpAction)] = username() + QStringLiteral(u" 猜中了") + opName() + QStringLiteral(u" 的最后一架飞机，") + username() + QStringLiteral(u" 胜了。");

                popupHintChanged();
                setState(State::Idle);
                setUIState(uiState() & ~UIState::TPopupMask | UIState::TPopupOpAction);
                goto out;
            }


            default:
                qDebug() << "错误: 状态为" << QtEnumToString(state()) << "但却接收到了" << QtEnumToString(g->type);
                goto out;
            };

        default:
            qDebug() << "错误: 状态为" << QtEnumToString(state()) << "但却接收到了" << QtEnumToString(ev);
            goto out;
        }
    }


out:
    return;
}


void BTP::handleStatusGram(Gram *gram)
{
    const char *initial = gram->data.constData();
    const char *p = initial + sizeof(gram->type);
    // 读取我目前状态
    QDataStream ds(gram->data);
    State s = State::Null;
    ds.skipRawData(sizeof(MsgType));
    ds >> underlie(s);

    if (!(state() & State::CliStates))
        setState(s);
    else {
        return;
    }

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
            qDebug() << "Status Msg Bad Structure";
            return;
        }
        setOpName(p);
        p = q+1;

        if (!(state() & (State::Deploy | State::OpDeployed))) {
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
            QVector<Grid> mapVector(100);

            for (int i = 0; i < 100; i++) {
                oMapVector[i] = *reinterpret_cast<const Grid *>(p);
                p++;
                m_oMap->setGrid_serverBitsOnly(i, oMapVector[i]);
            }
            //m_oMap->setDataByBTPMapGridTypeList(oMapVector);

            for (int i = 0; i < 100; i++) {
                mapVector[i] = *reinterpret_cast<const Grid *>(p);
                p++;
                m_map->setGrid_serverBitsOnly(i, mapVector[i]);
            }
            //m_map->setDataByBTPMapGridTypeList(mapVector);
        } else {
            p += 12 + 200;
        }
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
            qDebug() << "Status Msg Bad Structure";
            return;
        }
        q++;
        if (q + 3 - initial >= gram->header.length)
        {
            qDebug() << "Status Msg Bad Structure";
            return;
        }

        t = QByteArray(p);

        QString username = gbk->toUnicode(t);

        QDataStream ds(QByteArray(q, sizeof(State)));
        State hisState = State::CliError;
        ds >> underlie(hisState);
        qDebug() << "This friend:" << username << hisState;

        Friend *newFriend = new Friend(username, hisState);
        friends.push_back(newFriend);

        p = q + 4;
    }

    qDebug() << "All Online Friends: " << friends;
    this->friends()->setDataByFriendList(friends);
}

void BTP::syncMap()
{
    if ((uiState() & UIState::MapCampMask) == UIState::MapCampOurSide) {
        *m_currMap = *m_map;
        //m_currMap->print();
    } else {
        *m_currMap = *m_oMap;
        //m_currMap->print();
    }

}
