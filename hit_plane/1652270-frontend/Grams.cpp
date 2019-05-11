#include "Grams.h"
#include <QDebug>
GramHeader::GramHeader(const QByteArray &byteArray)
{
    this->fromByteArray(byteArray);
}

void GramHeader::fromByteArray(const QByteArray &byteArray)
{
    QDataStream dataStream(byteArray);
    dataStream >> this->length;
}

void GramHeader::fromDataStream(QDataStream &dataStream)
{
    dataStream >> this->length;
}

Gram::Gram(QObject *parent) : QObject(parent)
{
    this->header.length = 0;
}

Gram::Gram(const QByteArray &byteArray, QObject *parent) : Gram(parent)
{
    this->fromByteArray(byteArray);
}

Gram::Gram(const Gram &gram, QObject *parent) : QObject(parent)
{
    this->header = gram.header;
    this->type = gram.type;
    this->data = gram.data;
}


// 注意: Qt 的 QDataStream 对于数据的存储统一定义为大端序(网络序), 所以就没有 ntoh 和 hton 的麻烦了.
void Gram::fromByteArray(const QByteArray &byteArray)
{
    QDataStream dataStream(byteArray);

    this->header.fromDataStream(dataStream);

    this->data.resize(static_cast<int>(this->header.length));

    dataStream.readRawData(this->data.data(), static_cast<int>(this->header.length));

    this->type = MsgType::Reset;
    QDataStream(this->data) >> underlie(this->type);
    qDebug() << this->type;
}

QByteArray Gram::toByteArray() const
{
    QByteArray byteArray;
    QDataStream dataStream(&byteArray, QIODevice::OpenModeFlag::WriteOnly);

    dataStream << this->header.length;

    dataStream.writeRawData(this->data.constData(), static_cast<int>(this->header.length));

    return byteArray;
}

quint32 Gram::size()
{
    return GramHeader::size() + static_cast<unsigned int>(this->data.size());
}
