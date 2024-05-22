#include "TcpClientWorker.h"

#include <QHostAddress>
#include <QJsonDocument>

#include "GetChatHistoryRequest.h"
#include "SendChatMessage.h"

#include "TcpDataTransmitter.h"
#include "ProtocolFormatSrings.h"

const QHostAddress defaultHost = QHostAddress::LocalHost;
const quint16 defaultPort = 44000;

const QString requestTypeStringLiteral = ProtocolFormat::getProtocolStringLiteral(ProtocolFormat::ProtocolStringLiteral::REQUEST_TYPE_KEY_NAME);

TcpClientWorker::TcpClientWorker(QObject *parent)
    : QObject{parent},
      currentRequest(nullptr)
{
    connect(&workerSocket, &QTcpSocket::readyRead, this, &TcpClientWorker::onReadyRead);
    connect(&workerSocket, &QTcpSocket::stateChanged, this, &TcpClientWorker::onSocketStateChanged);
}

void TcpClientWorker::addGetChatRequest()
{
    auto request = std::make_shared<GetChatHistoryRequest>([this](QJsonArray history){
            emit chatHistoryReceived(history);
    });
    requestQueue.push(request);

    continueRequestProcessing();
}

void TcpClientWorker::addSendChatMessageRequest(QJsonObject message)
{
    auto request = std::make_shared<SendChatMessage>(message, [this](){
            emit chatMessageSent();
    });
    requestQueue.push(request);

    continueRequestProcessing();
}

void TcpClientWorker::start()
{
    workerSocket.connectToHost(defaultHost, defaultPort);
}

void TcpClientWorker::onReadyRead()
{
    auto response = TcpDataTransmitter::receiveData(workerSocket);
    QJsonParseError jsonParseError;
    auto responseDocument = QJsonDocument::fromJson(response, &jsonParseError);
    qDebug() << "response: " << response;
    if(responseDocument.isNull()){
        qDebug() << "Response parse error: " << jsonParseError.errorString();
        requestFinished();
        return;
    }
    if(!responseDocument.isObject()){
        qDebug() << "Response is not JSON object";
        requestFinished();
        return;
    }

    auto responseDocumentObject = responseDocument.object();
    auto responseRequestType = responseDocumentObject.value(requestTypeStringLiteral).toString();
    if(responseRequestType == ProtocolFormat::getProtocolStringLiteral(ProtocolFormat::ProtocolStringLiteral::NOTIFICATION_REQUEST_TYPE)){
        processNotification(responseDocumentObject);
    }
    else if((currentRequest != nullptr) &&
            (responseRequestType == ProtocolFormat::getProtocolStringLiteral(currentRequest->requestType()))){
        currentRequest->onResponse(responseDocument.object());
    }

    requestFinished();
}

void TcpClientWorker::onSocketStateChanged(QAbstractSocket::SocketState state)
{
    switch (state) {
    case QTcpSocket::UnconnectedState:{
        emit noConnectionToServer();
        qDebug() << "No connection to server";
        break;
    }
    case QTcpSocket::ConnectedState:{
        emit connectionToServerEstablished();
    }
    }
}

void TcpClientWorker::processTopRequest()
{
    if(currentRequest != nullptr){
        qDebug() << "Request is already in process!";
        return;
    }

    currentRequest = requestQueue.front();

    auto newRequest = currentRequest->request();
    QJsonDocument requestDocument;
    requestDocument.setObject(newRequest);

    if(!TcpDataTransmitter::sendData(requestDocument.toJson(), workerSocket)){
        qDebug() << "Chat request failed";
        return;
    }
}

void TcpClientWorker::processNotification(const QJsonObject& object)
{
    if(object.value(ProtocolFormat::getProtocolStringLiteral(ProtocolFormat::ProtocolStringLiteral::NOTIFICATION_TYPE_KEY_NAME)).toString() ==
            ProtocolFormat::getProtocolStringLiteral(ProtocolFormat::ProtocolStringLiteral::NOTIFICATION_MESSAGES_UPDATED)){
        emit chatHasBeenUpdated();
    }
}

bool TcpClientWorker::isInRequestProcessing() const
{
    return currentRequest != nullptr;
}

void TcpClientWorker::continueRequestProcessing()
{
    if(!isInRequestProcessing()){
        processTopRequest();
    }
}

void TcpClientWorker::requestFinished()
{
    currentRequest = nullptr;
    if(requestQueue.size() != 0){
        requestQueue.pop();
    }

    if(requestQueue.size() != 0){
        processTopRequest();
    }
}
