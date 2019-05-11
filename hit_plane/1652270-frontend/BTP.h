#ifndef BTP_H
#define BTP_H

#include <QObject>

#include <QTimer>
#include <QUrl>
#include <QMetaEnum>

#include "SocketMan.h"
#include "BTPMapModel.h"
#include "FriendsModel.h"
#include "enum.h"

class BTP : public QObject
{
    Q_OBJECT

public:
    enum class UIState {
        Null = 0,
        MapCampOurSide = 0x1,
        MapCampTheirSide = 0x1 << 1,
        MapCampMask = MapCampOurSide | MapCampTheirSide,
        MapModeBomb = 0x1 << 2,
        MapModeGuess = 0x1 << 3,
        MapModeMask = MapModeBomb | MapModeGuess,
        MapSelectedPos1 = 0x1 << 4,

        TPopupNone = 0x0,   // Trivial Popup
        TPopupInitiaInvit = 0x1 << 5,
        TPopupOpAction = 0x2 << 5,
        TPopupChat = 0x3 << 5,
        TPopupWinLose = 0x4 << 5,
        TPopupInvitResult = 0x5 << 5,
        TPopupDeployError = 0x6 << 5,
        TPopupActionResult = 0x7 << 5,
        TPopupError = 0x8 << 5,
        TPopupMask = 0xf << 5,


        CPopupNone = 0x0 << 9,  // Critical Popup
        CPopupAccInvit = 0x1 << 9,
        CPopupLogin = 0x3 << 9,
        CPopupWaitRespBusy = 0x4 << 9,
        CPopupWaitDeployBusy = 0x5 << 9,
        //CPopupOccupied = 0x6 << 8,


        CPopupMask = 0x15 << 8,

    };

    enum class Event {
        EvRefuseInvit,
        EvAcceptInvit,
        EvLogin,
        EvLogout,
        EvQuitGame,
        EvClickGrid,
        EvCancelInvit,
        EvConfirmInvit,
        EvConfirmOpAction,
        EvConfirmChat,
        EvConfirmError,
        EvConfirmInvitResult,
        EvGramArrived,
        EvInvite,
        EvGuess,
        EvBomb,
        EvDeploy,

    };

    Q_ENUM(UIState)
    Q_ENUM(Event)

    Q_PROPERTY(QString socketStateText READ socketStateText NOTIFY socketStateTextChanged)
    Q_PROPERTY(qreal reconnRemainingTime READ reconnRemainingTime NOTIFY reconnRemainingTimeChanged)

    Q_PROPERTY(State state READ state WRITE setState NOTIFY stateChanged)
    Q_PROPERTY(UIState uiState READ uiState WRITE setUIState NOTIFY uiStateChanged)

    Q_PROPERTY(BTPMapModel *map READ map NOTIFY mapChanged)
    Q_PROPERTY(BTPMapModel *oMap READ oMap NOTIFY oMapChanged)
    Q_PROPERTY(BTPMapModel *currMap READ currMap NOTIFY currMapChanged)

    Q_PROPERTY(FriendsModel *friends READ friends NOTIFY friendsChanged)

    Q_PROPERTY(QString username READ username WRITE setUsername NOTIFY usernameChanged)
    Q_PROPERTY(QString opName READ opName WRITE setOpName NOTIFY opNameChanged)
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY passwordChanged)

    Q_PROPERTY(QVariantMap popupHint READ popupHint WRITE setPopupHint NOTIFY popupHintChanged)


    Q_PROPERTY(QString mapHint1 READ mapHint1 WRITE setMapHint1 NOTIFY mapHint1Changed)
    Q_PROPERTY(QString mapHint2 READ mapHint2 WRITE setMapHint2 NOTIFY mapHint2Changed)

    Q_PROPERTY(quint8 mapPos1 READ mapPos1 WRITE setMapPos1 NOTIFY mapPos1Changed)
    Q_PROPERTY(quint8 mapPos2 READ mapPos2 WRITE setMapPos2 NOTIFY mapPos2Changed)

    Q_PROPERTY(QVariantList headTails READ headTails NOTIFY headTailsChanged)

    Q_PROPERTY(quint8 Coord_None MEMBER Coord_None CONSTANT)

    Q_PROPERTY(QString gameLog MEMBER gameLog NOTIFY gameLogChanged)

public:
    explicit BTP(QObject *parent = nullptr);
    ~BTP();

signals:
    void socketStateTextChanged();
    void reconnRemainingTimeChanged();

    void stateChanged();
    void uiStateChanged();

    void mapChanged();
    void oMapChanged();
    void currMapChanged();
    void friendsChanged();

    void usernameChanged();
    void opNameChanged();
    void passwordChanged();

    void popupHintChanged();
    void mapHint1Changed();
    void mapHint2Changed();

    void mapPos1Changed();
    void mapPos2Changed();

    void headTailsChanged();

    void periodicalRefresh();

    void gameLogChanged();

public slots:
    void onConnected();
    void onSocketError(QAbstractSocket::SocketError err);
    void onSocketStateChanged(QAbstractSocket::SocketState state);
    void onGramArrived(Gram *gram);

    void onPeriodicalRefresh();
    void onReconnRemainingTimeChanged();

public:

    QString socketStateText() const;
    qreal reconnRemainingTime() const;

    State state() const;
    Q_INVOKABLE void setState(State state);

    UIState uiState() const;
    Q_INVOKABLE void setUIState(UIState uiState);

    BTPMapModel *map() const;
    BTPMapModel *oMap() const;
    BTPMapModel *currMap() const;

    FriendsModel *friends();
    void setFriends(FriendsModel *friendsModel);

    QString username() const;
    Q_INVOKABLE void setUsername(const QString &username);

    QString opName() const;
    Q_INVOKABLE void setOpName(const QString &opName);

    QString password() const;
    Q_INVOKABLE void setPassword(const QString &password);

    const QVariantMap &popupHint() const;
    Q_INVOKABLE void setPopupHint(const QVariantMap &popupHint);

    QString mapHint1() const;
    Q_INVOKABLE void setMapHint1(const QString &mapHint1);

    QString mapHint2() const;
    Q_INVOKABLE void setMapHint2(const QString &mapHint2);

    quint8 mapPos1() const;
    Q_INVOKABLE void setMapPos1(quint8 mapPos1);

    quint8 mapPos2() const;
    Q_INVOKABLE void setMapPos2(quint8 mapPos2);

    QVariantList headTails() const;
    void setHeadTails(const QVariantList &qv);
    Q_INVOKABLE void setHeadTails(int i, const QVariant &qv);


    Q_INVOKABLE void init();
    Q_INVOKABLE void reset(bool isError);
    Q_INVOKABLE void afterError();


    Q_INVOKABLE void switchMap();
    Q_INVOKABLE void switchToBomb();
    Q_INVOKABLE void switchToGuess();

    Q_INVOKABLE QString getStateText(State state);
    Q_INVOKABLE QString getStateText_friendly_int(quint32 state);

    Q_INVOKABLE void triggerEvent(Event ev, const QVariant &param);

    void handleStatusGram(Gram *gram);

    Q_INVOKABLE void syncMap();

    void switchMapTo(UIState side);
private:

    State m_state;
    UIState m_uiState;

    BTPMapModel *m_map;
    BTPMapModel *m_oMap;
    BTPMapModel *m_currMap;
    FriendsModel *m_friends;

    QString m_username;
    QString m_password;

    QVariantMap m_popupHint;
    QString m_mapHint1;
    QString m_mapHint2;

    quint8 m_mapPos1;
    quint8 m_mapPos2;

    const unsigned char Coord_None = 100;

    QString m_opName;

    unsigned char m_h1;
    unsigned char m_h2;
    unsigned char m_h3;
    unsigned char m_t1;
    unsigned char m_t2;
    unsigned char m_t3;

    unsigned char m_oh1;
    unsigned char m_oh2;
    unsigned char m_oh3;
    unsigned char m_ot1;
    unsigned char m_ot2;
    unsigned char m_ot3;

    QString gameLog;

//////

    SocketMan *m_sm;

    QStringList m_socketStateText;

    QTimer *periodicalTimer;

    State m_nextStateAfterError;
};

Q_DECLARE_METATYPE(BTP::UIState)
Q_DECLARE_METATYPE(BTP::Event)

inline BTP::UIState operator&(BTP::UIState i, BTP::UIState j) {
    return static_cast<BTP::UIState>(static_cast<std::underlying_type<BTP::UIState>::type>(i) & static_cast<std::underlying_type<BTP::UIState>::type>(j));
}

inline BTP::UIState operator|(BTP::UIState i, BTP::UIState j) {
    return static_cast<BTP::UIState>(static_cast<std::underlying_type<BTP::UIState>::type>(i) | static_cast<std::underlying_type<BTP::UIState>::type>(j));
}

inline BTP::UIState operator~(BTP::UIState i) {
    return static_cast<BTP::UIState>(~static_cast<std::underlying_type<BTP::UIState>::type>(i));
}

inline bool operator+(const BTP::UIState &state) {
    return state != BTP::UIState::Null;
}

inline bool operator!(const BTP::UIState &state) {
    return state == BTP::UIState::Null;
}

#endif // BTP_H
