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

void TcpClient::initSession(const QUuid &userId, const QString &username)
{
    if(!started){
        qWarning() << "Client was not started!";
        return;
    }

    QMetaObject::invokeMethod(worker,
                              "requestNewSessionRequest",
                              Qt::QueuedConnection,
                              Q_ARG(QUuid, userId),
                              Q_ARG(QString, username));
}

void TcpClient::start(const QString &host, const quint16 port)
{
    started = true;

    workerThread = new QThread(this);
    worker = new TcpClientWorker();

    worker->moveToThread(workerThread);
    connect(worker, &TcpClientWorker::newSessionInitiated,
            this, &TcpClient::newSessionInitiated, Qt::QueuedConnection);
    connect(worker, &TcpClientWorker::chatHistoryReceived,
            this, &TcpClient::chatHistoryReceived, Qt::QueuedConnection);
    connect(worker, &TcpClientWorker::chatMessageSentSuccess,
            this, &TcpClient::chatMessageSentSuccess, Qt::QueuedConnection);
    connect(worker, &TcpClientWorker::startedSucessfully,
            this, &TcpClient::startedSuccessfully, Qt::QueuedConnection);
    connect(worker, &TcpClientWorker::stopped,
            this, &TcpClient::onWorkerStopped, Qt::QueuedConnection);
    connect(worker, &TcpClientWorker::chatHasBeenUpdated,
            this, &TcpClient::chatHasBeenUpdated, Qt::QueuedConnection);

    workerThread->start();
    QMetaObject::invokeMethod(worker,
                              &TcpClientWorker::init,
                              Qt::QueuedConnection);
    QMetaObject::invokeMethod(worker,
                              "start",
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
                              &TcpClientWorker::stop,
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

void TcpClient::onWorkerStopped()
{
    qDebug() << "onWorkerStopped()";

    if(!started){
        qCritical() << "Client was not started";
        return;
    }
    started = false;

    workerThread->quit();
    workerThread->wait();

    worker->deleteLater();
    workerThread->deleteLater();

    if(!restarting){
        emit stopped();
    }
    else{
        restarting = false;
        start(hostForRestart, portForRestart);
    }
}
