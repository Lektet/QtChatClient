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

class MessageItemDelegate;
class TcpClient;
class MessageModel;
class MessagesViewer;

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    MainWidget(QWidget *parent = nullptr);
    ~MainWidget();

protected:
    virtual void closeEvent(QCloseEvent *event) override;

private:
    QVBoxLayout* widgetLayout;
    QListView* chatHistoryView;
    MessageItemDelegate* messageItemDelegate;
    MessagesViewer* messagesViewer;
    QLabel* usernameErrorLabel;
    QLabel* messageErrorLabel;
    QLineEdit* usernameField;
    QTextEdit* messageField;
    QPushButton* sendButton;

    TcpClient* tcpClient;
    MessageModel* messageModel;

    bool disconnecting;

    virtual void paintEvent(QPaintEvent *event) override;

private slots:
    void onSendButtonPressed();
    void onChatMessageSentSuccess();
    void onChatHistoryReceived(const std::vector<ChatMessageData> chatHistory);
    void onTcpClientStopped();
    void onChatUpdated();
};
#endif // MAINWINDOW_H
