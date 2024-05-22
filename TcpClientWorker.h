#ifndef TCPCLIENTWORKER_H
#define TCPCLIENTWORKER_H

#include <QObject>
#include <QJsonObject>
#include <QTcpSocket>

#include <memory>
#include <queue>

class ChatRequest;

class TcpClientWorker : public QObject
{
    Q_OBJECT

public:
    explicit TcpClientWorker(QObject *parent = nullptr);

    void start();

public slots:
    void addGetChatRequest();
    void addSendChatMessageRequest(QJsonObject message);

signals:

    void chatHistoryReceived(const QJsonArray& history);
    void chatMessageSent();
    void noConnectionToServer();
    void connectionToServerEstablished();
    void chatHasBeenUpdated();

private:
    std::queue<std::shared_ptr<ChatRequest>> requestQueue;//TODO: Make ChatRequest const
    std::shared_ptr<ChatRequest> currentRequest;

    QTcpSocket workerSocket;

    void onReadyRead();
    void onSocketStateChanged(QAbstractSocket::SocketState);
    void processTopRequest();
    void processNotification(const QJsonObject&);

   bool isInRequestProcessing() const;
   void continueRequestProcessing();
   void requestFinished();
};

#endif // TCPCLIENTWORKER_H
