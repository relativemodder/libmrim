#ifndef MRIMCONNECTION_H
#define MRIMCONNECTION_H

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>

struct MrimPacketHeader {
    quint32 magic;
    quint32 proto;
    quint32 seq;
    quint32 msg;
    quint32 dlen;
    quint32 from;
    quint32 fromPort;
    quint8 reserved[16];

    MrimPacketHeader() : magic(0xDEADBEEF), proto(0x10010010),
        seq(0), msg(0), dlen(0), from(0), fromPort(0) {
        memset(reserved, 0, 16);
    }
};

class MrimConnection : public QObject
{
    Q_OBJECT
public:
    explicit MrimConnection(QObject *parent = nullptr);

    void connectToHost(const QString& host, quint16 port);
    void disconnectFromHost();
    void sendPacket(const MrimPacketHeader& header, const QByteArray& data = QByteArray());
    bool isConnected() const;

signals:
    void connected();
    void disconnected();
    void packetReceived(const MrimPacketHeader& header, const QByteArray& data);
    void error(const QString& message);

private slots:
    void onReadyRead();
    void onError(QAbstractSocket::SocketError socketError);

private:
    QByteArray serializeHeader(const MrimPacketHeader& header);
    MrimPacketHeader deserializeHeader(const QByteArray& data);

    QTcpSocket* m_socket;
    QByteArray m_receiveBuffer;
};

#endif // MRIMCONNECTION_H
