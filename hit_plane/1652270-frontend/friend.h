#ifndef FRIEND_H
#define FRIEND_H

#include <QObject>
#include <QModelIndex>

#include "enum.h"

class Friend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString username READ username NOTIFY usernameChanged)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString avatar READ avatar NOTIFY avatarChanged)

public:
    explicit Friend(const QString &username, State state, QObject *parent = nullptr);
    Friend(const Friend &_friend, QObject *parent = nullptr);
    Friend &operator=(const Friend &_friend);

    QString username() const;
    State state() const;
    QString avatar() const;

    void setUsername(const QString &username);
    void setState(State state);
    void setAvatar(const QString &avatar);

    void setIndex(QModelIndex index);

signals:
    void usernameChanged(const QString &username);
    void stateChanged(State state);
    void avatarChanged(const QString &avatar);

    void itemChanged(QModelIndex i);

public slots:
    void onSomethingChanged();

private:
    QString m_username;
    State m_state;
    QString m_avatar;

    QModelIndex m_index;

    void doConnects();
};



#endif // FRIEND_H
