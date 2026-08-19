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

#include <set>

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

    QStackedWidget* stackedWidget;
    QAction* settingsAction;
    QAction* messagesAction;
    QAction* userManagmentAction;
    QVBoxLayout* widgetLayout;

    QListView* chatHistoryView;
    MessageItemDelegate* messageItemDelegate;
    MessagesViewer* messagesViewer;
    QLabel* messageErrorLabel;
    QTextEdit* messageField;
    QPushButton* sendButton;

    UserManagmentWidget* userManagmentWidget;

    std::shared_ptr<SettingsWidget> settingsWidget;

    std::map<WidgetTypes, int> widgetIndexes;

    TcpClient* tcpClient;
    MessageModel* messageModel;

    QString username;
    QString password;

    QUuid userId;
    QUuid sessionId;

    bool disconnecting;

    virtual void paintEvent(QPaintEvent *event) override;

    void cleanChat();
    void setupLayout();

private slots:
    void onSendButtonPressed();
    void onChatMessageSentSuccess();

    void onStartedSuccessfully();
    void onStoppedOnConnectionError(const QAbstractSocket::SocketError errorCode);
    void onTcpClientStopped();

    void onNewSessionInitiated(const QUuid& receivedUserId, const QUuid& receivedSessionId, const UserRole userRole);
    void onNewSessionFailed(const QUuid &receivedUserId);
    void onChatHistoryReceived(const std::vector<ChatMessageData> chatHistory);
    void onChatUpdated();
    void onServerReceivedBadRequest(const ErrorInfo& errorInfo);

    void onSettingsSaved(const std::set<Settings>& changedSettings);
    void onSettingsWidgetCanceled();

    void onNewUserSubmitted(const QString& username, const QString& password, const UserRole role);
    void onAddUserResultReceived(bool success);
};

#endif // MAINWINDOW_H
