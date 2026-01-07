#include "mrim/redirector.h"
#include <QTcpSocket>
#include <QDebug>

Redirector::Redirector(QString redirectorServerAddress, quint32 port, QObject *parent)
    : QObject{parent},
    m_redirectorServerAddress(redirectorServerAddress),
    m_redirectorServerPort(port)
{
    socket = new QTcpSocket(this);
}

void Redirector::checkAvailableServer()
{
    connect(socket, &QTcpSocket::connected, this, &Redirector::connected);
    connect(socket, &QTcpSocket::disconnected, this, &Redirector::disconnected);
    connect(socket, &QTcpSocket::errorOccurred, [&](QAbstractSocket::SocketError socketError) {
        if (socketError == QAbstractSocket::SocketError::RemoteHostClosedError) {
            return; // Not an error lol, that's intended
        }
        emit this->error(socket->errorString());
    });

    connect(socket, &QTcpSocket::readyRead, [&] {
        QString response = socket->readAll();
        auto address = response.trimmed();
        emit this->serverAvailable(address);
    });

    qDebug() << "Redirector: Connecting to address" << m_redirectorServerAddress << "port" << m_redirectorServerPort;
    socket->connectToHost(m_redirectorServerAddress, m_redirectorServerPort);
}
