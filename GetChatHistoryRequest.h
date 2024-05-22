#ifndef GETCHATHISTORYREQUEST_H
#define GETCHATHISTORYREQUEST_H

#include <ChatRequest.h>

#include <QJsonArray>

#include <functional>

class GetChatHistoryRequest : public ChatRequest
{
public:
    GetChatHistoryRequest(std::function<void(QJsonArray)> callback);

    virtual ProtocolFormat::ProtocolStringLiteral requestType() override;

    virtual QJsonObject request() override;
    virtual void onResponse(QJsonObject response) override;

private:
    std::function<void(QJsonArray)> callback;
};

#endif // GETCHATHISTORYREQUEST_H
