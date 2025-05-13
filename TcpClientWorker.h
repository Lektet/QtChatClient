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
    void start(const QString &host, const quint16 port);
    void stop();

    void requestNewSessionRequest(const QUuid& userId, const QString& username);
    void confirmSessionRequest(const QUuid& userId, const QUuid& sessionId);

    void addGetChatRequest(const QUuid& sessionId);
    void addSendChatMessageRequest(const QUuid& sessionId, const NewChatMessageData& message);

signals:    
    void startedSucessfully();

    void newSessionInitiated(bool initSuccess, const QUuid& userId, const QUuid& sessionId);

    void chatHistoryReceived(const std::vector<ChatMessageData> history);
    void chatMessageSentSuccess();
    void chatHasBeenUpdated();

    void stopped();

private:;
    std::queue<Request> requestQueue;
    Request currentRequest;

    std::unique_ptr<QTcpSocket> workerSocket;
    QTimer requestTimer;

    std::mutex socketStateMutex;
    bool inRequestProcessing;

    bool connected;

    void onReadyRead();
    void processTopRequest();
    void processNotification(std::shared_ptr<NotificationMessage> notitification);
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
