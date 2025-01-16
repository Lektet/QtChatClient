#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>

#include <QVBoxLayout>
#include <QListView>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>

#include "ChatMessageData.h"

#include <set>

class MessageItemDelegate;
class TcpClient;
class MessageModel;
class MessagesViewer;
class SettingsWidget;

enum class Settings;

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MainWidget(QWidget *parent = nullptr);
    ~MainWidget();

protected:
    virtual void closeEvent(QCloseEvent *event) override;

private:
    QAction* settingsAction;
    QVBoxLayout* widgetLayout;
    QListView* chatHistoryView;
    MessageItemDelegate* messageItemDelegate;
    MessagesViewer* messagesViewer;
    QLabel* messageErrorLabel;
    QTextEdit* messageField;
    QPushButton* sendButton;
    std::shared_ptr<SettingsWidget> settingsWidget;

    TcpClient* tcpClient;
    MessageModel* messageModel;

    QString username;
    bool disconnecting;

    virtual void paintEvent(QPaintEvent *event) override;

    void cleanChat();
    void setupLayout();

private slots:
    void onSendButtonPressed();
    void onChatMessageSentSuccess();
    void onChatHistoryReceived(const std::vector<ChatMessageData> chatHistory);
    void onTcpClientStopped();
    void onChatUpdated();

    void onSettingsSaved(const std::set<Settings>& changedSettings);
    void onSettingsWidgetCanceled();
};
#endif // MAINWINDOW_H
