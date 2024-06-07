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

class TcpClientWorker : public QObject
{
    Q_OBJECT

public:
    explicit TcpClientWorker(QObject *parent = nullptr);

    void start();
    void stop();

    bool isDisconnected();

public slots:
    void addGetChatRequest();
    void addSendChatMessageRequest(QJsonObject message);

signals:

    void chatHistoryReceived(const QJsonArray& history);
    void chatMessageSentSuccess();
    void noConnectionToServer();
    void connectionToServerEstablished();
    void connectionFailed();
    void chatHasBeenUpdated();

private:;
    std::queue<std::shared_ptr<SimpleMessage>> requestQueue;
    std::shared_ptr<SimpleMessage> currentRequest;

    QTcpSocket workerSocket;
    QTimer requestTimer;

    QTcpSocket::SocketState lastSocketState;
    std::mutex socketMutex;
    bool inRequestProcessing;

    void onReadyRead();
    void onSocketStateChanged(QAbstractSocket::SocketState);
    void processTopRequest();
    void processNotification(std::shared_ptr<NotificationMessage> notitification);
    void processMessageData(const QByteArray& data, bool& responseReceived);

   bool isInRequestProcessing() const;
   void continueRequestProcessing();
   void finishRequest();
};

#endif // TCPCLIENTWORKER_H
