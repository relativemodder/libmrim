#include "mrimmessage.h"
#include <QTextCodec>
#include <QDebug>
#include <QIODevice>

MrimMessage::MrimMessage(QObject *parent)
    : QObject{parent},
    m_byteOrder(QDataStream::LittleEndian)
{}

MrimMessage* MrimMessage::field(const QString& key, FieldDataType dataType,
                                const QVariant& constantValue,
                                int subbufferSize, int maxSize)
{
    MessageField field;
    field.key = key;
    field.dataType = dataType;
    field.constantValue = constantValue;
    field.maxSize = maxSize;

    m_fields.append(field);
    return this;
}

MrimMessage* MrimMessage::fieldWithCustomHandlers(
    const QString& key,
    std::function<void(QDataStream&, const QVariant&)> customWriter,
    std::function<QVariant(QDataStream&)> customReader)
{
    MessageField field;
    field.key = key;
    field.customWriter = customWriter;
    field.customReader = customReader;

    m_fields.append(field);
    return this;
}

void MrimMessage::setByteOrder(QDataStream::ByteOrder order)
{
    m_byteOrder = order;
}

QByteArray MrimMessage::write(const QMap<QString, QVariant>& message, bool utf16required)
{
    QByteArray result;
    QDataStream stream(&result, QIODevice::WriteOnly);
    stream.setByteOrder(m_byteOrder);

    for (const MessageField& field : m_fields) {
        if (field.hasCustomHandlers()) {
            QVariant value = message.value(field.key);
            field.customWriter(stream, value);
            continue;
        }

        QVariant value = field.constantValue.isValid() ? field.constantValue : message.value(field.key);

        switch (field.dataType) {
            case MRIM_FD_BYTE: {
                quint8 val = value.toUInt();
                stream << val;
                break;
            }

            case MRIM_FD_UINT16: {
                quint16 val = value.toUInt();
                stream << val;
                break;
            }

            case MRIM_FD_UINT32: {
                quint32 val = value.toUInt();
                stream << val;
                break;
            }

            case MRIM_FD_UINT64: {
                quint64 val = value.toULongLong();
                stream << val;
                break;
            }

            case MRIM_FD_INT16: {
                qint16 val = value.toInt();
                stream << val;
                break;
            }

            case MRIM_FD_INT32: {
                qint32 val = value.toInt();
                stream << val;
                break;
            }

            case MRIM_FD_SUBBUFFER: {
                QByteArray data = value.toByteArray();
                stream.writeRawData(data.constData(), data.size());
                break;
            }

            case MRIM_FD_BYTE_ARRAY: {
                QByteArray data = value.toByteArray();
                quint32 len = data.size();
                stream << len;
                stream.writeRawData(data.constData(), len);
                break;
            }

            case MRIM_FD_UBIART_LIKE_STRING: {
                QString str = value.toString();
                QByteArray encoded = convertToCP1251(str);
                quint32 len = encoded.size();
                stream << len;
                stream.writeRawData(encoded.constData(), len);
                break;
            }

            case MRIM_FD_UNICODE_STRING: {
                QString str = value.toString();
                QByteArray encoded = utf16required ? convertToUTF16LE(str) : convertToCP1251(str);
                quint32 len = encoded.size();
                stream << len;
                stream.writeRawData(encoded.constData(), len);
                break;
            }
        }
    }

    return result;
}

QMap<QString, QVariant> MrimMessage::read(const QByteArray& data, bool utf16required)
{
    QMap<QString, QVariant> result;
    QDataStream stream(data);
    stream.setByteOrder(m_byteOrder);

    for (const MessageField& field : m_fields) {
        if (field.hasCustomHandlers()) {
            result[field.key] = field.customReader(stream);
            continue;
        }

        if (stream.atEnd()) {
            if (field.dataType == MRIM_FD_UBIART_LIKE_STRING ||
                field.dataType == MRIM_FD_UNICODE_STRING) {
                result[field.key] = QString("");
            } else {
                result[field.key] = 0;
            }
            continue;
        }

        switch (field.dataType) {
            case MRIM_FD_BYTE: {
                quint8 val;
                stream >> val;
                result[field.key] = val;

                if (field.constantValue.isValid()) {
                    Q_ASSERT(val == field.constantValue.toUInt());
                }
                break;
            }

            case MRIM_FD_UINT16: {
                quint16 val;
                stream >> val;
                result[field.key] = val;

                if (field.constantValue.isValid()) {
                    Q_ASSERT(val == field.constantValue.toUInt());
                }
                break;
            }

            case MRIM_FD_UINT32: {
                quint32 val;
                stream >> val;
                result[field.key] = val;

                if (field.constantValue.isValid()) {
                    Q_ASSERT(val == field.constantValue.toUInt());
                }
                break;
            }

            case MRIM_FD_UINT64: {
                quint64 val;
                stream >> val;
                result[field.key] = QVariant::fromValue(val);

                if (field.constantValue.isValid()) {
                    Q_ASSERT(val == field.constantValue.toULongLong());
                }
                break;
            }

            case MRIM_FD_INT16: {
                qint16 val;
                stream >> val;
                result[field.key] = val;

                if (field.constantValue.isValid()) {
                    Q_ASSERT(val == field.constantValue.toInt());
                }
                break;
            }

            case MRIM_FD_INT32: {
                qint32 val;
                stream >> val;
                result[field.key] = val;

                if (field.constantValue.isValid()) {
                    Q_ASSERT(val == field.constantValue.toInt());
                }
                break;
            }

            case MRIM_FD_SUBBUFFER: {
                QByteArray buffer(field.subbufferSize, 0);
                stream.readRawData(buffer.data(), field.subbufferSize);
                result[field.key] = buffer;
                break;
            }

            case MRIM_FD_BYTE_ARRAY: {
                quint32 length;
                stream >> length;
                QByteArray buffer(length, 0);
                stream.readRawData(buffer.data(), length);
                result[field.key] = buffer;
                break;
            }

            case MRIM_FD_UBIART_LIKE_STRING: {
                quint32 length;
                stream >> length;
                QByteArray buffer(length, 0);
                stream.readRawData(buffer.data(), length);
                QString str = convertFromCP1251(buffer);
                result[field.key] = str;
                break;
            }

            case MRIM_FD_UNICODE_STRING: {
                quint32 length;
                stream >> length;

                if (length > 0) {
                    bool useUtf16 = utf16required;

                    // falling back if odd
                    if (length % 2 != 0) {
                        useUtf16 = false;
                    }

                    QByteArray buffer(length, 0);
                    stream.readRawData(buffer.data(), length);

                    QString str = useUtf16
                                      ? convertFromUTF16LE(buffer)
                                      : convertFromCP1251(buffer);

                    // trimmin
                    if (str.length() > field.maxSize) {
                        str = str.left(field.maxSize);
                    }

                    result[field.key] = str;
                } else {
                    result[field.key] = QString("");
                }
                break;
            }
        }
    }

    return result;
}

QByteArray MrimMessage::convertToCP1251(const QString& str)
{
    QTextCodec* codec = QTextCodec::codecForName("Windows-1251");
    if (codec) {
        return codec->fromUnicode(str);
    }
    return str.toLatin1();
}

QString MrimMessage::convertFromCP1251(const QByteArray& data)
{
    QTextCodec* codec = QTextCodec::codecForName("Windows-1251");
    if (codec) {
        return codec->toUnicode(data);
    }
    qDebug() << "making qstring from latin" << data;
    return QString::fromLatin1(data);
}

QByteArray MrimMessage::convertToUTF16LE(const QString& str)
{
    auto toUtf16 = QStringEncoder(QStringEncoder::Utf16);
    QByteArray encodedString = toUtf16(str);

    return encodedString;
}

QString MrimMessage::convertFromUTF16LE(const QByteArray& data)
{
    auto fromUtf16 = QStringDecoder(QStringDecoder::Utf16);
    QString decodedString = fromUtf16(data);

    return decodedString;
}
