#ifndef ENUM_H
#define ENUM_H
#include <cstdint>

#ifdef QT_BEING_USED
#include <QtGlobal>
#include <QObject>
namespace StateNS
{
    Q_NAMESPACE
#endif

enum class State : uint32_t {
    Null = 0,
    Logout = 0x1,
    Idle = 0x1 << 1,
    P1WaitAcc = 0x1 << 1 << 1,
    P2WaitAcc = 0x1 << 2 << 1,
    Deploy = 0x1 << 3 << 1,
    WaitDeploy = 0x1 << 4 << 1,
    MyTurn = 0x1 << 5 << 1,
    WaitOp = 0x1 << 6 << 1,
    OpDeployed = 0x1 << 8,
    InGame = Deploy | WaitDeploy | OpDeployed | MyTurn | WaitOp,


    CliPasswordError = 0x1 << 29,
    CliUsernameError = 0x1 << 28,
    CliWaitLogin = 0x1 << 27,
    CliWaitEnd = 0x1 << 26,
    CliWaitRAcc = 0x1 << 25,
    CliWaitRInvit = 0x1 << 24,
    CliWaitRDeploy = 0x1 << 23,
    CliWaitRBomb = 0x1 << 22,
    CliWaitRDeployWhenOpDeployed = 0x1 << 21,
    CliWaitRGuess = 0x1 << 20,
    CliError = 0x1u << 31,

    CliNotLogin = CliPasswordError | CliUsernameError | CliWaitLogin | Logout,

    CliStates = CliPasswordError|CliUsernameError|CliWaitLogin |CliWaitEnd | CliWaitRAcc | CliWaitRInvit | CliWaitRDeploy |CliWaitRBomb| CliWaitRDeployWhenOpDeployed | CliWaitRGuess |CliError

};

#ifdef QT_BEING_USED
    Q_ENUM_NS(State)
}

using StateNS::State;
Q_DECLARE_METATYPE(State)
#endif

#ifdef QT_BEING_USED
namespace MsgTypeNS
{
    Q_NAMESPACE
#endif

enum class MsgType : uint32_t {
    Null,
    Reset,
    Login,
    Invite,
    End,
    Accept,
    Refuse,
    Deploy,
    Bomb,
    Guess,
    Win,
    Lose,
    Chat,
    Peek,
    Status,

    RLoginOk,
    RLoginError,  // Not directly used
    RInviteOk,
    RInviteError, // Not directly used
    REnd,
    RAccept,
    RRefuse,
    RDeployOk,
    RDeployError,
    RBomb,
    RGuess,
    RStatus,

    RLoginUsernameError = RLoginError << 16,
    RLoginPasswordError = (RLoginError << 16) | 0x1,

    RInviteErrorLogout = RInviteError << 16,
    RInviteErrorBusy = (RInviteError << 16) | 0x1
};

#ifdef QT_BEING_USED
    Q_ENUM_NS(MsgType)
}

using MsgTypeNS::MsgType;
Q_DECLARE_METATYPE(MsgType)
#endif


#ifdef QT_BEING_USED
namespace GridNS
{
    Q_NAMESPACE
#endif

enum class Grid :char {
    Blank = 0,
    Body = 0x1,
    Hit = 0x1 << 1,
    Head = 0x1 << 2,
    Tail = 0x1 << 3,
    ServerBits = Body | Hit | Head | Tail,

    Selected = 0x1 << 5,
    Invalid = 0x1 << 6,
    ClientBits = Selected | Invalid
};

#ifdef QT_BEING_USED
    Q_ENUM_NS(Grid)
}

using GridNS::Grid;
Q_DECLARE_METATYPE(Grid)
#endif


inline State operator&(const State &lhs, const State &rhs) {
    return static_cast<State>(static_cast<std::underlying_type<State>::type>(lhs) & static_cast<std::underlying_type<State>::type>(rhs));
}

inline State operator|(const State &lhs, const State &rhs) {
    return static_cast<State>(static_cast<std::underlying_type<State>::type>(lhs) | static_cast<std::underlying_type<State>::type>(rhs));
}

inline State operator~(const State &x) {
    return static_cast<State>(~static_cast<std::underlying_type<State>::type>(x));
}


inline bool operator+(const State &state) {
    return state != State::Null;
}

inline bool operator!(const State &state) {
    return state == State::Null;
}

inline bool isHit(const Grid &g) {
    return static_cast<int>(g) & static_cast<int>(Grid::Hit);
}

inline bool isHead(const Grid &g) {
    return static_cast<int>(g) & static_cast<int>(Grid::Head);
}

inline bool isTail(const Grid &g) {
    return static_cast<int>(g) & static_cast<int>(Grid::Tail);
}

inline bool isBody(const Grid &g) {
    return static_cast<int>(g) & static_cast<int>(Grid::Body);
}

inline Grid operator&(const Grid &lhs, const Grid &rhs) {
    return static_cast<Grid>(static_cast<std::underlying_type<Grid>::type>(lhs) & static_cast<std::underlying_type<Grid>::type>(rhs));
}

inline Grid operator|(const Grid &lhs, const Grid &rhs) {
    return static_cast<Grid>(static_cast<std::underlying_type<Grid>::type>(lhs) | static_cast<std::underlying_type<Grid>::type>(rhs));
}

inline Grid operator~(const Grid &x) {
    return static_cast<Grid>(~static_cast<std::underlying_type<Grid>::type>(x));
}

template <typename T>
typename std::underlying_type<T>::type &underlie(T &x) {
    return reinterpret_cast<typename std::underlying_type<T>::type &>(x);
}

template <typename T>
const typename std::underlying_type<T>::type &underlie(const T &x) {
    return reinterpret_cast<const typename std::underlying_type<T>::type &>(x);
}

#endif
