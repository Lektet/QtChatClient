#include "TcpClientWorker.h"

#include <QHostAddress>
#include <QJsonDocument>

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

const QHostAddress defaultHost = QHostAddress::LocalHost;
const quint16 defaultPort = 44000;

const int REQUEST_TIMEOUT = 10000;

TcpClientWorker::TcpClientWorker(QObject *parent)
    : QObject{parent},
      currentRequest(nullptr),
      lastSocketState(QTcpSocket::UnconnectedState),
      inRequestProcessing(false)
{
    requestTimer.setParent(this);
    requestTimer.setSingleShot(true);
    requestTimer.setInterval(REQUEST_TIMEOUT);
    connect(&requestTimer, &QTimer::timeout, this, &TcpClientWorker::finishRequest);

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

    bool currentRequestProcessed = false;
    for(auto& data : receivedData){
        bool responseReceived = false;
        processMessageData(data, responseReceived);
        if(currentRequestProcessed == responseReceived == true){
            qWarning() << "Inapropriate message received";
        }
        else if(responseReceived){
            currentRequestProcessed = true;
        }
    }

    if(currentRequestProcessed){
        finishRequest();
    }
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
    qDebug() << "Message to send: " << messageTypeToString(currentRequest->getMessageType());
    if(!TcpDataTransmitter::sendData(currentRequest->toJson().toJson(), workerSocket)){
        qDebug() << "Chat request failed";
        return;
    }
    requestTimer.start();
}

void TcpClientWorker::processNotification(std::shared_ptr<NotificationMessage> notitification)
{
    if(notitification->getNotificationType() == NotificationType::MessagesUpdated){
        emit chatHasBeenUpdated();
    }
}

void TcpClientWorker::processMessageData(const QByteArray &data, bool &responseReceived)
{
    QJsonParseError jsonParseError;
    auto document = QJsonDocument::fromJson(data, &jsonParseError);
//    qDebug() << "response: " << receivedData;
    if(document.isNull()){
        qWarning() << "Response parse error: " << jsonParseError.errorString();
        return;
    }
    if(!document.isObject()){
        qWarning() << "Response is not JSON object";
        return;
    }

    auto message = MessageUtils::createMessageFromJson(document);

    auto messageType = message->getMessageType();
    qDebug() << "Message type: " << messageTypeToString(messageType);

    if(messageType == MessageType::Notification){
        auto notificationMessage = std::static_pointer_cast<NotificationMessage>(message);
        processNotification(notificationMessage);
        return;
    }
    else if(!inRequestProcessing){
        qDebug() << "No data to be expected";
        return;
    }

    if(currentRequest == nullptr){
        qCritical() << "Invalid pointer";
        return;
    }

    auto sentMessageType = currentRequest->getMessageType();
    qDebug() << "Sent message type: " << messageTypeToString(sentMessageType);
    switch (messageType){
        case MessageType::GetHistoryResponse:{
            if(sentMessageType != MessageType::GetHistory){
                qDebug() << "Invalid message type";
                break;
            }

            auto responseMessage = std::static_pointer_cast<GetHistoryResponseMessage>(message);
            emit chatHistoryReceived(responseMessage->getMessagesHistory());
            break;
        }
        case MessageType::SendMessageResponse:{
            if(sentMessageType != MessageType::SendMessage){
                qDebug() << "Invalid message type";
                break;
            }

            auto responseMessage = std::static_pointer_cast<SendMessageResponseMessage>(message);
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

    responseReceived = true;
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

void TcpClientWorker::finishRequest()
{
    requestTimer.stop();
    inRequestProcessing = false;
    currentRequest = nullptr;
    if(requestQueue.size() != 0){
        requestQueue.pop();
    }

    if(requestQueue.size() != 0){
        processTopRequest();
    }
}
