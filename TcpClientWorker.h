#ifndef TCPCLIENTWORKER_H
#define TCPCLIENTWORKER_H

#include <QObject>
#include <QJsonObject>
#include <QTcpSocket>
#include <QTimer>

#include <memory>
#include <queue>
#include <mutex>

class SimpleMessage;
class NotificationMessage;

struct ChatMessageData;
struct NewChatMessageData;

struct ErrorInfo;

enum class UserRole;

class TcpClientWorker : public QObject
{
    Q_OBJECT

    struct Request{
        explicit Request(std::shared_ptr<SimpleMessage> requestMessage = nullptr,
                         bool waitForResponseToRequest = true) :
            message(requestMessage),
            waitForResponse(waitForResponseToRequest)
        {

        }

        bool isValid() const{
            return message != nullptr;
        }

        std::shared_ptr<SimpleMessage> message;
        bool waitForResponse;
    };

public:
    explicit TcpClientWorker(QObject *parent = nullptr);

public slots:
    void init();
    void connectToServer(const QString &host, const quint16 port);
    void disconnect();

    void requestNewSessionRequest(const QUuid& userId, const QString& username, const QString& password);
    void confirmSessionRequest(const QUuid& userId, const QUuid& sessionId);

    void addGetChatRequest(const QUuid& sessionId);
    void addSendChatMessageRequest(const QUuid& sessionId, const NewChatMessageData& message);

    void addUserRequest(const QUuid &sessionId, const QString &username, const QString &password, const UserRole role);

signals:
    void connectedSucessfully();
    void connectionErrorOccured(QAbstractSocket::SocketError errorCode);

    void newSessionInitiated(const QUuid& userId, const QUuid& sessionId, const UserRole userRole);
    void newSessionFailed(const QUuid& userId);

    void chatHistoryReceived(const std::vector<ChatMessageData> history);
    void chatMessageSentSuccess();
    void chatHasBeenUpdated();
    void addUserResultReceived(bool success);
    void serverReceivedBadRequest(const ErrorInfo& errorInfo);

    void disconnected();

private:
    std::queue<Request> requestQueue;
    Request lastSentRequest;

    std::unique_ptr<QTcpSocket> workerSocket;
    QTimer requestTimer;

    std::mutex socketStateMutex;
    bool inRequestProcessing;

    bool connected;

    void onReadyRead();
    void processTopRequest();
    void processNotification(const NotificationMessage& notitification);
    void processMessageData(const QByteArray& data, bool& responseReceived);

   bool isInRequestProcessing() const;
   void continueRequestProcessing();
   void finishRequest();

private slots:
   void onConnected();
   void onDisconnected();
   void onSocketErrorOccured(QAbstractSocket::SocketError socketError);
};

#endif // TCPCLIENTWORKER_H
