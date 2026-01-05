#include "messagehandler.h"
#include "mrimprotocol.h"

// HelloAck
HelloAckHandler::HelloAckHandler() {
    m_constructor = std::make_unique<MrimMessage>();
    m_constructor->field("pingInterval", MRIM_FD_UINT32);
}

void HelloAckHandler::handle(const QByteArray& data, MrimProtocol* protocol) {
    QMap<QString, QVariant> parsed = m_constructor->read(data);
    protocol->onHelloAck(parsed["pingInterval"].toUInt());
}

// LoginAck
void LoginAckHandler::handle(const QByteArray& data, MrimProtocol* protocol) {
    protocol->onLoginAck();
}


//LoginRejected
LoginRejHandler::LoginRejHandler() {
    m_constructor = std::make_unique<MrimMessage>();
    m_constructor->field("reason", MRIM_FD_UBIART_LIKE_STRING);
}

void LoginRejHandler::handle(const QByteArray& data, MrimProtocol* protocol) {
    QMap<QString, QVariant> parsed = m_constructor->read(data);
    protocol->onLoginRej(parsed["reason"].toString());
}


// UserInfo
UserInfoHandler::UserInfoHandler() {
    m_constructor = std::make_unique<MrimMessage>();
    m_constructor->field("key1", MRIM_FD_UBIART_LIKE_STRING)
        ->field("nickname", MRIM_FD_UNICODE_STRING)
        ->field("key2", MRIM_FD_UBIART_LIKE_STRING)
        ->field("totalMessages", MRIM_FD_UNICODE_STRING)
        ->field("key3", MRIM_FD_UBIART_LIKE_STRING)
        ->field("unreadMessages", MRIM_FD_UNICODE_STRING)
        ->field("key4", MRIM_FD_UBIART_LIKE_STRING)
        ->field("endpoint", MRIM_FD_UNICODE_STRING);
}

void UserInfoHandler::handle(const QByteArray &data, MrimProtocol *protocol)
{
    QMap<QString, QVariant> parsed = m_constructor->read(data, true);
    protocol->onUserInfo(parsed);
}


// MessageStatus
MessageStatusHandler::MessageStatusHandler() {
    m_constructor = std::make_unique<MrimMessage>();
    m_constructor->field("status", MRIM_FD_UINT32);
}

void MessageStatusHandler::handle(const QByteArray &data, MrimProtocol *protocol)
{
    QMap<QString, QVariant> parsed = m_constructor->read(data);
    protocol->onMessageStatus(parsed["status"].toUInt());
}


// Registry

void MessageHandlerRegistry::registerHandler(quint32 command, std::unique_ptr<MessageHandler> handler)
{
    m_handlers[command] = std::move(handler);
}

MessageHandler *MessageHandlerRegistry::getHandler(quint32 command)
{
    auto it = m_handlers.find(command);
    return it != m_handlers.end() ? it->second.get() : nullptr;
}
