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
