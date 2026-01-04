#ifndef MRIMMESSAGE_H
#define MRIMMESSAGE_H

#include <QObject>
#include <QByteArray>
#include <QDataStream>
#include <QMap>
#include <QVariant>
#include <functional>

#define BSWAP_16(x) ((((x)  & 0xFF) << 8) | ((x) >> 8))

enum FieldDataType {
    MRIM_FD_BYTE = 1,
    MRIM_FD_UINT16 = 2,
    MRIM_FD_UINT32 = 3,
    MRIM_FD_UINT64 = 10,
    MRIM_FD_INT16 = 4,
    MRIM_FD_INT32 = 5,
    MRIM_FD_SUBBUFFER = 6,
    MRIM_FD_BYTE_ARRAY = 7,
    MRIM_FD_UBIART_LIKE_STRING = 8,
    MRIM_FD_UNICODE_STRING = 9
};

struct MessageField {
    QString key;
    FieldDataType dataType;
    QVariant constantValue;
    int subbufferSize;
    int maxSize;

    std::function<void(QDataStream&, const QVariant&)> customWriter;
    std::function<QVariant(QDataStream&)> customReader;

    bool hasCustomHandlers() const {
        return customWriter && customReader;
    }

    MessageField() : dataType(MRIM_FD_BYTE), subbufferSize(0), maxSize(5000) {}
};

class MrimMessage : public QObject
{
    Q_OBJECT

public:
    explicit MrimMessage(QObject *parent = nullptr);

    MrimMessage* field(const QString& key, FieldDataType dataType,
                       const QVariant& constantValue = QVariant(),
                       int subbufferSize = 0, int maxSize = 5000);

    MrimMessage* fieldWithCustomHandlers(
        const QString& key,
        std::function<void(QDataStream&, const QVariant&)> customWriter,
        std::function<QVariant(QDataStream&)> customReader
        );

    QByteArray write(const QMap<QString, QVariant>& message, bool utf16required = false);
    QMap<QString, QVariant> read(const QByteArray& data, bool utf16required = false);

    void setByteOrder(QDataStream::ByteOrder order);

signals:

private:
    QList<MessageField> m_fields;
    QDataStream::ByteOrder m_byteOrder;

    QByteArray convertToCP1251(const QString& str);
    QString convertFromCP1251(const QByteArray& data);

    QByteArray convertToUTF16LE(const QString& str);
    QString convertFromUTF16LE(const QByteArray& data);
};

#endif // MRIMMESSAGE_H
