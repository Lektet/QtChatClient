#include "MessageModel.h"

#include <QJsonObject>

#include "MessageDataRole.h"

#include <QDebug>

const QString MESSAGE_USERNAME_KEY = "Username";
const QString MESSAGE_TEXT_KEY = "Text";
const QString MESSAGE_ID_KEY = "Id";
const QString MESSAGE_POST_TIME_KEY = "Time";

MessageModel::MessageModel(QObject *parent) :
    QAbstractListModel{parent}
{

}

int MessageModel::rowCount(const QModelIndex &parent) const
{
    return messages.size();
}

QVariant MessageModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid()){
        qWarning() << "Invalid model index";
        return QVariant();
    }

    if(index.row() >= messages.size()){
        qWarning() << "Incorrect model index";
    }

    switch (role) {
        case MessageDataRole::Id:{
            return messages.at(index.row()).id;
            break;
        }
        case MessageDataRole::Username:{
            return messages.at(index.row()).username;
            break;
        }
        case MessageDataRole::Text:{
            return messages.at(index.row()).text;
            break;
        }
        case MessageDataRole::Time:{
            bool convertIsOk = false;
            auto msecs = messages.at(index.row()).postTime.toLongLong(&convertIsOk);
            if(!convertIsOk){
                qDebug() << "Error converting QString value to quint64";
                return QVariant();
            }
            return QDateTime::fromMSecsSinceEpoch(msecs);
            break;
        }
        default:
            return QVariant();
    }
}

//TODO: Make only required changes in messages
void MessageModel::setMessages(const std::vector<ChatMessageData> messages)
{
    beginResetModel();
    this->messages = messages;
    endResetModel();
}

void MessageModel::wantsUpdate()
{
    emit layoutChanged();
}
