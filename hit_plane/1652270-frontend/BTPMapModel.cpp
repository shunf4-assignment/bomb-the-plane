#include "BTPMapModel.h"
#include <QPixmap>
#include <QDebug>

BTPMapModel::BTPMapModel(QObject *parent) : QAbstractListModel(parent), m_data(100), m_buttonLabel(100)
{
    for (int i = 0; i < 100; i++) {
        m_buttonLabel[i] = indexToCoordStr(quint8(i));
    }
}

BTPMapModel::~BTPMapModel()
{
}

Qt::ItemFlags BTPMapModel::flags(const QModelIndex &) const
{
    return Qt::ItemNeverHasChildren | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant BTPMapModel::data(const QModelIndex &i, int role) const
{
    BTPMapModelRoles r = static_cast<BTPMapModelRoles>(role);

    switch(r)
    {
    case BTPMapModelRoles::BTPMapGridTypeRole:
        return QVariant::fromValue(this->m_data.at(i.row()));
    case BTPMapModelRoles::BTPMapLabelRole:
        return QVariant::fromValue(this->m_buttonLabel.at(i.row()));
    }
}

int BTPMapModel::rowCount(const QModelIndex &) const
{
    return this->m_data.size(); // 100
}

void BTPMapModel::setDataByBTPMapGridTypeList(const QVector<BTPMapGridType> &newData)
{
    this->m_data = newData;
    emit dataChanged(index(0), index(99));
}

void BTPMapModel::setGrid(int i, BTPMapGridType type)
{
    this->m_data[i] = type;
    emit dataChanged(index(i), index(i));
}

void BTPMapModel::setGrid_i(int i, quint8 type)
{
    this->m_data[i] = static_cast<Grid>(type);
    emit dataChanged(index(i), index(i));
}

void BTPMapModel::setGridBits(int i, BTPMapGridType typeBits)
{
    this->m_data[i] = static_cast<BTPMapGridType>(static_cast<qint32>(this->m_data[i]) | static_cast<qint32>(typeBits));
    emit dataChanged(index(i), index(i));
}

void BTPMapModel::setGrid_serverBitsOnly(int i, BTPMapModel::BTPMapGridType type)
{
    this->m_data[i] = this->m_data[i] & ~Grid::ServerBits | type;
    emit dataChanged(index(i), index(i));
}

void BTPMapModel::depriveGridBits(int i, BTPMapGridType typeBits)
{
    this->m_data[i] = static_cast<BTPMapGridType>(static_cast<qint32>(this->m_data[i]) & ~static_cast<qint32>(typeBits));
    emit dataChanged(index(i), index(i));
}

void BTPMapModel::clear()
{
    for (int i = 0; i < 100; i++) {
        m_data[i] = Grid::Blank;
    }
    emit dataChanged(index(0), index(99));
}

void BTPMapModel::setGridLabel(int i, const QString &qs)
{
    m_buttonLabel[i] = qs;
    emit dataChanged(index(i), index(i));
}

void BTPMapModel::restoreGridLabel(int i)
{
    m_buttonLabel[i] = indexToCoordStr(quint8(i));
    emit dataChanged(index(i), index(i));
}

GridNS::Grid BTPMapModel::get(int i) const
{
    return m_data[i];
}

void BTPMapModel::print() const
{
    for (int i = 0; i < 10; i++) {
        auto qd = qDebug();
        for (int j = 0; j < 10; j++) {
            qd << static_cast<quint8>(m_data[10 * i + j]);
        }
    }
}

BTPMapModel &BTPMapModel::operator=(const BTPMapModel &another)
{
    this->m_data = another.m_data;
    this->m_buttonLabel = another.m_buttonLabel;
    emit dataChanged(index(0), index(99));
    return *this;
}


QVector<BTPMapModel::BTPMapGridType> &BTPMapModel::getData()
{
    return m_data;
}

QString BTPMapModel::indexToCoordStr(unsigned char index)
{
    char cstr[3] = {};
    cstr[0] = 'A' + (index / 10);
    cstr[1] = '0' + (index % 10);
    return QString(cstr);
}

Grid BTPMapModel::bomb(unsigned char index)
{
    setGridBits(index, BTPMapGridType::Hit);
    return this->m_data[index];
}


QHash<int, QByteArray> BTPMapModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[static_cast<int>(BTPMapModelRoles::BTPMapGridTypeRole)] = "type";
    roles[static_cast<int>(BTPMapModelRoles::BTPMapLabelRole)] = "label";
    return roles;
}
