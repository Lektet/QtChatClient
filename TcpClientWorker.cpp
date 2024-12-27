#include "TcpClientWorker.h"

#include <QHostAddress>
#include <QJsonDocument>

#include "MessageType.h"
#include "MessageUtils.h"
#include "GetHistoryMessage.h"
#include "GetHistoryResponseMessage.h"
#include "AddMessageMessage.h"
#include "AddMessageResponseMessage.h"
#include "NotificationMessage.h"
#include "Result.h"
#include "NotificationType.h"

#include "TcpDataTransmitter.h"

#include "ChatMessageData.h"

const QHostAddress defaultHost = QHostAddress::LocalHost;
const quint16 defaultPort = 44000;

const int REQUEST_TIMEOUT = 10000;

TcpClientWorker::TcpClientWorker(QObject *parent)
    : QObject{parent},
      currentRequest(nullptr),
      workerSocket(nullptr),
      inRequestProcessing(false),
      connected(false)
{
    requestTimer.setParent(this);
    requestTimer.setSingleShot(true);
    requestTimer.setInterval(REQUEST_TIMEOUT);
    connect(&requestTimer, &QTimer::timeout, this, &TcpClientWorker::finishRequest);
}

void TcpClientWorker::addGetChatRequest()
{
    requestQueue.push(std::make_shared<GetHistoryMessage>());
    continueRequestProcessing();
}

void TcpClientWorker::addSendChatMessageRequest(const NewChatMessageData& message)
{
    requestQueue.push(std::make_shared<AddMessageMessage>(message));
    continueRequestProcessing();
}

void TcpClientWorker::start()
{
    if(workerSocket != nullptr && workerSocket->state() != QTcpSocket::UnconnectedState){
        qWarning() << "Worker is already started";
        return;
    }
    workerSocket = std::make_unique<QTcpSocket>();
    connect(workerSocket.get(), &QTcpSocket::readyRead, this, &TcpClientWorker::onReadyRead);
    connect(workerSocket.get(), &QTcpSocket::connected, this, &TcpClientWorker::onConnected);
    connect(workerSocket.get(), &QTcpSocket::disconnected, this, &TcpClientWorker::onDisconnected);
    connect(workerSocket.get(), &QTcpSocket::errorOccurred, this, &TcpClientWorker::onSocketErrorOccured);
    workerSocket->connectToHost(defaultHost, defaultPort);
}

void TcpClientWorker::stop()
{
    if(workerSocket == nullptr || workerSocket->state() == QTcpSocket::UnconnectedState){
        qDebug() << "Worker was not started";
        return;
    }
    workerSocket->disconnectFromHost();
}

void TcpClientWorker::onReadyRead()
{
    auto receivedData = TcpDataTransmitter::receiveData(*workerSocket.get());

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

void TcpClientWorker::processTopRequest()//TODO: Process top request through event loop
{
    if(currentRequest != nullptr){
        qWarning() << "Request is already in process!";
        return;
    }

    inRequestProcessing = true;
    currentRequest = requestQueue.front();
    qDebug() << "Message to send: " << messageTypeToString(currentRequest->getMessageType());
    if(!TcpDataTransmitter::sendData(currentRequest->toJson().toJson(), *workerSocket.get())){
        qWarning() << "Chat request failed";
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
    qDebug() << "Received message type: " << messageTypeToString(messageType);

    if(messageType == MessageType::Notification){
        auto notificationMessage = std::static_pointer_cast<NotificationMessage>(message);
        processNotification(notificationMessage);
        return;
    }
    else if(!inRequestProcessing){
        qWarning() << "No data to be expected";
        return;
    }

    if(currentRequest == nullptr){
        qCritical() << "Invalid pointer";
        return;
    }

    auto currentRequestMessageType = currentRequest->getMessageType();
    qDebug() << "Last sent message type: " << messageTypeToString(currentRequestMessageType);
    switch (messageType){
        case MessageType::GetHistoryResponse:{
            if(currentRequestMessageType != MessageType::GetHistory){
                qWarning() << "Invalid message type";
                break;
            }

            auto responseMessage = std::static_pointer_cast<GetHistoryResponseMessage>(message);

            emit chatHistoryReceived(responseMessage->getMessagesHistory());
            break;
        }
        case MessageType::AddMessageResponse:{
            if(currentRequestMessageType != MessageType::AddMessage){
                qWarning() << "Invalid message type";
                break;
            }

            auto responseMessage = std::static_pointer_cast<AddMessageResponseMessage>(message);
            if(responseMessage->getResult() != Result::Success){
                qWarning() << "Message sent failed";
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

void TcpClientWorker::onConnected()
{
    connected = true;
    emit startedSucessfully();
}

void TcpClientWorker::onDisconnected()
{
    connected = false;
    emit stopped();
}

void TcpClientWorker::onSocketErrorOccured(QAbstractSocket::SocketError socketError)
{
    qWarning() << "Socket error: " << socketError;
    qWarning() << "Socket error description: " << workerSocket->errorString();
    if(!connected) emit stopped();
}
