#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>

#include <QAbstractSocket>

#include <QVBoxLayout>
#include <QStackedWidget>
#include <QListView>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QUuid>

#include "ChatMessageData.h"
#include "NewChatMessageData.h"

#include <set>
#include <queue>

class MessageItemDelegate;
class TcpClient;
class MessageModel;
class MessagesViewer;
class SettingsWidget;
class UserManagmentWidget;

enum class Settings;
enum class UserRole;

struct ErrorInfo;

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MainWidget(QWidget *parent = nullptr);
    ~MainWidget();

protected:
    virtual void closeEvent(QCloseEvent *event) override;

private:
    enum class WidgetTypes{
        Messages,
        UserManagment
    };

    enum class RequestType{
        NewSession,
        ConfirmSession,
        ChatHistory,
        SendMessage,
        AddUser,
        DeleteUser
    };

    struct Request{
        explicit Request(RequestType requestType,
                         bool responseToRequestAwaited = true):
            type(requestType),
            responseRequired(responseToRequestAwaited){

        };

        RequestType type;
        bool responseRequired;
    };

    struct NewSessionRequest: public Request{
        explicit NewSessionRequest(
            const QUuid& userId,
            QString newSessionUsernameme,
            QString newSessionPassword):
            Request(RequestType::NewSession),
            userId(userId),
            username(std::move(newSessionUsernameme)),
            password(std::move(newSessionPassword))
        {
        };

        QUuid userId;
        QString username;
        QString password;
    };

    struct ConfirmSessionRequest: public Request{
        explicit ConfirmSessionRequest(
            const QUuid& confirmUserId,
            const QUuid& confirmSessionId):
            Request(RequestType::ConfirmSession, false),
            userId(confirmUserId),
            sessionId(confirmSessionId)
        {
        };

        QUuid userId;
        QUuid sessionId;
    };

    struct SendMessageRequest: public Request{
        explicit SendMessageRequest(NewChatMessageData newMessageData):
            Request(RequestType::SendMessage),
            messageData(std::move(newMessageData))
        {
        };

        NewChatMessageData messageData;
    };

    struct AddUserRequest: public Request{
        explicit AddUserRequest(QString newUserUsernameme,
                                QString newUserPassword,
                                UserRole newUserRole):
            Request(RequestType::AddUser),
            username(std::move(newUserUsernameme)),
            password(std::move(newUserPassword)),
            role(newUserRole)
        {

        };

        QString username;
        QString password;
        UserRole role;
    };

    QStackedWidget* stackedWidget;
    QAction* settingsAction;
    QAction* messagesAction;
    QAction* userManagmentAction;
    QVBoxLayout* widgetLayout;

    QListView* chatHistoryView;
    MessageItemDelegate* messageItemDelegate;
    MessagesViewer* messagesViewer;
    QWidget* sendMessageWidget;
    QLabel* messageErrorLabel;
    QTextEdit* messageField;
    QPushButton* sendButton;

    UserManagmentWidget* userManagmentWidget;

    std::unique_ptr<SettingsWidget> settingsWidget;

    std::map<WidgetTypes, int> widgetIndexes;

    TcpClient* tcpClient;

    std::queue<std::unique_ptr<Request>> requestQueue;
    std::unique_ptr<Request> currentRequest;

    MessageModel* messageModel;

    QString username;
    QString password;

    QUuid userId;
    QUuid sessionId;

    bool disconnecting;

    void cleanChat();
    void setupLayout();

    void pushRequest(std::unique_ptr<Request> request);
    void processTopRequest();

    void warn(QWidget *parent, const char* title, const char* msg, const ErrorInfo& errorInfo);

private slots:
    void onSendButtonPressed();
    void onStartedSuccessfully();
    void onStoppedOnConnectionError(const QAbstractSocket::SocketError errorCode);
    void onTcpClientStopped();

    void onNewSessionRequestResultReceived(const QUuid& receivedUserId,
                                           const QUuid& receivedSessionId,
                                           const UserRole userRole,
                                           const ErrorInfo& errorInfo);

    void onErrorReceived(const ErrorInfo& errorInfo);

    void onGetChatMessagesReceived(const std::vector<ChatMessageData> chatHistory,
                                   const ErrorInfo& errorInfo);
    void onAddChatMessageResultReceived(const ErrorInfo &errorInfo);

    void onChatUpdated();
    void onServerReceivedBadRequest(const ErrorInfo& errorInfo);

    void onSettingsSaved(const std::set<Settings>& changedSettings);
    void onSettingsWidgetCanceled();

    void onMessagesViewerResized();

    void onNewUserSubmitted(const QString& username, const QString& password, const UserRole role);
    void onAddUserResultReceived(const ErrorInfo& errorInfo);
    void finishRequest();
};

#endif // MAINWINDOW_H
