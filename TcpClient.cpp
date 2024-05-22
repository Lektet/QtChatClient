#include "TcpClient.h"

#include <QHostAddress>

#include "GetChatHistoryRequest.h"
#include "SendChatMessage.h"
#include "TcpClientWorker.h"

const QHostAddress defaultHost = QHostAddress::LocalHost;
const quint16 defaultPort = 44000;

TcpClient::TcpClient(QObject *parent)
    : QObject{parent},
      workerThread(new QThread(this)),
      worker(new TcpClientWorker()),
      stopping(false)
{
    worker->moveToThread(workerThread);
    connect(worker, &TcpClientWorker::chatHistoryReceived,
            this, &TcpClient::chatHistoryReceived, Qt::QueuedConnection);
    connect(worker, &TcpClientWorker::chatMessageSent,
            this, &TcpClient::chatMessageSent, Qt::QueuedConnection);
    connect(worker, &TcpClientWorker::noConnectionToServer,
            this, &TcpClient::onNoConnectionToServer, Qt::QueuedConnection);
    connect(worker, &TcpClientWorker::connectionToServerEstablished,
            this, &TcpClient::connectionToServerEstablished, Qt::QueuedConnection);
    connect(worker, &TcpClientWorker::chatHasBeenUpdated,
            this, &TcpClient::chatHasBeenUpdated, Qt::QueuedConnection);

    connect(workerThread, &QThread::finished, this, [this](){
        qDebug() << "workerThread finished";
        worker->deleteLater();
        emit processingFinished();
    });
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

void TcpClient::addSendChatMessageRequest(const QJsonObject &message) const
{    
    QMetaObject::invokeMethod(worker,
                              "addSendChatMessageRequest",
                              Qt::QueuedConnection,
                              Q_ARG(QJsonObject, message));
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
    stopping = true;
    if(worker->isDisconnected()){
        workerThread->quit();
        return;
    }

    stopWorker();
}

void TcpClient::onNoConnectionToServer()
{
    if(stopping){
        workerThread->quit();
    }

    emit noConnectionToServer();
}

void TcpClient::stopWorker() const
{
    QMetaObject::invokeMethod(worker,
                              &TcpClientWorker::stop,
                              Qt::QueuedConnection);
}
