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
      active(false)
{
    qRegisterMetaType<NewChatMessageData>();
}

TcpClient::~TcpClient()
{

}

void TcpClient::addGetChatRequest() const
{
    if(!active){
        qCritical() << "Client is not started!";
        return;
    }

    QMetaObject::invokeMethod(worker,
                              &TcpClientWorker::addGetChatRequest,
                              Qt::QueuedConnection);
}

void TcpClient::addSendChatMessageRequest(const NewChatMessageData &message) const
{    
    if(!active){
        qWarning() << "Client was not started!";
        return;
    }

    QMetaObject::invokeMethod(worker,
                              "addSendChatMessageRequest",
                              Qt::QueuedConnection,
                              Q_ARG(NewChatMessageData, message));
}

void TcpClient::start()
{
    active = true;

    workerThread = new QThread(this);
    worker = new TcpClientWorker();

    worker->moveToThread(workerThread);
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
                              &TcpClientWorker::start,
                              Qt::QueuedConnection);
}

void TcpClient::stop()
{
    if(!active){
        qWarning() << "TcpClient was not started";
        return;
    }

    QMetaObject::invokeMethod(worker,
                              &TcpClientWorker::stop,
                              Qt::QueuedConnection);

}

bool TcpClient::isActive() const
{
    return active;
}

void TcpClient::onWorkerStopped()
{
    if(!active){
        qCritical() << "Client was not started";
        return;
    }
    active = false;

    workerThread->quit();
    workerThread->wait();

    worker->deleteLater();
    workerThread->deleteLater();

    emit stopped();
}
