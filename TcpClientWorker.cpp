#include "TcpClientWorker.h"

#include <QHostAddress>
#include <QJsonDocument>

#include "MessageType.h"
#include "MessageUtils.h"
#include "NewSessionRequestMessage.h"
#include "NewSessionResponseMessage.h"
#include "NewSessionConfirmMessage.h"
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
const int DISCONNECT_TIMEOUT = 5000;

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

void TcpClientWorker::init()
{
    workerSocket = std::make_unique<QTcpSocket>();
    connect(workerSocket.get(), &QTcpSocket::readyRead, this, &TcpClientWorker::onReadyRead);
    connect(workerSocket.get(), &QTcpSocket::connected, this, &TcpClientWorker::onConnected);
    connect(workerSocket.get(), &QTcpSocket::disconnected, this, &TcpClientWorker::onDisconnected);
    connect(workerSocket.get(), &QTcpSocket::errorOccurred, this, &TcpClientWorker::onSocketErrorOccured);
    connect(workerSocket.get(), &QTcpSocket::stateChanged,
            this, [this](QAbstractSocket::SocketState socketState){
                qDebug() << "Worker socket state: " << socketState;
            });
}

void TcpClientWorker::addGetChatRequest(const QUuid &sessionId)
{
    Request request(std::make_shared<GetHistoryMessage>(sessionId));
    requestQueue.push(std::move(request));
    continueRequestProcessing();
}

void TcpClientWorker::addSendChatMessageRequest(const QUuid &sessionId, const NewChatMessageData& message)
{
    Request request(std::make_shared<AddMessageMessage>(sessionId, message));
    requestQueue.push(std::move(request));
    continueRequestProcessing();
}

void TcpClientWorker::start(const QString &host, const quint16 port)
{
    Q_ASSERT(workerSocket != nullptr);
    if(workerSocket->state() != QTcpSocket::UnconnectedState){
        qWarning() << "Worker is already started";
        return;
    }

    workerSocket->connectToHost(host, port);
}

void TcpClientWorker::stop()
{
    Q_ASSERT(workerSocket != nullptr);
    if(workerSocket->state() == QTcpSocket::UnconnectedState){
        qDebug() << "Worker was not started";
        return;
    }
    workerSocket->disconnectFromHost();
    if(workerSocket->state() == QAbstractSocket::UnconnectedState ||
        workerSocket->waitForDisconnected(DISCONNECT_TIMEOUT)){
        qDebug() << "Socket disconnected!";
    }
    else{
        qWarning() << "Socket disconnection error";
        onSocketErrorOccured(workerSocket->error());
    }
}

void TcpClientWorker::requestNewSessionRequest(const QUuid &userId, const QString &username)
{
    Request request(std::make_shared<NewSessionRequestMessage>(userId, username));
    requestQueue.push(std::move(request));
    continueRequestProcessing();
}

void TcpClientWorker::confirmSessionRequest(const QUuid &userId, const QUuid &sessionId)
{
    Request request(std::make_shared<NewSessionConfirmMessage>(userId, sessionId), false);
    requestQueue.push(std::move(request));
    continueRequestProcessing();
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
    if(currentRequest.isValid()){
        qWarning() << "Request is already in process!";
        return;
    }

    inRequestProcessing = true;
    currentRequest = requestQueue.front();
    qDebug() << "Type of message to send: " << messageTypeToString(currentRequest.message->getMessageType());
    if(!TcpDataTransmitter::sendData(currentRequest.message->toJson().toJson(), *workerSocket.get())){
        qWarning() << "Chat request failed";
        return;
    }

    if(currentRequest.waitForResponse){
        requestTimer.start();
    }
    else{
        finishRequest();
    }
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
        auto notificationMessage = std::dynamic_pointer_cast<NotificationMessage>(message);
        processNotification(notificationMessage);
        return;
    }
    else if(!inRequestProcessing){
        qWarning() << "No data to be expected";
        return;
    }

    if(!currentRequest.isValid()){
        qCritical() << "Current request is invalid";
        return;
    }

    auto currentRequestMessageType = currentRequest.message->getMessageType();
    qDebug() << "Last sent message type: " << messageTypeToString(currentRequestMessageType);
    switch (messageType){
        case MessageType::NewSessionResponse:{
            if(currentRequestMessageType != MessageType::NewSessionRequest){
                qWarning() << "Invalid message type";
                break;
            }
            auto responseMessage = std::dynamic_pointer_cast<NewSessionResponseMessage>(message);

            emit newSessionInitiated(responseMessage->getUsernameIsValid(),
                                     responseMessage->getUserId(),
                                     responseMessage->getSessionId());
            break;
        }
        case MessageType::GetHistoryResponse:{
            if(currentRequestMessageType != MessageType::GetHistory){
                qWarning() << "Invalid message type";
                break;
            }

            auto responseMessage = std::dynamic_pointer_cast<GetHistoryResponseMessage>(message);

            emit chatHistoryReceived(responseMessage->getMessagesHistory());
            break;
        }
        case MessageType::AddMessageResponse:{
            if(currentRequestMessageType != MessageType::AddMessage){
                qWarning() << "Invalid message type";
                break;
            }

            auto responseMessage = std::dynamic_pointer_cast<AddMessageResponseMessage>(message);
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
    return currentRequest.isValid();
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
    currentRequest = Request();
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
    qDebug() << "TcpClientWorker::onDisconnected()";
    connected = false;
    emit stopped();
}

void TcpClientWorker::onSocketErrorOccured(QAbstractSocket::SocketError socketError)
{
    qWarning() << "Socket error: " << socketError;
    qWarning() << "Socket error description: " << workerSocket->errorString();
    if(!connected) emit stopped();
}
