#include "ChatRequest.h"

#include <QJsonDocument>

#include <TcpDataTransmitter.h>

#include <QDebug>

ChatRequest::ChatRequest()
{

}

void ChatRequest::doRequest(QTcpSocket &socket)
{
    auto newRequest = request();
    QJsonDocument requestDocument;
    requestDocument.setObject(newRequest);

    if(!TcpDataTransmitter::sendData(requestDocument.toJson(), socket)){
        qDebug() << "Chat request failed";
        return;
    }

    if(!socket.waitForReadyRead()){
        qDebug() << "No response to chat request";
        return;
    }

    auto response = TcpDataTransmitter::receiveData(socket);
    QJsonParseError jsonParseError;
    auto responseDocument = QJsonDocument::fromJson(response, &jsonParseError);
    qDebug() << "response: " << response;
    if(responseDocument.isNull()){
        qDebug() << "Response parse error: " << jsonParseError.errorString();
        return;
    }
    if(!responseDocument.isObject()){
        qDebug() << "Response is not JSON object";
        return;
    }

    onResponse(responseDocument.object());
}
