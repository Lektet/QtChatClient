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

public:
    explicit TcpClientWorker(QObject *parent = nullptr);

    void start();
    void stop();

public slots:
    void addGetChatRequest();
    void addSendChatMessageRequest(const NewChatMessageData& message);

signals:

    void chatHistoryReceived(const std::vector<ChatMessageData> history);
    void chatMessageSentSuccess();
    void connectedToServer();
    void disconnectedFromServer();
    void connectionErrorOccured();
    void chatHasBeenUpdated();

private:;
    std::queue<std::shared_ptr<SimpleMessage>> requestQueue;
    std::shared_ptr<SimpleMessage> currentRequest;

    std::unique_ptr<QTcpSocket> workerSocket;
    QTimer requestTimer;

    std::mutex socketStateMutex;
    bool inRequestProcessing;

    void onReadyRead();
    void processTopRequest();
    void processNotification(std::shared_ptr<NotificationMessage> notitification);
    void processMessageData(const QByteArray& data, bool& responseReceived);

   bool isInRequestProcessing() const;
   void continueRequestProcessing();
   void finishRequest();

private slots:
   void onSocketErrorOccured(QAbstractSocket::SocketError socketError);
};

#endif // TCPCLIENTWORKER_H
