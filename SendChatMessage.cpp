#include "SendChatMessage.h"


#include <QDebug>

const ProtocolFormat::ProtocolStringLiteral requestTypeValue = ProtocolFormat::ProtocolStringLiteral::SEND_CHAT_MESSAGE_REQUEST_TYPE;
const QString typeParameterString = ProtocolFormat::getProtocolStringLiteral(requestTypeValue);
const QString requestTypeKeyNameString = ProtocolFormat::getProtocolStringLiteral(ProtocolFormat::ProtocolStringLiteral::REQUEST_TYPE_KEY_NAME);
const QString chatMessageKeyString = ProtocolFormat::getProtocolStringLiteral(ProtocolFormat::ProtocolStringLiteral::CHAT_MESSAGE_KEY);
const QString sendChatMessageResultKeyNameString = ProtocolFormat::getProtocolStringLiteral(ProtocolFormat::ProtocolStringLiteral::SEND_CHAT_MESSAGE_RESULT_KEY_NAME);
const QString sendChatMessageResultCompletedString = ProtocolFormat::getProtocolStringLiteral(ProtocolFormat::ProtocolStringLiteral::SEND_CHAT_MESSAGE_RESULT_COMPLETED);

SendChatMessage::SendChatMessage(const QJsonObject &message, std::function<void ()> callback) :
    message(message),
    callback(std::move(callback))
{

}

ProtocolFormat::ProtocolStringLiteral SendChatMessage::requestType()
{
    return requestTypeValue;
}

QJsonObject SendChatMessage::request()
{
    QJsonObject request;
    request.insert(requestTypeKeyNameString, typeParameterString);
    request.insert(chatMessageKeyString, message);

    return request;
}

void SendChatMessage::onResponse(QJsonObject response)
{
    if(response.find(requestTypeKeyNameString) == response.end()){
        qDebug() << "No key: " << requestTypeKeyNameString << " in response";
        return;
    }

    if(response.value(requestTypeKeyNameString) != typeParameterString){
        qDebug() << "Invalid response type";
        return;
    }

    if(response.find(sendChatMessageResultKeyNameString) == response.end()){
        qDebug() << "No key: " << sendChatMessageResultKeyNameString << " in response";
        return;
    }

    if(response.value(sendChatMessageResultKeyNameString) != sendChatMessageResultCompletedString){
        qDebug() << "Invalid requst result";
        return;
    }

    callback();
}
