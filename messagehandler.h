// Auto-generated file. Do not edit manually!
// Generated from protocol.xml

#ifndef MESSAGEHANDLER_H
#define MESSAGEHANDLER_H

#include <memory>
#include <map>
#include <QByteArray>
#include "mrimmessage.h"

class MrimProtocol;

// Base handler interface
class MessageHandler
{
public:
    virtual ~MessageHandler() = default;
    virtual void handle(const QByteArray& data, MrimProtocol* protocol) = 0;
};

// Auto-generated handler classes

class HelloAckHandler : public MessageHandler {
public:
    HelloAckHandler();
    void handle(const QByteArray& data, MrimProtocol* protocol) override;
private:
    std::unique_ptr<MrimMessage> m_constructor;
};

class LoginAckHandler : public MessageHandler {
public:
    LoginAckHandler();
    void handle(const QByteArray& data, MrimProtocol* protocol) override;
private:
    std::unique_ptr<MrimMessage> m_constructor;
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

// Handler registry
class MessageHandlerRegistry {
public:
    void registerHandler(quint32 command, std::unique_ptr<MessageHandler> handler);
    MessageHandler* getHandler(quint32 command);
private:
    std::map<quint32, std::unique_ptr<MessageHandler>> m_handlers;
};

#endif // MESSAGEHANDLER_H
