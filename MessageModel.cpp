#include "MessageModel.h"

#include <QJsonObject>

#include "MessageDataRole.h"

#include <QDebug>

const QString MESSAGE_USERNAME_KEY = "Username";
const QString MESSAGE_TEXT_KEY = "Text";
const QString MESSAGE_ID_KEY = "Id";
const QString MESSAGE_TIME_KEY = "Time";

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
        qDebug() << "Invalid model index";
        return QVariant();
    }

    switch (role) {
        case MessageDataRole::Id:{
            auto jsonObject = messages.at(index.row()).toObject();
            return jsonObject.value(MESSAGE_ID_KEY).toVariant();
            break;
    }
        case MessageDataRole::Text:{
            auto jsonObject = messages.at(index.row()).toObject();
            return jsonObject.value(MESSAGE_TEXT_KEY).toVariant();
            break;
    }
        case MessageDataRole::Time:{
            auto jsonObject = messages.at(index.row()).toObject();
            bool convertIsOk = false;
            auto msecs = jsonObject.value(MESSAGE_TIME_KEY).toVariant().toLongLong(&convertIsOk);
            if(!convertIsOk){
                qDebug() << "Error converting jdonObject value to quint64";
                return QVariant();
            }
            return QDateTime::fromMSecsSinceEpoch(msecs);
            break;
    }
        case MessageDataRole::Username:{
            auto jsonObject = messages.at(index.row()).toObject();
            return jsonObject.value(MESSAGE_USERNAME_KEY).toVariant();
            break;
}
        default:
            return QVariant();
    }
}

//TODO: Make only required changes in messages
void MessageModel::setMessages(const QJsonArray &messages)
{
    beginResetModel();
    this->messages = messages;
    endResetModel();
}

void MessageModel::wantsUpdate()
{
    emit layoutChanged();
}
