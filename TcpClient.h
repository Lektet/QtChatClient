#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QObject>

#include <QTcpSocket>
#include <QThread>

#include "ChatMessageData.h"

#include <queue>
#include <thread>
#include <mutex>
#include <future>
#include <condition_variable>
#include <vector>

class TcpClientWorker;

struct NewChatMessageData;

class TcpClient : public QObject
{
    Q_OBJECT
public:
    explicit TcpClient(QObject *parent = nullptr);
    ~TcpClient();

    void addGetChatRequest() const;
    void addSendChatMessageRequest(const NewChatMessageData &message) const;

    void start();
    void stop();

    bool isActive() const;

signals:
    void startedSuccessfully();
    void stopped();

    void chatHistoryReceived(const std::vector<ChatMessageData>& history);
    void chatMessageSentSuccess();
    void chatHasBeenUpdated();

private:
    QThread* workerThread;
    TcpClientWorker* worker;

    bool active;

private slots:
    void onWorkerStopped();
};

#endif // TCPCLIENT_H
