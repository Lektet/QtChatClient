#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QObject>

#include <QTcpSocket>
#include <QThread>

#include <queue>
#include <thread>
#include <mutex>
#include <future>
#include <condition_variable>

class ChatRequest;
class TcpClientWorker;

class TcpClient : public QObject
{
    Q_OBJECT
public:
    explicit TcpClient(QObject *parent = nullptr);
    ~TcpClient();

    void addGetChatRequest();
    void addSendChatMessageRequest(const QJsonObject& message);

    void startRequestProcessing();

signals:
    void chatHistoryReceived(const QJsonArray& history);
    void chatMessageSent();
    void noConnectionToServer();
    void connectionToServerEstablished();
    void chatHasBeenUpdated();

private:
    QThread workerThread;
    TcpClientWorker* worker;
};

#endif // TCPCLIENT_H
