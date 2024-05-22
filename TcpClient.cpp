#include "TcpClient.h"

#include <QHostAddress>

#include "GetChatHistoryRequest.h"
#include "SendChatMessage.h"
#include "TcpClientWorker.h"

const QHostAddress defaultHost = QHostAddress::LocalHost;
const quint16 defaultPort = 44000;

TcpClient::TcpClient(QObject *parent)
    : QObject{parent},
      worker(new TcpClientWorker())
{
    worker->moveToThread(&workerThread);
    connect(worker, &TcpClientWorker::chatHistoryReceived,
            this, &TcpClient::chatHistoryReceived, Qt::QueuedConnection);
    connect(worker, &TcpClientWorker::chatMessageSent,
            this, &TcpClient::chatMessageSent, Qt::QueuedConnection);
    connect(worker, &TcpClientWorker::noConnectionToServer,
            this, &TcpClient::noConnectionToServer, Qt::QueuedConnection);
    connect(worker, &TcpClientWorker::connectionToServerEstablished,
            this, &TcpClient::connectionToServerEstablished, Qt::QueuedConnection);
    connect(worker, &TcpClientWorker::chatHasBeenUpdated,
            this, &TcpClient::chatHasBeenUpdated, Qt::QueuedConnection);
}

TcpClient::~TcpClient()
{

}

void TcpClient::addGetChatRequest()
{
    QMetaObject::invokeMethod(worker,
                              &TcpClientWorker::addGetChatRequest,
                              Qt::QueuedConnection);
}

void TcpClient::addSendChatMessageRequest(const QJsonObject &message)
{    
    QMetaObject::invokeMethod(worker,
                              "addSendChatMessageRequest",
                              Qt::QueuedConnection,
                              Q_ARG(QJsonObject, message));
}

void TcpClient::startRequestProcessing()
{
    workerThread.start();
    QMetaObject::invokeMethod(worker,
                              &TcpClientWorker::start,
                              Qt::QueuedConnection);
}
