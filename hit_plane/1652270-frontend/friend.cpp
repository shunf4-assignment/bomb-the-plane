#include "Friend.h"
#include "myimage.h"
#include <QDebug>

Friend::Friend(const QString &username, State _state, QObject *parent) : QObject(parent), m_username(username), m_state(_state), m_avatar(myImage), m_index()
{
    this->doConnects();
}

Friend::Friend(const Friend &_friend, QObject *parent) : QObject(parent), m_username(_friend.m_username), m_state(_friend.m_state), m_avatar(_friend.m_avatar), m_index(_friend.m_index)
{
    this->doConnects();
}

Friend &Friend::operator=(const Friend &_friend)
{
    this->m_username = _friend.m_username;
    this->m_state = _friend.m_state;
    this->m_avatar = _friend.m_avatar;
    return *this;
}


QString Friend::username() const
{
    return this->m_username;
}

State Friend::state() const
{
    return this->m_state;
}

QString Friend::avatar() const
{
    return this->m_avatar;
}



void Friend::setUsername(const QString &username)
{
    this->m_username = username;
    emit usernameChanged(this->m_username);
}

void Friend::setState(State state)
{
    this->m_state = state;
    emit stateChanged(this->m_state);
}

void Friend::setAvatar(const QString &avatar)
{
    this->m_avatar = avatar;
    emit avatarChanged(this->m_avatar);
}

void Friend::setIndex(QModelIndex index)
{
    this->m_index = index;
}

void Friend::onSomethingChanged()
{
    qDebug() << "friend: somethingChanged()" << this;
    emit itemChanged(this->m_index);
}

void Friend::doConnects()
{
    // 把自己的各字段变动，连接一份到 onSomethingChanged 来
    connect(this, SIGNAL(usernameChanged(const QString &)), this, SLOT(onSomethingChanged()));
    connect(this, SIGNAL(stateChanged(State)), this, SLOT(onSomethingChanged()));
    connect(this, SIGNAL(avatarChanged(const QString &)), this, SLOT(onSomethingChanged()));
}
