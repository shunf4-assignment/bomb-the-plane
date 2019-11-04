#include "FriendsModel.h"
#include <QPixmap>
#include "myimage.h"
#include <QDebug>

FriendsModel::FriendsModel(QObject *parent) : QAbstractListModel(parent)
{

}

FriendsModel::~FriendsModel()
{
    foreach(Friend *f, this->m_data)
    {
        f->deleteLater();
    }
}


int FriendsModel::count() const
{
    return m_data.count();
}


Qt::ItemFlags FriendsModel::flags(const QModelIndex &) const
{
    return Qt::ItemNeverHasChildren | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant FriendsModel::data(const QModelIndex &i, int role) const
{
    FriendsModelRoles r = static_cast<FriendsModelRoles>(role);
    Friend *f = this->m_data.at(i.row());

    switch(r)
    {
    case FriendsModelRoles::AvatarRole:
        return QVariant::fromValue(f->avatar());
    case FriendsModelRoles::UsernameRole:
        return QVariant::fromValue(f->username());
    case FriendsModelRoles::StateRole:
        return QVariant::fromValue(static_cast<quint32>(f->state()));
    }
}

int FriendsModel::rowCount(const QModelIndex &) const
{
    return this->m_data.size();
}

void FriendsModel::setDataByFriendList(const QVector<Friend *> &newData)
{
    beginResetModel();
    foreach(Friend *f, this->m_data)
    {
        f->deleteLater();
    }
    this->m_data = newData;
    for (int i = 0; i < m_data.size(); i++)
    {
        auto f = m_data.at(i);
        f->setIndex(index(i));
        qDebug() << "friend connect()" << f << this;
        connect(f, SIGNAL(itemChanged(QModelIndex)), this, SLOT(onItemChanged(QModelIndex)));
    }
    endResetModel();
    emit dataChanged(index(0), index(m_data.size() - 1));
    onItemChanged(index(0));
    emit countChanged();
}

QVector<Friend *> FriendsModel::getData()
{
    return m_data;
}

void FriendsModel::onItemChanged(QModelIndex i)
{
    qDebug() << "friendModel: onItemChanged()";
    emit dataChanged(i, i);
    emit countChanged();
}

QHash<int, QByteArray> FriendsModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[static_cast<int>(FriendsModelRoles::AvatarRole)] = "avatar";
    roles[static_cast<int>(FriendsModelRoles::UsernameRole)] = "username";
    roles[static_cast<int>(FriendsModelRoles::StateRole)] = "currState";
    return roles;
}


