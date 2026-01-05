#include "mrimconnection.h"

MrimConnection::MrimConnection(QObject *parent)
    : QObject{parent}
    , m_socket(new QTcpSocket(this))
{
    connect(m_socket, &QTcpSocket::connected, this, &MrimConnection::connected);
    connect(m_socket, &QTcpSocket::disconnected, this, &MrimConnection::disconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &MrimConnection::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &MrimConnection::onError);
}

void MrimConnection::connectToHost(const QString &host, quint16 port)
{
    m_socket->connectToHost(host, port);
}

void MrimConnection::disconnectFromHost()
{
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
    }
}

void MrimConnection::sendPacket(const MrimPacketHeader &header, const QByteArray &data)
{
    QByteArray packet = serializeHeader(header);
    packet.append(data);
    m_socket->write(packet);
    m_socket->flush();
}

bool MrimConnection::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

void MrimConnection::onReadyRead()
{
    m_receiveBuffer.append(m_socket->readAll());

    const int HEADER_SIZE = 44;

    while (m_receiveBuffer.size() >= HEADER_SIZE) {
        MrimPacketHeader header = deserializeHeader(m_receiveBuffer.left(HEADER_SIZE));

        if (header.magic != 0xDEADBEEF) {
            qWarning() << "Incorrect header magic:" << Qt::hex << header.magic;
            m_receiveBuffer.clear();
            return;
        }

        if (m_receiveBuffer.size() < HEADER_SIZE + header.dlen) {
            return; // waiting for more
        }

        QByteArray data = m_receiveBuffer.mid(HEADER_SIZE, header.dlen);
        m_receiveBuffer.remove(0, HEADER_SIZE + header.dlen);

        emit packetReceived(header, data);
    }
}

void MrimConnection::onError(QAbstractSocket::SocketError socketError)
{
    emit error(m_socket->errorString());
}

QByteArray MrimConnection::serializeHeader(const MrimPacketHeader &header)
{
    QByteArray result;
    QDataStream stream(&result, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);

    stream << header.magic << header.proto << header.seq << header.msg << header.dlen << header.from << header.fromPort;
    stream.writeRawData((const char*)header.reserved, 16);

    return result;
}

MrimPacketHeader MrimConnection::deserializeHeader(const QByteArray &data)
{
    MrimPacketHeader header;
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::LittleEndian);

    stream >> header.magic >> header.proto >> header.seq >> header.msg >> header.dlen >> header.from >> header.fromPort;
    stream.readRawData((char*)header.reserved, 16);

    return header;
}

