#include "mrim/redirector.h"
#include <QTcpSocket>
#include <QDebug>

Redirector::Redirector(QString redirectorServerAddress, QObject *parent)
    : QObject{parent},
    m_redirectorServerAddress(redirectorServerAddress)
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

    auto addrSplit = m_redirectorServerAddress.split(":");

    QHostAddress address(addrSplit.first());

    if (addrSplit.count() < 2) {
        emit error("You need to specify a port!");
        return;
    }

    auto port = addrSplit.last().toUInt();
    socket->connectToHost(address, port);
}
