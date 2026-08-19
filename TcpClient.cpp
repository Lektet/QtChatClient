#include "TcpClient.h"

#include <QHostAddress>

#include "TcpClientWorker.h"

#include "NewChatMessageData.h"

const QHostAddress defaultHost = QHostAddress::LocalHost;
const quint16 defaultPort = 44000;

TcpClient::TcpClient(QObject *parent)
    : QObject{parent},
    workerThread(nullptr),
    worker(nullptr),
    started(false),
    connected(false),
    restarting(false)
{
    qRegisterMetaType<NewChatMessageData>();
}

TcpClient::~TcpClient()
{

}

void TcpClient::addGetChatRequest(const QUuid &sessionId) const
{
    if(!started){
        qCritical() << "Client is not started!";
        return;
    }

    QMetaObject::invokeMethod(worker,
                              "addGetChatRequest",
                              Qt::QueuedConnection,
                              Q_ARG(QUuid, sessionId));
}

void TcpClient::addSendChatMessageRequest(const QUuid &sessionId, const NewChatMessageData &message) const
{
    if(!started){
        qWarning() << "Client was not started!";
        return;
    }

    QMetaObject::invokeMethod(worker,
                              "addSendChatMessageRequest",
                              Qt::QueuedConnection,
                              Q_ARG(QUuid, sessionId),
                              Q_ARG(NewChatMessageData, message));
}

void TcpClient::addUserRequest(const QUuid &sessionId,
                               const QString &username,
                               const QString &password,
                               const UserRole role)
{
    if(!started){
        qWarning() << "Client was not started!";
        return;
    }

    QMetaObject::invokeMethod(worker,
                              "addUserRequest",
                              Qt::QueuedConnection,
                              Q_ARG(QUuid, sessionId),
                              Q_ARG(QString, username),
                              Q_ARG(QString, password),
                              Q_ARG(UserRole, role));
}

void TcpClient::confirmSession(const QUuid &userId, const QUuid &sessionId)
{
    if(!started){
        qWarning() << "Client was not started!";
        return;
    }

    QMetaObject::invokeMethod(worker,
                              "confirmSessionRequest",
                              Qt::QueuedConnection,
                              Q_ARG(QUuid, userId),
                              Q_ARG(QUuid, sessionId));
}

void TcpClient::initSession(const QUuid &userId, const QString &username, const QString &password)
{
    if(!started){
        qWarning() << "Client was not started!";
        return;
    }

    QMetaObject::invokeMethod(worker,
                              "requestNewSessionRequest",
                              Qt::QueuedConnection,
                              Q_ARG(QUuid, userId),
                              Q_ARG(QString, username),
                              Q_ARG(QString, password));
}

void TcpClient::start(const QString &host, const quint16 port)
{
    started = true;

    workerThread = new QThread(this);
    worker = new TcpClientWorker();

    worker->moveToThread(workerThread);
    connect(worker, &TcpClientWorker::newSessionInitiated,
            this, &TcpClient::newSessionInitiated, Qt::QueuedConnection);
    connect(worker, &TcpClientWorker::newSessionFailed,
            this, &TcpClient::newSessionFailed, Qt::QueuedConnection);
    connect(worker, &TcpClientWorker::chatHistoryReceived,
            this, &TcpClient::chatHistoryReceived, Qt::QueuedConnection);
    connect(worker, &TcpClientWorker::chatMessageSentSuccess,
            this, &TcpClient::chatMessageSentSuccess, Qt::QueuedConnection);
    connect(worker, &TcpClientWorker::connectedSucessfully,
            this, [this](){
                    connected = true;
                    emit startedSuccessfully();
            }, Qt::QueuedConnection);
    connect(worker, &TcpClientWorker::disconnected,
            this, &TcpClient::onWorkerDisconnected, Qt::QueuedConnection);
    connect(worker, &TcpClientWorker::chatHasBeenUpdated,
            this, &TcpClient::chatHasBeenUpdated, Qt::QueuedConnection);
    connect(worker, &TcpClientWorker::addUserResultReceived,
            this, &TcpClient::addUserResultReceived, Qt::QueuedConnection);
    connect(worker, &TcpClientWorker::serverReceivedBadRequest,
            this, &TcpClient::serverReceivedBadRequest, Qt::QueuedConnection);
    // connect(worker, &TcpClientWorker::connectionErrorOccured,
    //         this, &TcpClient::connectionErrorOccured, Qt::QueuedConnection);
    connect(worker, &TcpClientWorker::connectionErrorOccured,
            this, &TcpClient::stopWorkerOnConnectionError, Qt::QueuedConnection);

    workerThread->start();
    QMetaObject::invokeMethod(worker,
                              &TcpClientWorker::init,
                              Qt::QueuedConnection);
    QMetaObject::invokeMethod(worker,
                              "connectToServer",
                              Qt::QueuedConnection,
                              Q_ARG(QString, host),
                              Q_ARG(quint16, port));
}


void TcpClient::stop()
{
    if(!started){
        qWarning() << "TcpClient was not started";
        return;
    }

    QMetaObject::invokeMethod(worker,
                              &TcpClientWorker::disconnect,
                              Qt::QueuedConnection);

}

void TcpClient::restart(const QString &host, const quint16 port)
{
    restarting = true;
    hostForRestart = host;
    portForRestart= port;
    stop();
}

bool TcpClient::isStarted() const
{
    return started;
}

bool TcpClient::isConnected() const
{
    return connected;
}

void TcpClient::stopWorker()
{
    if(!started){
        qCritical() << "Client was not started";
        return;
    }
    started = false;

    workerThread->quit();
    workerThread->wait();

    //TODO: Remove thread and worker only on quit?
    worker->deleteLater();
    workerThread->deleteLater();

}

void TcpClient::stopWorkerOnConnectionError(QAbstractSocket::SocketError errorCode)
{
    stopWorker();

    emit stoppedOnConnectionError(errorCode);
}

void TcpClient::onWorkerDisconnected()
{
    qDebug() << "onWorkerStopped()";

    connected = false;

    stopWorker();

    if(!restarting){
        emit stopped();
    }
    else{
        restarting = false;
        start(hostForRestart, portForRestart);
    }
}
