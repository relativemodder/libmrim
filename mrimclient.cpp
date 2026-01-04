#include "mrimclient.h"

MrimClient::MrimClient(QObject *parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
    , m_pingTimer(new QTimer(this))
    , m_seq(1)
    , m_proto(0x10010010)
    , m_pingInterval(5000)
    , m_waitingForRedirector(false)
{
    connect(m_socket, &QTcpSocket::connected, this, &MrimClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &MrimClient::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &MrimClient::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred,
            this, &MrimClient::onError);

    connect(m_pingTimer, &QTimer::timeout, this, &MrimClient::onPingTimeout);

    initMessageConstructors();
}

MrimClient::~MrimClient()
{
    disconnect();

    delete m_helloAckConstructor;
    delete m_userInfoConstructor;
    delete m_loginRejConstructor;
    delete m_messageStatusConstructor;
}

void MrimClient::connectToServer(const QString &address)
{
    qDebug() << "Connecting to" << address;
    auto addrSplit = address.split(":");

    QString addr(addrSplit.first());

    if (addrSplit.count() < 2) {
        emit error("You need to specify a port!");
        return;
    }

    auto port = addrSplit.last().toUInt();

    m_serverHost = addr;
    m_serverPort = port;
    m_socket->connectToHost(addr, port);
}

void MrimClient::initMessageConstructors()
{
    // HELLO_ACK
    m_helloAckConstructor = new MrimMessage(this);
    m_helloAckConstructor->field("pingInterval", MRIM_FD_UINT32);

    // USER_INFO
    m_userInfoConstructor = new MrimMessage(this);
    m_userInfoConstructor->field("key1", MRIM_FD_UBIART_LIKE_STRING)  // MRIM.NICKNAME
        ->field("nickname", MRIM_FD_UNICODE_STRING)
        ->field("key2", MRIM_FD_UBIART_LIKE_STRING)  // MESSAGES.TOTAL
        ->field("totalMessages", MRIM_FD_UNICODE_STRING)
        ->field("key3", MRIM_FD_UBIART_LIKE_STRING)  // MESSAGES.UNREAD
        ->field("unreadMessages", MRIM_FD_UNICODE_STRING)
        ->field("key4", MRIM_FD_UBIART_LIKE_STRING)  // client.endpoint
        ->field("endpoint", MRIM_FD_UNICODE_STRING);

    // LOGIN_REJ
    m_loginRejConstructor = new MrimMessage(this);
    m_loginRejConstructor->field("reason", MRIM_FD_UBIART_LIKE_STRING);

    // MESSAGE_STATUS
    m_messageStatusConstructor = new MrimMessage(this);
    m_messageStatusConstructor->field("status", MRIM_FD_UINT32);
}

void MrimClient::login(const QString& username, const QString& password, quint32 status)
{
    m_username = username;
    m_password = password;

    qDebug() << "Authorizing" << m_username;

    MrimMessage loginConstructor;
    loginConstructor.field("login", MRIM_FD_UBIART_LIKE_STRING)
        ->field("password", MRIM_FD_UBIART_LIKE_STRING)
        ->field("status", MRIM_FD_UINT32)
        ->field("userAgent", MRIM_FD_UBIART_LIKE_STRING);

    QMap<QString, QVariant> loginData;
    loginData["login"] = username;
    loginData["password"] = password;
    loginData["status"] = status;
    loginData["userAgent"] = "Qt MRIM Client " MRIM_LIB_VERSION;

    QByteArray data = loginConstructor.write(loginData);
    sendPacket(MRIM::CS_LOGIN2, data);
}

void MrimClient::disconnect()
{
    if (m_socket->state() == QAbstractSocket::UnconnectedState) {
        return;
    }
    m_pingTimer->stop();
    m_socket->disconnectFromHost();
}

void MrimClient::sendMessage(const QString& to, const QString& message)
{
    qDebug() << "Sending message to" << to << ", " << message;

    MrimMessage msgConstructor;
    msgConstructor.field("flags", MRIM_FD_UINT32)
        ->field("to", MRIM_FD_UBIART_LIKE_STRING)
        ->field("message", MRIM_FD_UNICODE_STRING)
        ->field("rtfMessage", MRIM_FD_UBIART_LIKE_STRING);

    QMap<QString, QVariant> msgData;
    msgData["flags"] = 0x0;
    msgData["to"] = to;
    msgData["message"] = message;
    msgData["rtfMessage"] = "";

    QByteArray data = msgConstructor.write(msgData, true);
    sendPacket(MRIM::CS_MESSAGE, data);
}

bool MrimClient::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

void MrimClient::onConnected()
{
    qDebug() << "Connection established";
    emit connected();

    sendPacket(MRIM::CS_HELLO);
}

void MrimClient::onDisconnected()
{
    qDebug() << "Disconnected from the server";
    m_pingTimer->stop();
    emit disconnected();
}

void MrimClient::onReadyRead()
{
    m_receiveBuffer.append(m_socket->readAll());

    while(m_receiveBuffer.size() > MRIM::HEADER_SIZE) {
        MrimPacketHeader header = deserializeHeader(m_receiveBuffer.left(MRIM::HEADER_SIZE));

        if (header.magic != MRIM::CS_MAGIC) {
            qWarning() << "Incorrect header magic:" << Qt::hex << header.magic;
            m_receiveBuffer.clear();
            return;
        }

        if (m_receiveBuffer.size() < MRIM::HEADER_SIZE + header.dlen) {
            // we're just waiting for more data
            return;
        }

        QByteArray data = m_receiveBuffer.mid(MRIM::HEADER_SIZE, header.dlen);
        m_receiveBuffer.remove(0, MRIM::HEADER_SIZE + header.dlen);

        qDebug() << "Packet received: command =" << Qt::hex << header.msg
                 << "seq =" << Qt::dec << header.seq
                 << "dlen =" << header.dlen;

        emit packetReceived(header.msg, data);
        processPacket(header, data);
    }
}

void MrimClient::onError(QAbstractSocket::SocketError socketError)
{
    qWarning() << "Socket error:" << socketError << m_socket->errorString();
    emit error(m_socket->errorString());
}

void MrimClient::onPingTimeout()
{
    qDebug() << "sending ping";
    sendPacket(MRIM::CS_PING);
}

void MrimClient::sendPacket(quint32 command, const QByteArray &data)
{
    MrimPacketHeader header;
    header.proto = m_proto;
    header.seq = m_seq++;
    header.msg = command;
    header.dlen = data.size();

    QByteArray packet = serializeHeader(header);
    packet.append(data);

    qDebug() << "Sending packet: msg =" << Qt::hex << command
             << "seq =" << Qt::dec << header.seq
             << "dlen =" << header.dlen;

    m_socket->write(packet);
    m_socket->flush();

    emit packetSent(command, data);
}

void MrimClient::processPacket(const MrimPacketHeader& header, const QByteArray& data)
{
    switch (header.msg) {
    case MRIM::CS_HELLO_ACK:
        processHelloAck(data);
        break;

    case MRIM::CS_LOGIN_ACK:
        processLoginAck(data);
        break;

    case MRIM::CS_LOGIN_REJ:
        processLoginRej(data);
        break;

    case MRIM::CS_USER_INFO:
        processUserInfo(data);
        break;

    case MRIM::CS_MESSAGE:
    case MRIM::CS_MESSAGE_ACK:
        processMessage(data);
        break;

    case MRIM::CS_MESSAGE_STATUS:
        processSendMessageStatus(data);
        break;

    case MRIM::CS_LOGOUT:
        qDebug() << "Oh, we got LOGOUT, but more like GETOUT from the server";
        disconnect();
        break;

    default:
        qDebug() << "Something unknown from the server:" << Qt::hex << header.msg;
        break;
    }
}

void MrimClient::processHelloAck(const QByteArray& data)
{
    qDebug() << "Processing HELLO_ACK";

    QMap<QString, QVariant> helloData = m_helloAckConstructor->read(data);
    m_pingInterval = helloData["pingInterval"].toUInt() * 1000; // converting to ms

    qDebug() << "PING interval:" << m_pingInterval << "ms";

    m_pingTimer->start(m_pingInterval);
}

void MrimClient::processLoginAck(const QByteArray& data)
{
    qDebug() << "Successful login";
    emit loginSuccessful();
}

void MrimClient::processLoginRej(const QByteArray& data)
{
    QMap<QString, QVariant> rejData = m_loginRejConstructor->read(data);
    QString reason = rejData["reason"].toString();

    qWarning() << "Login rejected:" << reason;
    emit loginRejected(reason);
}

void MrimClient::processUserInfo(const QByteArray& data)
{
    qDebug() << "Processing USER_INFO";

    QMap<QString, QVariant> userInfo = m_userInfoConstructor->read(data, true);

    qDebug() << "Nickname:" << userInfo["nickname"].toString();
    qDebug() << "Endpoint:" << userInfo["endpoint"].toString();
    qDebug() << "Total messages:" << userInfo["totalMessages"].toString();
    qDebug() << "Unread:" << userInfo["unreadMessages"].toString();

    emit userInfoReceived(userInfo);
}

void MrimClient::processMessage(const QByteArray& data)
{
    qDebug() << "Processing MESSAGE (data size:" << data.size() << ")";
    // WIP
}

void MrimClient::processSendMessageStatus(const QByteArray& data)
{
    qDebug() << "Processing MESSAGE_STATUS";
    QMap<QString, QVariant> statusInfo = m_messageStatusConstructor->read(data, true);

    auto status = statusInfo["status"].toUInt();

    qDebug() << "Message status:" << Qt::hex << status;

    emit messageStatusReceived(MRIM::MessageStatus(status));
}

QByteArray MrimClient::serializeHeader(const MrimPacketHeader& header)
{
    QByteArray result;
    QDataStream stream(&result, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);

    stream << header.magic;
    stream << header.proto;
    stream << header.seq;
    stream << header.msg;
    stream << header.dlen;
    stream << header.from;
    stream << header.fromPort;
    stream.writeRawData(reinterpret_cast<const char*>(header.reserved), 16);

    return result;
}

MrimPacketHeader MrimClient::deserializeHeader(const QByteArray& data)
{
    MrimPacketHeader header;
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::LittleEndian);

    stream >> header.magic;
    stream >> header.proto;
    stream >> header.seq;
    stream >> header.msg;
    stream >> header.dlen;
    stream >> header.from;
    stream >> header.fromPort;
    stream.readRawData(reinterpret_cast<char*>(header.reserved), 16);

    return header;
}
