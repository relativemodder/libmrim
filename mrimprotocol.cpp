#include "mrimprotocol.h"


MrimProtocol::MrimProtocol(MrimConnection *connection, QObject *parent)
    : QObject(parent)
    , m_connection(connection)
    , m_pingTimer(new QTimer(this))
    , m_seq(1)
    , m_proto(0x10010010) // 1.16
    , m_pingInterval(5000) // i still feel alive
{
    connect(m_connection, &MrimConnection::packetReceived, this, &MrimProtocol::onPacketReceived);
    connect(m_pingTimer, &QTimer::timeout, this, &MrimProtocol::onPingTimeout);

    initHandlers();
}

void MrimProtocol::initHandlers()
{
    m_registry.registerHandler(MRIM::CS_HELLO_ACK, std::make_unique<HelloAckHandler>());
    m_registry.registerHandler(MRIM::CS_LOGIN_ACK, std::make_unique<LoginAckHandler>());
    m_registry.registerHandler(MRIM::CS_LOGIN_REJ, std::make_unique<LoginRejHandler>());
    m_registry.registerHandler(MRIM::CS_USER_INFO, std::make_unique<UserInfoHandler>());
    m_registry.registerHandler(MRIM::CS_MESSAGE_STATUS, std::make_unique<MessageStatusHandler>());
}


void MrimProtocol::sendHello()
{
    sendPacket(MRIM::CS_HELLO);
}

void MrimProtocol::sendLogin(const QString &username, const QString &password, quint32 status)
{
    MrimMessage constructor;
    constructor.field("login", MRIM_FD_UBIART_LIKE_STRING)
        ->field("password", MRIM_FD_UBIART_LIKE_STRING)
        ->field("status", MRIM_FD_UINT32)
        ->field("userAgent", MRIM_FD_UBIART_LIKE_STRING);

    QMap<QString, QVariant> data;
    data["login"] = username;
    data["password"] = password;
    data["status"] = status;
    data["userAgent"] = "Qt MRIM Client v" MRIM_LIB_VERSION;

    sendPacket(MRIM::CS_LOGIN2, constructor.write(data)); // don't ask for login3, but maybe, just maybe, we will implement this
}

void MrimProtocol::sendMessage(const QString &to, const QString &message)
{
    MrimMessage constructor;
    constructor.field("flags", MRIM_FD_UINT32)
        ->field("to", MRIM_FD_UBIART_LIKE_STRING)
        ->field("message", MRIM_FD_UNICODE_STRING)
        ->field("rtfMessage", MRIM_FD_UBIART_LIKE_STRING);

    QMap<QString, QVariant> data;
    data["flags"] = 0x0;
    data["to"] = to;
    data["message"] = message;
    data["rtfMessage"] = "";

    sendPacket(MRIM::CS_MESSAGE, constructor.write(data, true));
}

void MrimProtocol::sendPing()
{
    sendPacket(MRIM::CS_PING);
}

void MrimProtocol::sendPacket(quint32 command, const QByteArray &data)
{
    MrimPacketHeader header;
    header.proto = m_proto;
    header.seq = m_seq++;
    header.msg = command;
    header.dlen = data.size();

    m_connection->sendPacket(header, data);
}

void MrimProtocol::onPacketReceived(const MrimPacketHeader &header, const QByteArray &data)
{
    MessageHandler* handler = m_registry.getHandler(header.msg);
    if (handler) {
        handler->handle(data, this);
    }
    else {
        qDebug() << "No handler for command" << Qt::hex << header.msg;
    }
}

void MrimProtocol::onHelloAck(quint32 pingInterval)
{
    m_pingInterval = pingInterval * 1000;
    m_pingTimer->start(m_pingInterval);
}

void MrimProtocol::onLoginAck()
{
    emit loginSuccessful();
}

void MrimProtocol::onLoginRej(const QString &reason)
{
    emit loginRejected(reason);
}

void MrimProtocol::onUserInfo(const QMap<QString, QVariant> &info)
{
    emit userInfoReceived(info);
}

void MrimProtocol::onMessageStatus(quint32 status)
{
    emit messageStatusReceived(MRIM::MessageStatus(status));
}

void MrimProtocol::onPingTimeout()
{
    sendPing();
}
