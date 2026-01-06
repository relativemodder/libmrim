#include "mrim/mrimmessage.h"
#include <QTextCodec>
#include <QStringEncoder>
#include <QStringDecoder>
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
    field.subbufferSize = subbufferSize;
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

// Function pointer approach - no more giant switches!
MrimMessage::WriterFunc MrimMessage::getWriter(FieldDataType type)
{
    switch (type) {
    case MRIM_FD_BYTE:
        return [](QDataStream& s, const QVariant& v) { s << quint8(v.toUInt()); };
    case MRIM_FD_UINT16:
        return [](QDataStream& s, const QVariant& v) { s << quint16(v.toUInt()); };
    case MRIM_FD_UINT32:
        return [](QDataStream& s, const QVariant& v) { s << quint32(v.toUInt()); };
    case MRIM_FD_UINT64:
        return [](QDataStream& s, const QVariant& v) { s << quint64(v.toULongLong()); };
    case MRIM_FD_INT16:
        return [](QDataStream& s, const QVariant& v) { s << qint16(v.toInt()); };
    case MRIM_FD_INT32:
        return [](QDataStream& s, const QVariant& v) { s << qint32(v.toInt()); };
    case MRIM_FD_SUBBUFFER:
        return [](QDataStream& s, const QVariant& v) {
            QByteArray data = v.toByteArray();
            s.writeRawData(data.constData(), data.size());
        };
    case MRIM_FD_BYTE_ARRAY:
        return [](QDataStream& s, const QVariant& v) {
            QByteArray data = v.toByteArray();
            s << quint32(data.size());
            s.writeRawData(data.constData(), data.size());
        };
    default:
        return [](QDataStream&, const QVariant&) {};
    }
}

MrimMessage::ReaderFunc MrimMessage::getReader(FieldDataType type)
{
    switch (type) {
    case MRIM_FD_BYTE:
        return [](QDataStream& s, bool) { quint8 v; s >> v; return v; };
    case MRIM_FD_UINT16:
        return [](QDataStream& s, bool) { quint16 v; s >> v; return v; };
    case MRIM_FD_UINT32:
        return [](QDataStream& s, bool) { quint32 v; s >> v; return v; };
    case MRIM_FD_UINT64:
        return [](QDataStream& s, bool) { quint64 v; s >> v; return QVariant::fromValue(v); };
    case MRIM_FD_INT16:
        return [](QDataStream& s, bool) { qint16 v; s >> v; return v; };
    case MRIM_FD_INT32:
        return [](QDataStream& s, bool) { qint32 v; s >> v; return v; };
    case MRIM_FD_SUBBUFFER:
        return [this](QDataStream& s, bool) {
            // Need subbuffer size from field
            return QVariant();
        };
    case MRIM_FD_BYTE_ARRAY:
        return [](QDataStream& s, bool) {
            quint32 len;
            s >> len;
            QByteArray buf(len, 0);
            s.readRawData(buf.data(), len);
            return buf;
        };
    default:
        return [](QDataStream&, bool) { return QVariant(); };
    }
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

        // Special handling for strings
        if (field.dataType == MRIM_FD_UBIART_LIKE_STRING) {
            QString str = value.toString();
            QByteArray encoded = convertToCP1251(str);
            stream << quint32(encoded.size());
            stream.writeRawData(encoded.constData(), encoded.size());
        } else if (field.dataType == MRIM_FD_UNICODE_STRING) {
            QString str = value.toString();
            QByteArray encoded = utf16required ? convertToUTF16LE(str) : convertToCP1251(str);
            stream << quint32(encoded.size());
            stream.writeRawData(encoded.constData(), encoded.size());
        } else {
            // Use function pointer
            auto writer = getWriter(field.dataType);
            writer(stream, value);
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

        // Special handling for strings
        if (field.dataType == MRIM_FD_UBIART_LIKE_STRING) {
            quint32 length;
            stream >> length;
            QByteArray buffer(length, 0);
            stream.readRawData(buffer.data(), length);
            result[field.key] = convertFromCP1251(buffer);
        } else if (field.dataType == MRIM_FD_UNICODE_STRING) {
            quint32 length;
            stream >> length;
            if (length > 0) {
                bool useUtf16 = utf16required && (length % 2 == 0);
                QByteArray buffer(length, 0);
                stream.readRawData(buffer.data(), length);
                QString str = useUtf16 ? convertFromUTF16LE(buffer) : convertFromCP1251(buffer);
                if (str.length() > field.maxSize) {
                    str = str.left(field.maxSize);
                }
                result[field.key] = str;
            } else {
                result[field.key] = QString("");
            }
        } else if (field.dataType == MRIM_FD_SUBBUFFER) {
            QByteArray buffer(field.subbufferSize, 0);
            stream.readRawData(buffer.data(), field.subbufferSize);
            result[field.key] = buffer;
        } else {
            // Use function pointer
            auto reader = getReader(field.dataType);
            QVariant val = reader(stream, utf16required);
            result[field.key] = val;

            // Validate constant values
            if (field.constantValue.isValid()) {
                Q_ASSERT(val == field.constantValue);
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
    return QString::fromLatin1(data);
}

QByteArray MrimMessage::convertToUTF16LE(const QString& str)
{
    auto toUtf16 = QStringEncoder(QStringEncoder::Utf16);
    return toUtf16(str);
}

QString MrimMessage::convertFromUTF16LE(const QByteArray& data)
{
    auto fromUtf16 = QStringDecoder(QStringDecoder::Utf16);
    return fromUtf16(data);
}
