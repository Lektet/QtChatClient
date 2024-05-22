#ifndef SENDCHATMESSAGE_H
#define SENDCHATMESSAGE_H

#include <ChatRequest.h>

#include <functional>

class SendChatMessage : public ChatRequest
{
public:
    SendChatMessage(const QJsonObject& message, std::function<void()> callback);

    virtual ProtocolFormat::ProtocolStringLiteral requestType() override;

    virtual QJsonObject request() override;
    virtual void onResponse(QJsonObject response) override;

private:
    QJsonObject message;
    std::function<void()> callback;
};

#endif // SENDCHATMESSAGE_H
