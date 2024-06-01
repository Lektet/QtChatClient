#include "MainWidget.h"

#include <QJsonObject>
#include <QJsonArray>

#include <QHBoxLayout>
#include <QMessageBox>
#include <QScrollBar>

#include "TcpClient.h"
#include "MessageModel.h"
#include "MessageItemDelegate.h"
#include "MessagesViewer.h"

#include <QDebug>

const QString MESSAGE_USERNAME_KEY = "Username";
const QString MESSAGE_TEXT_KEY = "Text";
const QString ERROR_LABEL_STYLE = "QLabel{"
                                  "color: red;"
                                  "}";
const QString CHAT_HISTORY_VIEW_STYLE = "QListView::item:selected{"
                                        "selection-background-color: rgb(128,128,255);"
                                        "}";

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent),
      widgetLayout(new QVBoxLayout(this)),
      chatHistoryView(new QListView()),
      messageItemDelegate(new MessageItemDelegate(this)),
      messagesViewer(new MessagesViewer(this)),
      usernameErrorLabel(new QLabel(tr("Username empty!"))),
      messageErrorLabel(new QLabel(tr("Message empty!"))),
      usernameField(new QLineEdit()),
      messageField(new QTextEdit()),
      sendButton(new QPushButton(tr("sendButton"))),
      tcpClient(new TcpClient(this)),
      messageModel(new MessageModel(this))
{
    usernameErrorLabel->setStyleSheet(ERROR_LABEL_STYLE);
    usernameErrorLabel->hide();
    messageErrorLabel->setStyleSheet(ERROR_LABEL_STYLE);
    messageErrorLabel->hide();

//    chatHistoryView->setModel(messageModel);
//    messageItemDelegate->setWidth(chatHistoryView->width());
//    chatHistoryView->setItemDelegate(messageItemDelegate);
//    chatHistoryView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
//    chatHistoryView->setStyleSheet(CHAT_HISTORY_VIEW_STYLE);
//    widgetLayout->addWidget(chatHistoryView);
    messagesViewer->setDataFromModel(messageModel);
    widgetLayout->addWidget(messagesViewer);

    widgetLayout->addSpacing(5);

    auto usernameLabelsLayout = new QHBoxLayout();
    usernameLabelsLayout->setMargin(0);
    auto usernameLabel = new QLabel(tr("Username:"));
    usernameLabelsLayout->addWidget(usernameLabel);
    usernameLabelsLayout->addWidget(usernameErrorLabel);

    widgetLayout->addLayout(usernameLabelsLayout);
    widgetLayout->setAlignment(usernameLabelsLayout, Qt::AlignLeft);

    widgetLayout->addWidget(usernameField);
    widgetLayout->addSpacing(5);

    auto messageLabelsLayout = new QHBoxLayout();
    messageLabelsLayout->setMargin(0);
    auto messageLabel = new QLabel(tr("Message:"));
    messageLabelsLayout->addWidget(messageLabel);
    messageLabelsLayout->addWidget(messageErrorLabel);

    widgetLayout->addLayout(messageLabelsLayout);
    widgetLayout->setAlignment(messageLabelsLayout, Qt::AlignLeft);

    widgetLayout->addWidget(messageField);




    widgetLayout->addWidget(sendButton);

    connect(sendButton, &QPushButton::pressed, this, &MainWidget::onSendButtonPressed);

    connect(tcpClient, &TcpClient::chatMessageSentSuccess, this, &MainWidget::onChatMessageSentSuccess);
    connect(tcpClient, &TcpClient::chatHistoryReceived, this, &MainWidget::onChatHistoryReceived);
    connect(tcpClient, &TcpClient::connectionToServerEstablished, this, [this](){
        tcpClient->addGetChatRequest();
    });
    connect(tcpClient, &TcpClient::noConnectionToServer, this, &MainWidget::onNoConnectionToServer);
    tcpClient->startRequestProcessing();
    connect(tcpClient, &TcpClient::chatHasBeenUpdated, this, &MainWidget::onChatUpdated);
    connect(tcpClient, &TcpClient::processingFinished, this, [this](){
        close();
    });
}

MainWidget::~MainWidget()
{
}

void MainWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    //Только здесь, при первом запуске, скроллбар будет visible
    auto delegateWidth = chatHistoryView->width();
    if(chatHistoryView->verticalScrollBar()->isVisible()){
        delegateWidth -= chatHistoryView->verticalScrollBar()->width();
    }
    if(delegateWidth != messageItemDelegate->getWidth()){
        messageItemDelegate->setWidth(delegateWidth);
        messageModel->wantsUpdate();
    }
}

void MainWidget::onSendButtonPressed()
{
    bool requiredFieldIsEmpty = false;
    if(usernameField->text().isEmpty()){
        usernameErrorLabel->show();
        requiredFieldIsEmpty = true;
    }
    else{
        usernameErrorLabel->hide();
    }
    if(messageField->toPlainText().isEmpty()){
        messageErrorLabel->show();
        requiredFieldIsEmpty = true;
    }
    else{
        messageErrorLabel->hide();
    }

    if(requiredFieldIsEmpty){
        return;
    }


    QJsonObject message;
    message.insert(MESSAGE_USERNAME_KEY, usernameField->text());
    message.insert(MESSAGE_TEXT_KEY, messageField->toPlainText());

    tcpClient->addSendChatMessageRequest(message);
}

void MainWidget::onChatMessageSentSuccess()
{
    qDebug() << "Chat message sent";
    tcpClient->addGetChatRequest();
}

void MainWidget::onChatHistoryReceived(const QJsonArray &chatHistory)
{
//    qDebug() << "Chat history: " << chatHistory;
    messageModel->setMessages(chatHistory);
    messagesViewer->setDataFromModel(messageModel);
    messagesViewer->verticalScrollBar()->setValue(messagesViewer->verticalScrollBar()->maximum());
}

void MainWidget::onNoConnectionToServer()
{
    QMessageBox msgBox;
    msgBox.setText(tr("No connection to server.\nTry to reconnect?"));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Yes);

    auto ret = msgBox.exec();

    switch (ret) {
        case QMessageBox::Yes :
            tcpClient->startRequestProcessing();
            break;
        case QMessageBox::Cancel:
            tcpClient->quitRequestProcessing();
            break;
        default:
            break;
    }
}

void MainWidget::onChatUpdated()
{
    tcpClient->addGetChatRequest();
}

