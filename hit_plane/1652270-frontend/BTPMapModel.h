#ifndef BTPMAPMODEL_H
#define BTPMAPMODEL_H

#include "enum.h"
#include <QAbstractListModel>

class BTPMapModel : public QAbstractListModel
{
    Q_OBJECT
public:
    using BTPMapGridType = Grid;

    enum class BTPMapModelRoles {
        BTPMapGridTypeRole = Qt::UserRole + 1,
        BTPMapLabelRole,
    };

    BTPMapModel(QObject *parent = nullptr);
    ~BTPMapModel() override;

    Qt::ItemFlags flags(const QModelIndex &i) const override;
    QVariant data(const QModelIndex &i, int role) const override;
    int rowCount(const QModelIndex &i) const override;

    Q_INVOKABLE void setDataByBTPMapGridTypeList(const QVector<BTPMapGridType> &newData);

    Q_INVOKABLE void setGrid(int i, BTPMapGridType type);
    Q_INVOKABLE void setGrid_i(int i, quint8 type);
    Q_INVOKABLE void setGridBits(int i, BTPMapGridType typeBits);
    Q_INVOKABLE void setGrid_serverBitsOnly(int i, BTPMapGridType type);
    Q_INVOKABLE void depriveGridBits(int i, BTPMapGridType typeBits);

    Q_INVOKABLE void clear();

    Q_INVOKABLE void setGridLabel(int i, const QString &qs);
    Q_INVOKABLE void restoreGridLabel(int i);

    Q_INVOKABLE Grid get(int i) const;

    void print() const;

    BTPMapModel &operator=(const BTPMapModel &another);


    QVector<BTPMapGridType> &getData();

    QString indexToCoordStr(unsigned char index);
    Grid bomb(unsigned char index);

public slots:

protected:
    QHash<int, QByteArray> roleNames() const override;

private:
    QVector<BTPMapGridType> m_data;
    QVector<QString> m_buttonLabel;
};

#endif // BTPMAPMODEL_H
