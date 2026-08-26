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
#include "UserManagmentWidget.h"

#include "NewChatMessageData.h"
#include "ErrorInfo.h"
#include "UserRole.h"

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
    Settings::Username,
    Settings::Password,
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
    stackedWidget(new QStackedWidget()),
    settingsAction(new QAction(QIcon("://resources/icons/settings.png"), "")),
    messagesAction(new QAction(tr("Messsages"))),
    userManagmentAction(new QAction(tr("User Managment"))),
    widgetLayout(new QVBoxLayout(this)),
    chatHistoryView(new QListView()),
    messageItemDelegate(new MessageItemDelegate(this)),
    messagesViewer(new MessagesViewer(this)),
    sendMessageWidget(new QWidget()),
    messageErrorLabel(new QLabel(tr("Message empty!"))),
    messageField(new QTextEdit()),
    sendButton(new QPushButton(tr("sendButton"))),
    userManagmentWidget(new UserManagmentWidget()),
    settingsWidget(std::make_shared<SettingsWidget>()),
    tcpClient(new TcpClient(this)),
    messageModel(new MessageModel(this)),
    disconnecting(false)
{
    setupLayout();

    qRegisterMetaType<UserRole>();

    connect(sendButton, &QPushButton::pressed, this, &MainWidget::onSendButtonPressed);
    connect(settingsAction, &QAction::triggered, this, [this](){
        setDisabled(true);
        settingsWidget->show();
    });
    connect(settingsWidget.get(), &SettingsWidget::settingsSaved,
            this, &MainWidget::onSettingsSaved);
    connect(settingsWidget.get(), &SettingsWidget::canceled,
            this, &MainWidget::onSettingsWidgetCanceled);

    connect(userManagmentWidget, &UserManagmentWidget::newUserSubmitted,
            this, &MainWidget::onNewUserSubmitted);

    connect(tcpClient, &TcpClient::newSessionInitiated,
            this, &MainWidget::onNewSessionInitiated);
    connect(tcpClient, &TcpClient::newSessionFailed,
            this, &MainWidget::onNewSessionFailed);
    connect(tcpClient, &TcpClient::chatMessageSentSuccess,
            this, &MainWidget::onChatMessageSentSuccess);
    connect(tcpClient, &TcpClient::chatHistoryReceived,
            this, &MainWidget::onChatHistoryReceived);
    connect(tcpClient, &TcpClient::startedSuccessfully,
            this, &MainWidget::onStartedSuccessfully);
    connect(tcpClient, &TcpClient::serverReceivedBadRequest,
            this, &MainWidget::onServerReceivedBadRequest);

    connect(tcpClient, &TcpClient::stopped, this, &MainWidget::onTcpClientStopped);
    connect(tcpClient, &TcpClient::stoppedOnConnectionError,
            this, &MainWidget::onStoppedOnConnectionError);

    connect(tcpClient, &TcpClient::chatHasBeenUpdated, this, &MainWidget::onChatUpdated);

    QSettings settings;
    username = settings.value("username").toString();
    password = settings.value("password").toString();
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
        event->ignore();
        tcpClient->stop();
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
    toolBar->addAction(messagesAction);
    toolBar->addAction(userManagmentAction);
    toolBar->addWidget(spacer);
    toolBar->addAction(settingsAction);
    widgetLayout->addWidget(toolBar);

    widgetLayout->addWidget(stackedWidget);


    auto messagesWidget = new QWidget();

    auto messagesWidgetContentLayout = new QVBoxLayout();

    messageErrorLabel->setStyleSheet(ERROR_LABEL_STYLE);
    messageErrorLabel->hide();

    messagesViewer->setDataFromModel(messageModel);
    messagesWidgetContentLayout->addWidget(messagesViewer);

    messagesWidgetContentLayout->addSpacing(5);

    messagesWidgetContentLayout->setContentsMargins(11, 0, 11, 11);

    auto messageLabelsLayout = new QHBoxLayout();
    messageLabelsLayout->setContentsMargins(0, 0, 0, 0);
    auto messageLabel = new QLabel(tr("Message:"));
    messageLabelsLayout->addWidget(messageLabel);
    messageLabelsLayout->addWidget(messageErrorLabel);


    auto sendMessageWidgetLayout = new QVBoxLayout();
    sendMessageWidgetLayout->addLayout(messageLabelsLayout);
    sendMessageWidgetLayout->addWidget(messageField);
    sendMessageWidgetLayout->addWidget(sendButton);
    sendMessageWidgetLayout->setContentsMargins(0,0,0,0);
    sendMessageWidget->setLayout(sendMessageWidgetLayout);

    messagesWidgetContentLayout->addWidget(sendMessageWidget);

    messagesWidget->setLayout(messagesWidgetContentLayout);

    widgetIndexes[WidgetTypes::Messages] = stackedWidget->addWidget(messagesWidget);
    widgetIndexes[WidgetTypes::UserManagment] = stackedWidget->addWidget(userManagmentWidget);

    connect(messagesAction, &QAction::triggered, this, [this](){
        stackedWidget->setCurrentIndex(widgetIndexes[WidgetTypes::Messages]);
    });
    connect(userManagmentAction, &QAction::triggered, this, [this](){
        stackedWidget->setCurrentIndex(widgetIndexes[WidgetTypes::UserManagment]);
    });
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
    tcpClient->initSession(userId, username, password);
}

void MainWidget::onStoppedOnConnectionError(const QAbstractSocket::SocketError errorCode)
{
    if(errorCode == QAbstractSocket::SocketError::RemoteHostClosedError){
        QMessageBox::warning(this, tr("Connection error"), tr("Disconnected by server"));
    }
    setDisabled(true);
    settingsWidget->show();
}

void MainWidget::onNewSessionInitiated(const QUuid &receivedUserId, const QUuid &receivedSessionId, const UserRole userRole)
{
    if(receivedSessionId.isNull()){
        QMessageBox::warning(this, tr("Login error"), tr("Invalid login data received"));
        tcpClient->stop();//TODO: Implement login/logout logic
        return;
    }

    if(receivedUserId != userId){
        QMessageBox::warning(this, tr("Login error"), tr("Invalid login data received"));
        tcpClient->stop();//TODO: Implement login/logout logic
        return;
    }

    sessionId = receivedSessionId;

    tcpClient->confirmSession(userId, sessionId);
    tcpClient->addGetChatRequest(sessionId);

    userManagmentAction->setDisabled(userRole != UserRole::Admin);
    sendMessageWidget->setVisible(userRole != UserRole::Guest);
}

void MainWidget::onNewSessionFailed(const QUuid &receivedUserId)
{
    if(receivedUserId.isNull() || receivedUserId != userId){
        qWarning() << "Received invalid  user id!";
    }
    else{
        QMessageBox::warning(this, tr("Login failed"), tr("Invalid credentials"));
        tcpClient->stop();//TODO: Implement login/logout logic
    }
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
}

void MainWidget::onChatUpdated()
{
    tcpClient->addGetChatRequest(sessionId);
}

void MainWidget::onServerReceivedBadRequest(const ErrorInfo &errorInfo)
{
    qDebug() << "Server received bad request!";
    qDebug() <<"Error description: " << errorInfo.errorDescription;
}

void MainWidget::onSettingsSaved(const std::set<Settings> &changedSettings)
{
    QSettings settings;
    if(changedSettings.contains(Settings::Username)){
        username = settings.value("username").toString();
    }
    if(changedSettings.contains(Settings::Password)){
        password = settings.value("password").toString();
    }

    auto serverHost = settings.value("serverHost").toString();
    auto serverPort = settings.value("serverPort").toInt();

    if(!tcpClient->isConnected()){
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
    if(tcpClient->isConnected()){
        setDisabled(false);
    }
    else{
        close();
    }
}

void MainWidget::onNewUserSubmitted(const QString &username, const QString &password, const UserRole role)
{
    tcpClient->addUserRequest(sessionId, username, password, role);
}

void MainWidget::onAddUserResultReceived(bool success)
{
    if(success){
        QMessageBox::information(this, tr("User created"), tr("New user successfully created!"));
    }
    else{
        QMessageBox::warning(this, tr("User not created"), tr("Failed to create new user!"));
    }
}
