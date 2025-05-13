#include "MainWidget.h"

#include <QJsonObject>
#include <QJsonArray>

#include <QHBoxLayout>
#include <QMessageBox>
#include <QScrollBar>
#include <QToolBar>
#include <QAction>
#include <QSettings>

#include <QCloseEvent>

#include "TcpClient.h"
#include "MessageModel.h"
#include "MessageItemDelegate.h"
#include "MessagesViewer.h"
#include "SettingsWidget.h"
#include "Settings.h"

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

const std::set<Settings> settingsRequiringReconnect = {
    Settings::Host,
    Settings::Port
};

bool reconnectRequiredForSettings(const std::set<Settings>& settings){
    for(auto& requiredSetting: settingsRequiringReconnect){
        for(auto& providedSetting: settings){
            if (requiredSetting == providedSetting){
                return true;
            }
        }
    }
    return false;
};

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent),
    settingsAction(new QAction(QIcon("://resources/icons/settings.png"), "")),
    widgetLayout(new QVBoxLayout(this)),
    chatHistoryView(new QListView()),
    messageItemDelegate(new MessageItemDelegate(this)),
    messagesViewer(new MessagesViewer(this)),
    messageErrorLabel(new QLabel(tr("Message empty!"))),
    messageField(new QTextEdit()),
    sendButton(new QPushButton(tr("sendButton"))),
    settingsWidget(std::make_shared<SettingsWidget>()),
    tcpClient(new TcpClient(this)),
    messageModel(new MessageModel(this)),
    disconnecting(false)
{
    setupLayout();

    connect(sendButton, &QPushButton::pressed, this, &MainWidget::onSendButtonPressed);
    connect(settingsAction, &QAction::triggered, this, [this](){
        setDisabled(true);
        settingsWidget->show();
    });
    connect(settingsWidget.get(), &SettingsWidget::settingsSaved,
            this, &MainWidget::onSettingsSaved);
    connect(settingsWidget.get(), &SettingsWidget::canceled,
            this, &MainWidget::onSettingsWidgetCanceled);

    connect(tcpClient, &TcpClient::newSessionInitiated,
            this, &MainWidget::onNewSessionInitiated);
    connect(tcpClient, &TcpClient::chatMessageSentSuccess,
            this, &MainWidget::onChatMessageSentSuccess);
    connect(tcpClient, &TcpClient::chatHistoryReceived,
            this, &MainWidget::onChatHistoryReceived);
    connect(tcpClient, &TcpClient::startedSuccessfully,
            this, &MainWidget::onStartedSuccessfully);

    connect(tcpClient, &TcpClient::stopped, this, &MainWidget::onTcpClientStopped);
    connect(tcpClient, &TcpClient::chatHasBeenUpdated, this, &MainWidget::onChatUpdated);

    QSettings settings;
    username = settings.value("username").toString();
    auto serverHost = settings.value("serverHost").toString();
    auto serverPort = settings.value("serverPort").toInt();
    tcpClient->start(serverHost, serverPort);
}

MainWidget::~MainWidget()
{
}

void MainWidget::closeEvent(QCloseEvent *event)
{
    if(disconnecting || !tcpClient->isStarted()){
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

void MainWidget::cleanChat()
{
    messageModel->setMessages(std::vector<ChatMessageData>());
    messagesViewer->setDataFromModel(messageModel);
}

void MainWidget::setupLayout()
{
    widgetLayout->setContentsMargins(0, 0, 0, 0);

    auto toolBar = new QToolBar(this);
    auto spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolBar->addWidget(spacer);
    toolBar->addAction(settingsAction);
    widgetLayout->addWidget(toolBar);

    auto widgetContentLayout = new QVBoxLayout();

    messageErrorLabel->setStyleSheet(ERROR_LABEL_STYLE);
    messageErrorLabel->hide();

    messagesViewer->setDataFromModel(messageModel);
    widgetContentLayout->addWidget(messagesViewer);

    widgetContentLayout->addSpacing(5);

    widgetContentLayout->setContentsMargins(11, 0, 11, 11);

    auto messageLabelsLayout = new QHBoxLayout();
    messageLabelsLayout->setContentsMargins(0, 0, 0, 0);
    auto messageLabel = new QLabel(tr("Message:"));
    messageLabelsLayout->addWidget(messageLabel);
    messageLabelsLayout->addWidget(messageErrorLabel);

    widgetContentLayout->addLayout(messageLabelsLayout);

    widgetContentLayout->addWidget(messageField);

    widgetContentLayout->addWidget(sendButton);

    widgetLayout->addLayout(widgetContentLayout);
}

void MainWidget::onSendButtonPressed()
{
    if(messageField->toPlainText().isEmpty()){
        messageErrorLabel->show();
        return;
    }

    NewChatMessageData message(username, messageField->toPlainText());
    tcpClient->addSendChatMessageRequest(sessionId, message);
}

void MainWidget::onChatMessageSentSuccess()
{
    qDebug() << "Chat message sent successfully";
}

void MainWidget::onStartedSuccessfully()
{
    userId = QUuid::createUuid();
    tcpClient->initSession(userId, username);
}

void MainWidget::onNewSessionInitiated(bool initSuccess, const QUuid &receivedUserId, const QUuid &receivedSessionId)
{
    if(!initSuccess){
        QMessageBox::warning(this, tr("Login error"), tr("Invalid username"));
        tcpClient->stop();//TODO: Implement login/logout logic
        return;
    }

    if(receivedUserId != userId){
        QMessageBox::warning(this, tr("Login error"), tr("Invalid user id"));
        tcpClient->stop();//TODO: Implement login/logout logic
        return;
    }

    sessionId = receivedSessionId;

    tcpClient->confirmSession(userId, sessionId);
    tcpClient->addGetChatRequest(sessionId);
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
    else{
        setDisabled(true);
        QMessageBox::warning(this, tr("Connection error"), tr("Failed to connect to server"));
        settingsWidget->show();
    }
}

void MainWidget::onChatUpdated()
{
    tcpClient->addGetChatRequest(sessionId);
}

void MainWidget::onSettingsSaved(const std::set<Settings> &changedSettings)
{
    QSettings settings;
    if(changedSettings.contains(Settings::Username)){
        username = settings.value("username").toString();
    }

    auto serverHost = settings.value("serverHost").toString();
    auto serverPort = settings.value("serverPort").toInt();

    if(!tcpClient->isStarted()){
        cleanChat();
        tcpClient->start(serverHost, serverPort);
    }
    else if(reconnectRequiredForSettings(changedSettings)){
        cleanChat();
        tcpClient->restart(serverHost, serverPort);
    }

    setDisabled(false);
}

void MainWidget::onSettingsWidgetCanceled()
{
    if(tcpClient->isStarted()){
        setDisabled(false);
    }
    else{
        close();
    }
}
