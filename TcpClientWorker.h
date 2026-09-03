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

    void requestNewSession(const QUuid& userId, const QString& username, const QString& password);
    void requestConfirmSession(const QUuid& userId, const QUuid& sessionId);

    void requestChatMessages(const QUuid& sessionId);
    void requestAddChatMessage(const QUuid& sessionId, const NewChatMessageData& message);

    void requestAddUser(const QUuid &sessionId, const QString &username, const QString &password, const UserRole role);

signals:
    void connectedSucessfully();
    void connectionErrorOccured(QAbstractSocket::SocketError errorCode);

    void newSessionRequestResultReceived(const QUuid& userId,
                                         const QUuid& sessionId,
                                         const UserRole userRole,
                                         const ErrorInfo& errorInfo);

    void errorReceived(const ErrorInfo &errorInfo);

    void chatMessagesReceived(const std::vector<ChatMessageData> history, const ErrorInfo &errorInfo);
    void addChatMessageResultReceived(const ErrorInfo &errorInfo);
    void chatHasBeenUpdated();
    void addUserResultReceived(const ErrorInfo &errorInfo);
    void serverReceivedBadRequest(const ErrorInfo& errorInfo);

    void disconnected();

private:
    std::queue<Request> requestQueue;

    std::unique_ptr<QTcpSocket> workerSocket;

    std::mutex socketStateMutex;
    bool inRequestProcessing;

    bool connected;

    void onReadyRead();
    void processTopRequest();
    void processNotification(const NotificationMessage& notitification);
    void processMessageData(const QByteArray& data);

   void continueRequestProcessing();
   void finishRequest();

private slots:
   void onConnected();
   void onDisconnected();
   void onSocketErrorOccured(QAbstractSocket::SocketError socketError);
};

#endif // TCPCLIENTWORKER_H
