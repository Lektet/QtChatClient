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

    void addGetChatRequest() const;
    void addSendChatMessageRequest(const QJsonObject& message) const;

    void startRequestProcessing();
    void quitRequestProcessing();

signals:
    void chatHistoryReceived(const QJsonArray& history);
    void chatMessageSentSuccess();
    void noConnectionToServer();
    void connectionToServerEstablished();
    void connectionFailed();
    void chatHasBeenUpdated();

    void processingFinished();

private:
    QThread* workerThread;
    TcpClientWorker* worker;
    bool stopping;

    void onNoConnectionToServer();

    void stopWorker() const;
};

#endif // TCPCLIENT_H
