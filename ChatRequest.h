#ifndef CHATREQUEST_H
#define CHATREQUEST_H

#include <QTcpSocket>

#include <QJsonObject>
#include "ProtocolFormatSrings.h"

class ChatRequest
{
public:
    explicit ChatRequest();

    void doRequest(QTcpSocket &socket);

    virtual ProtocolFormat::ProtocolStringLiteral requestType() = 0;

    virtual QJsonObject request() = 0;
    virtual void onResponse(QJsonObject response) = 0;
};

#endif // CHATREQUEST_H
