#include "TcpClientWorker.h"

#include <QHostAddress>
#include <QJsonDocument>

#include "MessageType.h"
#include "MessageUtils.h"
#include "NewSessionRequestMessage.h"
#include "NewSessionResponseMessage.h"
#include "NewSessionConfirmMessage.h"
#include "GetChatMessagesMessage.h"
#include "GetChatMessagesResponseMessage.h"
#include "AddMessageMessage.h"
#include "NotificationMessage.h"
#include "AddUserMessage.h"
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
      workerSocket(nullptr),
      inRequestProcessing(false),
      connected(false)
{

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

void TcpClientWorker::requestChatMessages(const QUuid &sessionId)
{
    Request request(std::make_shared<GetChatMessagesMessage>(sessionId));
    requestQueue.push(std::move(request));
    continueRequestProcessing();
}

void TcpClientWorker::requestAddChatMessage(const QUuid &sessionId, const NewChatMessageData& message)
{
    Request request(std::make_shared<AddChatMessageMessage>(sessionId, message));
    requestQueue.push(std::move(request));
    continueRequestProcessing();
}

void TcpClientWorker::requestAddUser(const QUuid &sessionId, const QString &username, const QString &password, const UserRole role)
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

void TcpClientWorker::requestNewSession(const QUuid &userId, const QString &username, const QString& password)
{
    Request request(std::make_shared<NewSessionRequestMessage>(userId, username, password));
    requestQueue.push(std::move(request));
    continueRequestProcessing();
}

void TcpClientWorker::requestConfirmSession(const QUuid &userId, const QUuid &sessionId)
{
    Request request(std::make_shared<NewSessionConfirmMessage>(userId, sessionId));
    requestQueue.push(std::move(request));
    continueRequestProcessing();
}

void TcpClientWorker::onReadyRead()
{
    auto receivedData = TcpDataTransmitter::receiveData(*workerSocket.get());

    for(auto& data : receivedData){
        processMessageData(data);
    }

    continueRequestProcessing();
}

void TcpClientWorker::processTopRequest()//TODO: Process top request through event loop
{
    auto request = requestQueue.front();
    qDebug() << "Type of message to send: " << messageTypeToString(request.message->getMessageType());
    if(!TcpDataTransmitter::sendData(request.message->toJson().toJson(), *workerSocket.get())){
        qWarning() << "Chat request failed";
        return;
    }

    finishRequest();
}

void TcpClientWorker::processNotification(const NotificationMessage &notitification)
{
    if(notitification.getNotificationType() == NotificationType::MessagesUpdated){
        emit chatHasBeenUpdated();
    }
}

void TcpClientWorker::processMessageData(const QByteArray &data)
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

    switch (messageType){
        case MessageType::NewSessionResponse:{
            bool success = false;
            auto responseMessage = MessageUtils::createMessageFromJson<NewSessionResponseMessage>(document, &success);
            if(!success){
                //TODO: Signal about error
                qWarning() << "Error parsing received message";
                break;
            }

            // bool sessionInitiated = responseMessage.getErrorInfo().errorCode == ErrorCode::NoError;
            // if(sessionInitiated){
            //     emit newSessionInitiated(responseMessage.getUserId(),
            //                              responseMessage.getSessionId(),
            //                              responseMessage.getUserRole());
            // }
            // else{
            //     emit newSessionFailed(responseMessage.getUserId());
            // }
            emit newSessionRequestResultReceived(responseMessage.getUserId(),
                                                 responseMessage.getSessionId(),
                                                 responseMessage.getUserRole(),
                                                 responseMessage.getErrorInfo());
            break;
        }
        case MessageType::GetChatMessagesResponse:{
            bool success = false;
            auto responseMessage = MessageUtils::createMessageFromJson<GetChatMessagesResponseMessage>(document, &success);
            if(!success){
                qWarning() << "Error parsing received message";
                break;
            }

            emit chatMessagesReceived(responseMessage.getMessagesHistory(), responseMessage.getErrorInfo());
            break;
        }
        case MessageType::ResponseMessage:{
            bool success = false;
            auto responseMessage = MessageUtils::createMessageFromJson<ResponseMessage>(document, &success);
            if(!success){
                qWarning() << "Error parsing received message";
            }

            emit errorReceived(responseMessage.getErrorInfo());
            break;
        }
        case MessageType::AddMessageResponse:{
            bool success = false;
            auto responseMessage = MessageUtils::createMessageFromJson<ResponseMessage>(document, &success);
            if(!success){
                qWarning() << "Error parsing received message";
            }
            emit addChatMessageResultReceived(responseMessage.getErrorInfo());
            break;
        }
        case MessageType::AddUserResponse:{
            bool success = false;
            auto responseMessage = MessageUtils::createMessageFromJson<ResponseMessage>(document, &success);
            if(!success){
                qWarning() << "Error parsing received message";
            }
            emit addUserResultReceived(responseMessage.getErrorInfo());
            break;
        }
        case MessageType::BadRequestResponse:{
            bool success = false;
            auto responseMessage = MessageUtils::createMessageFromJson<ResponseMessage>(document, &success);
            if(!success){
                qWarning() << "Error parsing received message";
            }
            emit serverReceivedBadRequest(responseMessage.getErrorInfo());
            break;
        }
        case MessageType::Invalid:
            qWarning() << "Invalid message type";
            break;
        default:
            break;
    }
}

void TcpClientWorker::continueRequestProcessing()
{
    if(!requestQueue.empty()){
        processTopRequest();
    }
}

void TcpClientWorker::finishRequest()
{
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
