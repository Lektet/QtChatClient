#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QObject>

#include <QTcpSocket>
#include <QThread>

#include "ChatMessageData.h"

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

    void start(const QString& host, const quint16 port);
    void stop();
    void restart(const QString& host, const quint16 port);

    bool isStarted() const;

signals:
    void startedSuccessfully();
    void stopped();

    void chatHistoryReceived(const std::vector<ChatMessageData>& history);
    void chatMessageSentSuccess();
    void chatHasBeenUpdated();

private:
    QThread* workerThread;
    TcpClientWorker* worker;

    bool started;
    bool restarting;
    QString hostForRestart;
    quint16 portForRestart;

private slots:
    void onWorkerStopped();
};

#endif // TCPCLIENT_H
