#ifndef MESSAGESMODEL_H
#define MESSAGESMODEL_H

#include <QAbstractListModel>

#include "ChatMessageData.h"

#include <vector>

class MessageModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit MessageModel(QObject *parent = nullptr);

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void setMessages(const std::vector<ChatMessageData> messages);

    void wantsUpdate();


private:
    std::vector<ChatMessageData> messages;
};

#endif // MESSAGESMODEL_H
