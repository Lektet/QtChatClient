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
struct ErrorInfo;

enum class UserRole;

class TcpClient : public QObject
{
    Q_OBJECT
public:
    explicit TcpClient(QObject *parent = nullptr);
    ~TcpClient();

    void requestChatMessages(const QUuid& sessionId) const;
    void addChatMessage(const QUuid& sessionId,
                                   const NewChatMessageData &message) const;

    void addUser(const QUuid &sessionId,
                        const QString &username,
                        const QString &password,
                        const UserRole role);

    void initSession(const QUuid& userId, const QString& username, const QString &password);
    void confirmSession(const QUuid& userId, const QUuid& sessionId);

    void start(const QString& host, const quint16 port);
    void stop();
    void restart(const QString& host, const quint16 port);

    bool isStarted() const;
    bool isConnected() const;

signals:
    void startedSuccessfully();
    void stopped();
    void stoppedOnConnectionError(QAbstractSocket::SocketError errorCode);
    void connectionErrorOccured(QAbstractSocket::SocketError errorCode);

    void chatMessagesReceived(const std::vector<ChatMessageData>& history);
    void addChatMessageResultReceived(bool success);
    void chatHasBeenUpdated();
    void addUserResultReceived(bool success);

    void newSessionInitiated(const QUuid& userId, const QUuid& sessionId, const UserRole userRole);
    void newSessionFailed(const QUuid& userId);

    void serverReceivedBadRequest(const ErrorInfo& errorInfo);

private:
    QThread* workerThread;
    TcpClientWorker* worker;

    bool started;
    bool connected;
    bool restarting;
    QString hostForRestart;
    quint16 portForRestart;

    void stopWorker();
    void stopWorkerOnConnectionError(QAbstractSocket::SocketError errorCode);

private slots:
    void onWorkerDisconnected();
};

#endif // TCPCLIENT_H
