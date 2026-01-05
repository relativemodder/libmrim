#ifndef MESSAGEHANDLER_H
#define MESSAGEHANDLER_H

#include <QObject>
#include <QMap>
#include <memory>
#include "mrimmessage.h"

class MrimProtocol;

class MessageHandler
{
public:
    virtual ~MessageHandler() = default;
    virtual void handle(const QByteArray& data, MrimProtocol* protocol) = 0; // some fucking C++ magic made me write this =0
};


class HelloAckHandler : public MessageHandler {
public:
    HelloAckHandler();
    void handle(const QByteArray& data, MrimProtocol* protocol) override;
private:
    std::unique_ptr<MrimMessage> m_constructor;
};


class LoginAckHandler : public MessageHandler {
public:
    void handle(const QByteArray& data, MrimProtocol* protocol) override;
};


class LoginRejHandler : public MessageHandler {
public:
    LoginRejHandler();
    void handle(const QByteArray& data, MrimProtocol* protocol) override;
private:
    std::unique_ptr<MrimMessage> m_constructor;
};


class UserInfoHandler : public MessageHandler {
public:
    UserInfoHandler();
    void handle(const QByteArray& data, MrimProtocol* protocol) override;
private:
    std::unique_ptr<MrimMessage> m_constructor;
};


class MessageStatusHandler : public MessageHandler {
public:
    MessageStatusHandler();
    void handle(const QByteArray& data, MrimProtocol* protocol) override;
private:
    std::unique_ptr<MrimMessage> m_constructor;
};


class MessageHandlerRegistry {
public:
    void registerHandler(quint32 command, std::unique_ptr<MessageHandler> handler);
    MessageHandler* getHandler(quint32 command);
private:
    std::map<quint32, std::unique_ptr<MessageHandler>> m_handlers;
};


#endif // MESSAGEHANDLER_H
