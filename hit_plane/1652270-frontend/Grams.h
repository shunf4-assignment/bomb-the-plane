#ifndef GRAM_H
#define GRAM_H

#include <QtGlobal>
#include <QObject>
#include <QDataStream>
#include "enum.h"

struct GramHeader
{
    quint32 length;

    GramHeader() = default;
    GramHeader(const QByteArray &byteArray);
    void fromByteArray(const QByteArray &byteArray);
    void fromDataStream(QDataStream &dataStream);
    static constexpr quint32 size() {return sizeof(GramHeader);}
};

class Gram : public QObject
{
    Q_OBJECT
public:
    explicit Gram(QObject *parent = nullptr);
    explicit Gram(const QByteArray &byteArray, QObject *parent = nullptr);
    Gram(const Gram &gram, QObject *parent = nullptr);

signals:

public slots:

public:
    GramHeader header;
    MsgType type;
    QByteArray data;    // data includes type
    void fromByteArray(const QByteArray &byteArray);
    QByteArray toByteArray() const;
    quint32 size();
};

#endif // GRAM_H
