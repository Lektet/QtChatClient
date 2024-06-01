#include "TcpClientWorker.h"

#include <QHostAddress>
#include <QJsonDocument>

#include "GetChatHistoryRequest.h"
#include "SendChatMessage.h"

#include "MessageType.h"
#include "MessageUtils.h"
#include "GetHistoryMessage.h"
#include "GetHistoryResponseMessage.h"
#include "SendMessageMessage.h"
#include "SendMessageResponseMessage.h"
#include "NotificationMessage.h"
#include "Result.h"
#include "NotificationType.h"

#include "TcpDataTransmitter.h"
#include "ProtocolFormatSrings.h"

const QHostAddress defaultHost = QHostAddress::LocalHost;
const quint16 defaultPort = 44000;

const QString requestTypeStringLiteral = ProtocolFormat::getProtocolStringLiteral(ProtocolFormat::ProtocolStringLiteral::REQUEST_TYPE_KEY_NAME);

TcpClientWorker::TcpClientWorker(QObject *parent)
    : QObject{parent},
      currentRequest(nullptr),
      lastSocketState(QTcpSocket::UnconnectedState),
      inRequestProcessing(false)
{
    connect(&workerSocket, &QTcpSocket::readyRead, this, &TcpClientWorker::onReadyRead);
    connect(&workerSocket, &QTcpSocket::stateChanged, this, &TcpClientWorker::onSocketStateChanged);
}

void TcpClientWorker::addGetChatRequest()
{
    requestQueue.push(std::make_shared<GetHistoryMessage>());
    continueRequestProcessing();
}

void TcpClientWorker::addSendChatMessageRequest(QJsonObject message)
{
    requestQueue.push(std::make_shared<SendMessageMessage>(message));
    continueRequestProcessing();
}

void TcpClientWorker::start()
{
    workerSocket.connectToHost(defaultHost, defaultPort);
}

void TcpClientWorker::stop()
{
    workerSocket.disconnectFromHost();
}

bool TcpClientWorker::isDisconnected()
{
    std::lock_guard<std::mutex> lock(socketMutex);
    qDebug() << "socket last state: " << lastSocketState;
    qDebug() << "socket last state is unconnected: " << (lastSocketState == QTcpSocket::UnconnectedState);
    return  lastSocketState == QTcpSocket::UnconnectedState;
}

void TcpClientWorker::onReadyRead()
{
    auto receivedData = TcpDataTransmitter::receiveData(workerSocket);
    QJsonParseError jsonParseError;
    auto receivedDocument = QJsonDocument::fromJson(receivedData, &jsonParseError);
//    qDebug() << "response: " << receivedData;
    if(receivedDocument.isNull()){
        qWarning() << "Response parse error: " << jsonParseError.errorString();
        requestFinished();
        return;
    }
    if(!receivedDocument.isObject()){
        qWarning() << "Response is not JSON object";
        requestFinished();
        return;
    }

    auto receivedMessage = MessageUtils::createMessageFromJson(receivedDocument);

    auto receivedMessageType = receivedMessage->getMessageType();
    qDebug() << "responseRequestType: " << messageTypeToString(receivedMessageType);
    if(receivedMessageType == MessageType::Notification){
        auto notificationMessage = std::static_pointer_cast<NotificationMessage>(receivedMessage);
        processNotification(notificationMessage);
        return;
    }

    if(!inRequestProcessing){
        qDebug() << "No response to be expected";
        return;
    }

    if(currentRequest == nullptr){
        qCritical() << "Invalid pointer";
        return;
    }

    auto sentMessageType = currentRequest->getMessageType();
    qDebug() << "current request type: " << messageTypeToString(sentMessageType);
    switch (receivedMessageType){
        case MessageType::GetHistoryResponse:{
            if(currentRequest->getMessageType() != MessageType::GetHistory){
                qDebug() << "Invalid message type";
                return;
            }

            auto responseMessage = std::static_pointer_cast<GetHistoryResponseMessage>(receivedMessage);
            emit chatHistoryReceived(responseMessage->getMessagesHistory());
            break;
        }
        case MessageType::SendMessageResponse:{
            if(currentRequest->getMessageType() != MessageType::SendMessage){
                qDebug() << "Invalid message type";
                break;
            }

            auto responseMessage = std::static_pointer_cast<SendMessageResponseMessage>(receivedMessage);
            if(responseMessage->getResult() != Result::Success){
                qDebug() << "Message sent failed";
            }
            else{
                emit chatMessageSentSuccess();
            }
            break;
        }
        default:
            break;
    }

    requestFinished();
}

void TcpClientWorker::onSocketStateChanged(QAbstractSocket::SocketState state)
{
    qDebug() << "Socket state changed: " << state;
    socketMutex.lock();
    lastSocketState = state;
    socketMutex.unlock();
    switch (state) {
        case QTcpSocket::UnconnectedState:{
            emit noConnectionToServer();
            qDebug() << "No connection to server";
            break;
        }
        case QTcpSocket::ConnectedState:{
            emit connectionToServerEstablished();
        }
        default:
            break;
    }
}

void TcpClientWorker::processTopRequest()
{
    if(currentRequest != nullptr){
        qDebug() << "Request is already in process!";
        return;
    }

    inRequestProcessing = true;
    currentRequest = requestQueue.front();
    qDebug() << "Request to send: " << messageTypeToString(currentRequest->getMessageType());
    if(!TcpDataTransmitter::sendData(currentRequest->toJson().toJson(), workerSocket)){
        qDebug() << "Chat request failed";
        return;
    }
}

void TcpClientWorker::processNotification(std::shared_ptr<NotificationMessage> notitification)
{
    if(notitification->getNotificationType() == NotificationType::MessagesUpdated){
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
    inRequestProcessing = false;
    currentRequest = nullptr;
    if(requestQueue.size() != 0){
        requestQueue.pop();
    }

    if(requestQueue.size() != 0){
        processTopRequest();
    }
}
