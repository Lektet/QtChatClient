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
    settingsWidget(std::make_unique<SettingsWidget>()),
    tcpClient(new TcpClient(this)),
    currentRequest(nullptr),
    messageModel(new MessageModel(this)),
    disconnecting(false)
{
    setupLayout();

    qRegisterMetaType<UserRole>();
    qRegisterMetaType<ErrorInfo>();
    qRegisterMetaType<std::vector<ChatMessageData>>();

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

    connect(tcpClient, &TcpClient::newSessionRequestResultReceived,
            this, &MainWidget::onNewSessionRequestResultReceived);
    connect(tcpClient, &TcpClient::errorReceived,
            this, &MainWidget::onErrorReceived);
    connect(tcpClient, &TcpClient::addChatMessageResultReceived,
            this, &MainWidget::onAddChatMessageResultReceived);
    connect(tcpClient, &TcpClient::chatMessagesReceived,
            this, &MainWidget::onGetChatMessagesReceived);
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
    currentRequest = nullptr;

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
    auto request = std::make_unique<SendMessageRequest>(std::move(message));
    pushRequest(std::move(request));
}

void MainWidget::onAddChatMessageResultReceived(const ErrorInfo &errorInfo)
{
    if(currentRequest == nullptr || currentRequest->type != RequestType::SendMessage){
        qWarning() << "Unexpected call!";
        return;
    }

    if(errorInfo.errorCode != ErrorCode::NoError){
        warn(this, "Message sending error", "Failed to send message", errorInfo);
    }
    else{
        qInfo() << "Chat message sent successfully";
    }

    finishRequest();
}

void MainWidget::onStartedSuccessfully()
{
    userId = QUuid::createUuid();

    auto request = std::make_unique<NewSessionRequest>(userId, username, password);
    pushRequest(std::move(request));
}

void MainWidget::onStoppedOnConnectionError(const QAbstractSocket::SocketError errorCode)
{
    setDisabled(true);
    settingsWidget->show();

    if(errorCode == QAbstractSocket::SocketError::RemoteHostClosedError){
        QMessageBox::warning(settingsWidget.get(), tr("Connection error"), tr("Disconnected by server"));
    }
}

void MainWidget::onNewSessionRequestResultReceived(const QUuid &receivedUserId,
                                                   const QUuid &receivedSessionId,
                                                   const UserRole userRole,
                                                   const ErrorInfo &errorInfo)
{
    if(currentRequest == nullptr || currentRequest->type != RequestType::NewSession){
        qWarning() << "Unexpected call!";
        return;
    }

    if(errorInfo.errorCode != ErrorCode::NoError){
        if(receivedUserId.isNull() || receivedUserId != userId){
            qWarning() << tr("Received invalid  user id!");
        }
        else{
            const char* errorMsg;
            if(errorInfo.errorCode == ErrorCode::InvalidData){
                errorMsg = "Invalid credentials";
            }
            else{
                errorMsg = "Login error";
            }
            warn(settingsWidget.get(), "Login error", errorMsg, errorInfo);
        }

        currentRequest = nullptr;
        tcpClient->stop();//TODO: Implement login/logout logic
        return;
    }

    if(receivedSessionId.isNull()){
        QMessageBox::warning(settingsWidget.get(), tr("Login error"), tr("Invalid login data received"));
        tcpClient->stop();//TODO: Implement login/logout logic
        return;
    }

    if(receivedUserId != userId){
        QMessageBox::warning(settingsWidget.get(), tr("Login error"), tr("Invalid login data received"));
        tcpClient->stop();//TODO: Implement login/logout logic
        return;
    }

    sessionId = receivedSessionId;

    auto chatRequest = std::make_unique<Request>(RequestType::ChatHistory);
    pushRequest(std::move(chatRequest));

    userManagmentAction->setDisabled(userRole != UserRole::Admin);
    sendMessageWidget->setVisible(userRole != UserRole::Guest);

    finishRequest();
}

void MainWidget::onErrorReceived(const ErrorInfo &errorInfo)
{
    if(currentRequest == nullptr){
        qWarning() << "Unexpected message received!";
    }

    if(currentRequest->type == RequestType::NewSession ){
        QMessageBox::warning(settingsWidget.get(), tr("Error"), errorInfo.errorDescription);
        tcpClient->stop();
    }
    else{
        QMessageBox::warning(this, tr("Error"), errorInfo.errorDescription);
        finishRequest();
    }
}

void MainWidget::onGetChatMessagesReceived(const std::vector<ChatMessageData> chatHistory, const ErrorInfo &errorInfo)
{
    if(currentRequest == nullptr || currentRequest->type != RequestType::ChatHistory){
        qWarning() << "Unexpected call!";
        return;
    }

    if(errorInfo.errorCode != ErrorCode::NoError){
        warn(this,
             "Get chat messages error",
             "Failed to get chat messages!",
             errorInfo);
        finishRequest();
    }

    messageModel->setMessages(std::move(chatHistory));
    messagesViewer->setDataFromModel(messageModel);
    messagesViewer->verticalScrollBar()->setValue(messagesViewer->verticalScrollBar()->maximum());

    finishRequest();
}

void MainWidget::onTcpClientStopped()
{
    if(disconnecting){
        close();
        return;
    }

    std::queue<std::unique_ptr<Request>> empty;
    std::swap(requestQueue, empty);
    settingsWidget->show();
}

void MainWidget::onChatUpdated()
{
    auto request = std::make_unique<Request>(RequestType::ChatHistory);
    pushRequest(std::move(request));
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
    auto request = std::make_unique<AddUserRequest>(username, password, role);
    pushRequest(std::move(request));
}

void MainWidget::onAddUserResultReceived(const ErrorInfo &errorInfo)
{
    if(currentRequest == nullptr || currentRequest->type != RequestType::AddUser){
        qWarning() << "Unexpected call!";
        return;
    }

    if(errorInfo.errorCode == ErrorCode::NoError){
        QMessageBox::information(this, tr("User created"), tr("New user successfully created!"));
    }
    else{
        const char* errorMsg = "User not created!";
        qWarning() << errorMsg << " Error: " << errorInfo;
        QMessageBox::warning(this,
                             tr("Create user error"),
                             tr(errorMsg));
    }

    finishRequest();
}

void MainWidget::finishRequest()
{
    currentRequest = nullptr;
    processTopRequest();
}

void MainWidget::pushRequest(std::unique_ptr<Request> request)
{
    requestQueue.push(std::move(request));
    if(currentRequest == nullptr){
        processTopRequest();
    }
}

void MainWidget::processTopRequest()
{
    if(requestQueue.empty()){
        return;
    }

    currentRequest = std::move(requestQueue.front());
    requestQueue.pop();

    switch (currentRequest->type) {
    case RequestType::NewSession:{
        auto request = static_cast<NewSessionRequest*>(currentRequest.get());
        tcpClient->initSession(userId, request->username, request->password);
        break;
    }
    case RequestType::ConfirmSession:
        tcpClient->confirmSession(userId, sessionId);
        break;
    case RequestType::ChatHistory:
        tcpClient->requestChatMessages(sessionId);
        break;
    case RequestType::SendMessage:{
        auto request = static_cast<SendMessageRequest*>(currentRequest.get());
        tcpClient->addChatMessage(sessionId, request->messageData);
        break;
    }
    case RequestType::AddUser:{
        auto request = static_cast<AddUserRequest*>(currentRequest.get());
        tcpClient->addUser(sessionId, request->username, request->password, request->role);
        break;
    }
    case RequestType::DeleteUser:
    default:
        break;
    }

    if(!currentRequest->responseRequired ){
        currentRequest = nullptr;
        if(!requestQueue.empty()){
            processTopRequest();
        }
    }
}

void MainWidget::warn(QWidget *parent, const char *title, const char *msg, const ErrorInfo &errorInfo)
{
    qWarning() << msg << " Error: " << errorInfo;
    QMessageBox::warning(parent,
                         tr(title),
                         tr(msg));
}
