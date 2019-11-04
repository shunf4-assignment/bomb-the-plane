#ifndef FRIENDSMODEL_H
#define FRIENDSMODEL_H

#include <QAbstractListModel>
#include <QObject>
#include "Friend.h"

class FriendsModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
    enum class FriendsModelRoles {
        AvatarRole = Qt::UserRole + 1,
        UsernameRole,
        StateRole,
    };
    explicit FriendsModel(QObject *parent = nullptr);
    ~FriendsModel() override;

    Qt::ItemFlags flags(const QModelIndex &i) const override;
    QVariant data(const QModelIndex &i, int role) const override;
    int rowCount(const QModelIndex &i) const override;

    void setDataByFriendList(const QVector<Friend *> &newData);

    QVector<Friend *> getData();

    int count() const;
signals:
    void countChanged();

public slots:
    void onItemChanged(QModelIndex i);

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    QVector<Friend *> m_data;
};

#endif // FRIENDSMODEL_H
