#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QObject>

#include <QTcpSocket>
#include <QThread>
#include <QUuid>

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

    void addGetChatRequest(const QUuid& sessionId) const;
    void addSendChatMessageRequest(const QUuid& sessionId,
                                   const NewChatMessageData &message) const;

    void initSession(const QUuid& userId, const QString& username);
    void confirmSession(const QUuid& userId, const QUuid& sessionId);

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

    void newSessionInitiated(bool initSuccess, const QUuid& userId, const QUuid& sessionId);

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
