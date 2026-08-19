#include "TcpClientWorker.h"

#include <QHostAddress>
#include <QJsonDocument>

#include "MessageType.h"
#include "MessageUtils.h"
#include "NewSessionRequestMessage.h"
#include "NewSessionResponseMessage.h"
#include "NewSessionConfirmMessage.h"
#include "NewSessionFailedResponseMessage.h"
#include "GetHistoryMessage.h"
#include "GetHistoryResponseMessage.h"
#include "AddMessageMessage.h"
#include "AddMessageResponseMessage.h"
#include "BadRequestResponseMessage.h"
#include "NotificationMessage.h"
#include "AddUserMessage.h"
#include "AddUserResponseMessage.h"
// #include "Result.h"
#include "ErrorInfo.h"
#include "NotificationType.h"

#include "TcpDataTransmitter.h"

#include "ChatMessageData.h"

const QHostAddress defaultHost = QHostAddress::LocalHost;
const quint16 defaultPort = 44000;

const int REQUEST_TIMEOUT = 10000;
const int DISCONNECT_TIMEOUT = 5000;

TcpClientWorker::TcpClientWorker(QObject *parent)
    : QObject{parent},
      lastSentRequest(nullptr),
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

void TcpClientWorker::addUserRequest(const QUuid &sessionId, const QString &username, const QString &password, const UserRole role)
{
    Request request(std::make_shared<AddUserMessage>(sessionId, username, password, role));
    requestQueue.push(std::move(request));
    continueRequestProcessing();
}

void TcpClientWorker::connectToServer(const QString &host, const quint16 port)
{
    Q_ASSERT(workerSocket != nullptr);
    if(workerSocket->state() != QTcpSocket::UnconnectedState){
        qWarning() << "Worker is already started";
        return;
    }

    workerSocket->connectToHost(host, port);
}

void TcpClientWorker::disconnect()
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

void TcpClientWorker::requestNewSessionRequest(const QUuid &userId, const QString &username, const QString& password)
{
    Request request(std::make_shared<NewSessionRequestMessage>(userId, username, password));
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

    bool responseToLastRequestReceived = false;
    for(auto& data : receivedData){
        bool responseReceived = false;
        processMessageData(data, responseReceived);
        if(responseToLastRequestReceived == responseReceived == true){
            qWarning() << "Inapropriate message received";
        }
        else if(responseReceived){
            responseToLastRequestReceived = true;
        }
    }

    if(responseToLastRequestReceived){
        finishRequest();
    }
}

void TcpClientWorker::processTopRequest()//TODO: Process top request through event loop
{
    if(lastSentRequest.isValid()){
        qWarning() << "Request is already in process!";
        return;
    }

    inRequestProcessing = true;
    lastSentRequest = requestQueue.front();
    qDebug() << "Type of message to send: " << messageTypeToString(lastSentRequest.message->getMessageType());
    if(!TcpDataTransmitter::sendData(lastSentRequest.message->toJson().toJson(), *workerSocket.get())){
        qWarning() << "Chat request failed";
        return;
    }

    if(lastSentRequest.waitForResponse){
        requestTimer.start();
    }
    else{
        finishRequest();
    }
}

void TcpClientWorker::processNotification(const NotificationMessage &notitification)
{
    if(notitification.getNotificationType() == NotificationType::MessagesUpdated){
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

    auto messageType = MessageUtils::getMessageType(document);
    qDebug() << "Received message type: " << messageTypeToString(messageType);

    if(messageType == MessageType::Notification){
        bool success = false;
        auto notificationMessage = MessageUtils::createMessageFromJson<NotificationMessage>(document, &success);
        if(!success){
            return;
        }
        processNotification(notificationMessage);
        return;
    }
    else if(!inRequestProcessing){
        qWarning() << "No data to be expected";
        return;
    }

    if(!lastSentRequest.isValid()){
        qCritical() << "Current request is invalid";
        return;
    }

    auto currentRequestMessageType = lastSentRequest.message->getMessageType();
    qDebug() << "Last sent message type: " << messageTypeToString(currentRequestMessageType);
    switch (messageType){
        case MessageType::NewSessionResponse:{
            if(currentRequestMessageType != MessageType::NewSessionRequest){
                qWarning() << "Invalid message type";
                break;
            }
            bool success = false;
            auto responseMessage = MessageUtils::createMessageFromJson<NewSessionSuccessResponseMessage>(document, &success);
            if(!success){
                qWarning() << "Error parsing received message";
                break;
            }

            emit newSessionInitiated(responseMessage.getUserId(),
                                     responseMessage.getSessionId(),
                                     responseMessage.getUserRole());
            break;
        }
        case MessageType::NewSessionFailedResponse:{
            if(currentRequestMessageType != MessageType::NewSessionRequest){
                qWarning() << "Invalid message type";
                break;
            }
            bool success = false;
            auto responseMessage = MessageUtils::createMessageFromJson<NewSessionFailedResponseMessage>(document, &success);
            if(!success){
                qWarning() << "Error parsing received message";
                break;
            }

            emit newSessionFailed(responseMessage.getUserId());
            break;
        }
        case MessageType::GetHistoryResponse:{
            if(currentRequestMessageType != MessageType::GetHistory){
                qWarning() << "Invalid message type";
                break;
            }

            bool success = false;
            auto responseMessage = MessageUtils::createMessageFromJson<GetHistoryResponseMessage>(document, &success);
            if(!success){
                qWarning() << "Error parsing received message";
                break;
            }

            emit chatHistoryReceived(responseMessage.getMessagesHistory());
            break;
        }
        case MessageType::AddMessageResponse:{
            if(currentRequestMessageType != MessageType::AddMessage){
                qWarning() << "Invalid message type";
                break;
            }

            bool success = false;
            auto responseMessage = MessageUtils::createMessageFromJson<AddMessageResponseMessage>(document, &success);
            if(!success){
                qWarning() << "Error parsing received message";
                break;
            }

            if(!responseMessage.getResult()){
                qWarning() << "Message send failed";
            }
            else{
                emit chatMessageSentSuccess();
            }
            break;
        }
        case MessageType::AddUserResponse:{
            if(currentRequestMessageType != MessageType::AddUserResponse){
                qWarning() << "Invalid message type";
                break;
            }

            bool success = false;
            auto responseMessage = MessageUtils::createMessageFromJson<AddUserResponseMessage>(document, &success);
            if(!success){
                qWarning() << "Error parsing received message";
                break;
            }

            emit addUserResultReceived(responseMessage.getResult());
            break;
        }
        case MessageType::BadRequestResponse:{
            if(currentRequestMessageType != MessageType::GetHistory &&
                currentRequestMessageType != MessageType::AddMessage){
                qWarning() << "Invalid message type";
                break;
            }

            bool success = false;
            auto responseMessage = MessageUtils::createMessageFromJson<BadRequestResponseMessage>(document, &success);
            if(!success){
                qWarning() << "Error parsing received message";
                break;
            }

            emit serverReceivedBadRequest(responseMessage.getErrorInfo());
        }
        default:
            break;
    }

    responseReceived = true;
}

bool TcpClientWorker::isInRequestProcessing() const
{
    return lastSentRequest.isValid();
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
    lastSentRequest = Request();
    if(requestQueue.size() != 0){
        requestQueue.pop();
    }

    if(requestQueue.size() != 0){
        processTopRequest();
    }
}

void TcpClientWorker::onConnected()
{
    emit connectedSucessfully();
}

void TcpClientWorker::onDisconnected()
{
    qDebug() << "TcpClientWorker::onDisconnected()";
    connected = false;
    emit disconnected();
}

void TcpClientWorker::onSocketErrorOccured(QAbstractSocket::SocketError socketError)
{
    qDebug() << "TcpClientWorker::onSocketErrorOccured()";
    qWarning() << "Socket error: " << socketError;
    qWarning() << "Socket error description: " << workerSocket->errorString();

    emit connectionErrorOccured(socketError);
}
