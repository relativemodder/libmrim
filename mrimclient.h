#ifndef MRIMCLIENT_H
#define MRIMCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QByteArray>
#include <QMap>
#include "mrimmessage.h"

namespace MRIM {

    const quint32 CS_MAGIC = 0xDEADBEEF;

    enum Command {
        CS_HELLO = 0x1001,
        CS_HELLO_ACK = 0x1002,
        CS_LOGIN = 0x1003,
        CS_LOGIN_ACK = 0x1004,
        CS_LOGIN_REJ = 0x1005,
        CS_PING = 0x1006,
        CS_MESSAGE = 0x1008,
        CS_MESSAGE_ACK = 0x1009,
        CS_MESSAGE_RECV = 0x1011,
        CS_MESSAGE_STATUS = 0x1012,
        CS_LOGOUT = 0x1013,
        CS_USER_INFO = 0x1015,
        CS_CONTACT_LIST2 = 0x1028,
        CS_LOGIN2 = 0x1038,
        CS_SSL = 0x1086,
        CS_SSL_ACK = 0x1087
    };

    enum Status {
        STATUS_OFFLINE = 0x00000000,
        STATUS_ONLINE = 0x00000001,
        STATUS_AWAY = 0x00000002,
        STATUS_UNDETERMINATED = 0x00000003,
        STATUS_USER_DEFINED = 0x00000004
    };

    enum MessageStatus {
        DELIVERED = 0x0000,
        USER_DOESNT_EXIST = 0x8001,
        INTERNAL_SERVER_ERROR = 0x8003,
        OFFLINE_MESSAGES_LIMIT = 0x8004,
        MESSAGE_IS_TOO_BIG = 0x8005,
        USER_DOESNT_RECEIVE_OFFLINE_MESSAGES = 0x8006
    };

    const int HEADER_SIZE = 44;

};

struct MrimPacketHeader {
    quint32 magic;      // DEADBEEF
    quint32 proto;      // protocol version
    quint32 seq;        // packet number
    quint32 msg;        // command
    quint32 dlen;       // data length
    quint32 from;       // sender IP
    quint32 fromPort;   // sender port
    quint8 reserved[16]; // don't touch

    MrimPacketHeader()
        : magic(MRIM::CS_MAGIC)
        , proto(0x10010010)
        , seq(0)
        , msg(0)
        , dlen(0)
        , from(0)
        , fromPort(0)
    {
        memset(reserved, 0, 16);
    }
};

class MrimClient : public QObject
{
    Q_OBJECT
public:
    explicit MrimClient(QObject *parent = nullptr);
    ~MrimClient();

    void connectToServer(const QString& address);

    void login(const QString& username, const QString& password, quint32 status = MRIM::STATUS_ONLINE);

    void disconnect();

    void sendMessage(const QString& to, const QString& message);

    bool isConnected() const;

    QString getUsername() const { return m_username; }

signals:
    void connected();
    void disconnected();
    void error(const QString& errorString);

    // auth
    void loginSuccessful();
    void loginRejected(const QString& reason);

    // data receive signals
    void userInfoReceived(const QMap<QString, QVariant>& info);
    void messageReceived(const QString& from, const QString& message);
    void messageStatusReceived(MRIM::MessageStatus status);
    void contactListReceived(const QByteArray& data);

    // debug
    void packetReceived(quint32 command, const QByteArray& data);
    void packetSent(quint32 command, const QByteArray& data);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError socketError);
    void onPingTimeout();

private:
    void sendPacket(quint32 command, const QByteArray& data = QByteArray());

    void processPacket(const MrimPacketHeader& header, const QByteArray& data);
    void processHelloAck(const QByteArray& data);
    void processLoginAck(const QByteArray& data);
    void processLoginRej(const QByteArray& data);
    void processUserInfo(const QByteArray& data);
    void processMessage(const QByteArray& data);
    void processSendMessageStatus(const QByteArray& data);

    QByteArray serializeHeader(const MrimPacketHeader& header);
    MrimPacketHeader deserializeHeader(const QByteArray& data);

    void initMessageConstructors();

private:
    QTcpSocket* m_socket;
    QTimer* m_pingTimer;

    QString m_username;
    QString m_password;
    QString m_serverHost;
    quint16 m_serverPort;

    quint32 m_seq;
    quint32 m_proto;
    quint32 m_pingInterval;

    QByteArray m_receiveBuffer;
    bool m_waitingForRedirector;

    // message constructors
    MrimMessage* m_helloAckConstructor;
    MrimMessage* m_userInfoConstructor;
    MrimMessage* m_loginRejConstructor;
    MrimMessage* m_messageStatusConstructor;
};

#endif // MRIMCLIENT_H
