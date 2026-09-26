#ifndef MESSAGESMODEL_H
#define MESSAGESMODEL_H

#include <QAbstractListModel>

#include "ChatMessageData.h"

#include <deque>

class MessagesModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit MessagesModel(QObject *parent = nullptr);

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void setMessages(const std::vector<ChatMessageData> messagesToSet);
    void addMessages(std::vector<ChatMessageData> messagesToAdd);
    void removeMessages(const int from, int count);

    quint64 findIdIndex(const QString& element);

private:
    using MessagesDeque = std::deque<ChatMessageData>;
    MessagesDeque messages;
    std::unordered_map<QString, MessagesDeque::iterator> messageIteratorById;

    quint64 minId;
    quint64 maxId;
};

#endif // MESSAGESMODEL_H
