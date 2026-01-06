#ifndef MRIMCLIENT_H
#define MRIMCLIENT_H

#include <QObject>
#include "mrimconnection.h"
#include "mrimprotocol.h"

class MrimClient : public QObject {
    Q_OBJECT
public:
    explicit MrimClient(QObject *parent = nullptr);

    void connectToServer(const QString& host, quint16 port);
    void login(const QString& username, const QString& password,
               quint32 status = MRIM::STATUS_ONLINE);
    void disconnect();
    void sendMessage(const QString& to, const QString& message);
    bool isConnected() const;

signals:
    void connected();
    void disconnected();
    void error(const QString& errorString);
    void loginSuccessful();
    void loginRejected(const QString& reason);
    void userInfoReceived(const QMap<QString, QVariant>& info);
    void messageStatusReceived(MRIM::MessageStatus status);
    void messageReceived(const QString& from, const QString& text);
    void offlineMessageReceived(const QString& from, const QString& text);

private slots:
    void onConnected();
    void handleIncomingMessage(quint32 messageId, quint32 flags,
                               QString from, QString message, QString rtfMessage);

private:
    MrimConnection* m_connection;
    MrimProtocol* m_protocol;
    QString m_username;
    QString m_password;
};

#endif // MRIMCLIENT_H
