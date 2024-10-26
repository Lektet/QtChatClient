#include "TcpClient.h"

#include <QHostAddress>

#include "TcpClientWorker.h"

#include "NewChatMessageData.h"

const QHostAddress defaultHost = QHostAddress::LocalHost;
const quint16 defaultPort = 44000;

TcpClient::TcpClient(QObject *parent)
    : QObject{parent},
      workerThread(new QThread(this)),
      worker(new TcpClientWorker())
{
    worker->moveToThread(workerThread);
    connect(worker, &TcpClientWorker::chatHistoryReceived,
            this, &TcpClient::chatHistoryReceived, Qt::QueuedConnection);
    connect(worker, &TcpClientWorker::chatMessageSentSuccess,
            this, &TcpClient::chatMessageSentSuccess, Qt::QueuedConnection);
    connect(worker, &TcpClientWorker::connectedToServer,
            this, &TcpClient::connectionToServerEstablished, Qt::QueuedConnection);
    connect(worker, &TcpClientWorker::disconnectedFromServer,
            this, &TcpClient::disconnectedFromServer, Qt::QueuedConnection);
    connect(worker, &TcpClientWorker::connectionErrorOccured,
            this, &TcpClient::connectionErrorOccured, Qt::QueuedConnection);
    connect(worker, &TcpClientWorker::chatHasBeenUpdated,
            this, &TcpClient::chatHasBeenUpdated, Qt::QueuedConnection);

    connect(workerThread, &QThread::finished, this, [this](){
        qDebug() << "workerThread finished";
        worker->deleteLater();
        emit processingFinished();
    });

    qRegisterMetaType<NewChatMessageData>();
}

TcpClient::~TcpClient()
{

}

void TcpClient::addGetChatRequest() const
{
    QMetaObject::invokeMethod(worker,
                              &TcpClientWorker::addGetChatRequest,
                              Qt::QueuedConnection);
}

void TcpClient::addSendChatMessageRequest(const NewChatMessageData &message) const
{    
    QMetaObject::invokeMethod(worker,
                              "addSendChatMessageRequest",
                              Qt::QueuedConnection,
                              Q_ARG(NewChatMessageData, message));
}

void TcpClient::startRequestProcessing()
{
    workerThread->start();
    QMetaObject::invokeMethod(worker,
                              &TcpClientWorker::start,
                              Qt::QueuedConnection);
}

void TcpClient::quitRequestProcessing()
{
    if(!workerThread->isRunning()){
        qWarning() << "TcpClient was not started";
        return;
    }

    QMetaObject::invokeMethod(worker,
                              &TcpClientWorker::stop,
                              Qt::QueuedConnection);
    workerThread->quit();
    workerThread->wait();
}
