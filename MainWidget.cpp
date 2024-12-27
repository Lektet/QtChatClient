#include "MainWidget.h"

#include <QJsonObject>
#include <QJsonArray>

#include <QHBoxLayout>
#include <QMessageBox>
#include <QScrollBar>

#include <QCloseEvent>

#include "TcpClient.h"
#include "MessageModel.h"
#include "MessageItemDelegate.h"
#include "MessagesViewer.h"

#include "NewChatMessageData.h"

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
      messageModel(new MessageModel(this)),
      disconnecting(false)
{
    usernameErrorLabel->setStyleSheet(ERROR_LABEL_STYLE);
    usernameErrorLabel->hide();
    messageErrorLabel->setStyleSheet(ERROR_LABEL_STYLE);
    messageErrorLabel->hide();

    messagesViewer->setDataFromModel(messageModel);
    widgetLayout->addWidget(messagesViewer);

    widgetLayout->addSpacing(5);

    auto usernameLabelsLayout = new QHBoxLayout();
    // usernameLabelsLayout->setMargin(0);
    auto usernameLabel = new QLabel(tr("Username:"));
    usernameLabelsLayout->addWidget(usernameLabel);
    usernameLabelsLayout->addWidget(usernameErrorLabel);

    widgetLayout->addLayout(usernameLabelsLayout);
    widgetLayout->setAlignment(usernameLabelsLayout, Qt::AlignLeft);

    widgetLayout->addWidget(usernameField);
    widgetLayout->addSpacing(5);

    auto messageLabelsLayout = new QHBoxLayout();
    // messageLabelsLayout->setMargin(0);
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
    connect(tcpClient, &TcpClient::startedSuccessfully, this, [this](){
        tcpClient->addGetChatRequest();
    });
    connect(tcpClient, &TcpClient::stopped, this, &MainWidget::onTcpClientStopped);
    tcpClient->start();
    connect(tcpClient, &TcpClient::chatHasBeenUpdated, this, &MainWidget::onChatUpdated);
}

MainWidget::~MainWidget()
{
}

void MainWidget::closeEvent(QCloseEvent *event)
{
    if(disconnecting || !tcpClient->isActive()){
        event->accept();
    }
    else{
        disconnecting = true;
        tcpClient->stop();
        event->ignore();
    }
}

void MainWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    //Only here on first launch scrollbar will be "visible"
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


    NewChatMessageData message(usernameField->text(), messageField->toPlainText());
    tcpClient->addSendChatMessageRequest(message);
}

void MainWidget::onChatMessageSentSuccess()
{
    qDebug() << "Chat message sent successfully";
}

void MainWidget::onChatHistoryReceived(const std::vector<ChatMessageData> chatHistory)
{
    messageModel->setMessages(std::move(chatHistory));
    messagesViewer->setDataFromModel(messageModel);
    messagesViewer->verticalScrollBar()->setValue(messagesViewer->verticalScrollBar()->maximum());
}

void MainWidget::onTcpClientStopped()
{
    if(disconnecting){
        close();
        return;
    }

    QMessageBox msgBox;
    msgBox.setText(tr("No connection to server.\nTry to reconnect?"));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Yes);

    auto ret = msgBox.exec();

    switch (ret) {
        case QMessageBox::Yes :
            tcpClient->start();
            break;
        case QMessageBox::Cancel:
            close();
            break;
        default:
            break;
    }
}

void MainWidget::onChatUpdated()
{
    tcpClient->addGetChatRequest();
}

