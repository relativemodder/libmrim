#include "mrim/mrimclient.h"

MrimClient::MrimClient(QObject *parent)
    : QObject(parent)
    , m_connection(new MrimConnection(this))
    , m_protocol(new MrimProtocol(m_connection, this))
{
    connect(m_connection, &MrimConnection::connected, this, &MrimClient::onConnected);
    connect(m_connection, &MrimConnection::disconnected, this, &MrimClient::disconnected);
    connect(m_connection, &MrimConnection::error, this, &MrimClient::error);

    connect(m_protocol, &MrimProtocol::loginSuccessful, this, &MrimClient::loginSuccessful);
    connect(m_protocol, &MrimProtocol::loginRejected, this, &MrimClient::loginRejected);
    connect(m_protocol, &MrimProtocol::userInfoReceived, this, &MrimClient::userInfoReceived);
    connect(m_protocol, &MrimProtocol::messageStatusReceived, this, &MrimClient::messageStatusReceived);
    connect(m_protocol, &MrimProtocol::messageReceived, this, &MrimClient::handleIncomingMessage);
}

void MrimClient::connectToServer(const QString &host, quint16 port)
{
    m_connection->connectToHost(host, port);
}

void MrimClient::login(const QString &username, const QString &password, quint32 status)
{
    m_username = username;
    m_password = password;
    m_protocol->sendLogin(username, password, status);
}

void MrimClient::disconnect()
{
    m_connection->disconnectFromHost();
}

void MrimClient::sendMessage(const QString &to, const QString &message)
{
    m_protocol->sendMessage(to, message);
}

bool MrimClient::isConnected() const
{
    return m_connection->isConnected();
}

void MrimClient::onConnected()
{
    emit connected();
    m_protocol->sendHello();
}

void MrimClient::handleIncomingMessage(quint32 messageId, quint32 flags,
                                       QString from, QString message,
                                       QString rtfMessage) {
    m_protocol->sendMessageReceipt(messageId, from); // yeah, we received it

    // is it offline?
    if (flags & 0x00000001) {
        emit offlineMessageReceived(from, message);
    } else {
        emit messageReceived(from, message);
    }

    // WIP
    // 0x00000008 - auth request
    // 0x00000200 - forwarded contacts
    // 0x00000400 - "typing"
}
