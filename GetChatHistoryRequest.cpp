#include "GetChatHistoryRequest.h"

#include "ProtocolFormatSrings.h"

#include <QDebug>

const ProtocolFormat::ProtocolStringLiteral requestTypeValue = ProtocolFormat::ProtocolStringLiteral::GET_CHAT_HISTORY_REQUEST_TYPE;
const QString typeParameterString = getProtocolStringLiteral(requestTypeValue);
const QString requestTypeKeyNameString = getProtocolStringLiteral(ProtocolFormat::ProtocolStringLiteral::REQUEST_TYPE_KEY_NAME);
const QString chatHistoryKeyNameString = getProtocolStringLiteral(ProtocolFormat::ProtocolStringLiteral::CHAT_HISTORY_KEY_NAME);

GetChatHistoryRequest::GetChatHistoryRequest(std::function<void (QJsonArray)> callback) :
    callback(std::move(callback))
{

}

ProtocolFormat::ProtocolStringLiteral GetChatHistoryRequest::requestType()
{
    return requestTypeValue;
}

QJsonObject GetChatHistoryRequest::request()
{
    QJsonObject request;
    request.insert(getProtocolStringLiteral(ProtocolFormat::ProtocolStringLiteral::REQUEST_TYPE_KEY_NAME),
                   getProtocolStringLiteral(requestTypeValue));

    return request;
}

void GetChatHistoryRequest::onResponse(QJsonObject response)
{
    if(response.find(requestTypeKeyNameString) == response.end()){
        qDebug() << "No key: " << requestTypeKeyNameString << " in response";
        return;
    }

    if(response.value(requestTypeKeyNameString) != typeParameterString){
        qDebug() << "Invalid response type";
        return;
    }

    if(response.find(chatHistoryKeyNameString) == response.end()){
        qDebug() << "No key: " << chatHistoryKeyNameString << " in response";
        return;
    }

    auto chatHistory = response.value(chatHistoryKeyNameString).toArray();
    callback(std::move(chatHistory));
}
