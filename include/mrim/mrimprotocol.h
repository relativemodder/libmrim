// Auto-generated file. Do not edit manually!
// Generated from protocol.xml

#ifndef MRIMPROTOCOL_H
#define MRIMPROTOCOL_H

#include <QObject>
#include <QTimer>
#include "mrim/mrimconnection.h"
#include "mrim/messagehandler.h"

namespace MRIM {

// Auto-generated commands enum
enum Command {
    CS_HELLO = 0x1001,
    CS_HELLO_ACK = 0x1002,
    CS_LOGIN2 = 0x1038,
    CS_LOGIN_ACK = 0x1004,
    CS_LOGIN_REJ = 0x1005,
    CS_PING = 0x1006,
    CS_MESSAGE = 0x1008,
    CS_MESSAGE_ACK = 0x1009,
    CS_MESSAGE_STATUS = 0x1012,
    CS_LOGOUT = 0x1013,
    CS_USER_INFO = 0x1015,
};

enum Status {
    STATUS_OFFLINE = 0x00000000,
    STATUS_ONLINE = 0x00000001,
    STATUS_AWAY = 0x00000002,
};

enum MessageStatus {
    DELIVERED = 0x0000,
    USER_DOESNT_EXIST = 0x8001,
    INTERNAL_SERVER_ERROR = 0x8003,
};

}; // namespace MRIM

class MrimProtocol : public QObject
{
    Q_OBJECT
public:
    explicit MrimProtocol(MrimConnection* connection, QObject *parent = nullptr);
    
    // Auto-generated send methods
    void sendHello();
    void sendLogin(const QString& username, const QString& password, const quint32& status);
    void sendMessage(const QString& to, const QString& message);
    void sendPing();

    // Auto-generated callbacks (called by handlers)
    void onHelloAck(quint32 pingInterval);
    void onLoginAck();
    void onLoginRej(const QString& reason);
    void onUserInfo(const QMap<QString, QVariant>& info);
    void onMessageStatus(quint32 status);

signals:
    // Auto-generated signals
    void helloAckReceived(quint32 pingInterval);
    void loginSuccessful();
    void loginRejected(QString reason);
    void userInfoReceived(QMap<QString, QVariant> info);
    void messageStatusReceived(MRIM::MessageStatus status);

private slots:
    void onPacketReceived(const MrimPacketHeader& header, const QByteArray& data);
    void onPingTimeout();

private:
    void sendPacket(quint32 command, const QByteArray& data = QByteArray());
    void initHandlers();
    
    MrimConnection* m_connection;
    MessageHandlerRegistry m_registry;
    QTimer* m_pingTimer;
    quint32 m_seq;
    quint32 m_proto;
    quint32 m_pingInterval;
};

#endif // MRIMPROTOCOL_H
