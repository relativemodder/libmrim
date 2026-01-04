#ifndef REDIRECTOR_H
#define REDIRECTOR_H

#include <QObject>
#include <QTcpSocket>
#include <QDataStream>

class Redirector : public QObject
{
    Q_OBJECT
public:
    explicit Redirector(QString redirectorServerAddress, QObject *parent = nullptr);

public slots:
    void checkAvailableServer();

signals:
    void serverAvailable(QString address);
    void connected();
    void disconnected();
    void error(QString message);

private:
    QString m_redirectorServerAddress;
    QTcpSocket *socket;
};

#endif // REDIRECTOR_H
