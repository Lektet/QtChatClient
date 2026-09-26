#include "MessageModel.h"

#include <QJsonObject>

#include "MessageDataRole.h"

#include <ranges>

#include <QDebug>

const QString MESSAGE_USERNAME_KEY = "Username";
const QString MESSAGE_TEXT_KEY = "Text";
const QString MESSAGE_ID_KEY = "Id";
const QString MESSAGE_POST_TIME_KEY = "Time";

MessagesModel::MessagesModel(QObject *parent) :
    QAbstractListModel{parent},
    minId(0),
    maxId(0)
{

}

int MessagesModel::rowCount(const QModelIndex &parent) const
{
    return messages.size();
}

QVariant MessagesModel::data(const QModelIndex &index, int role) const
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

//TODO: Make only required changes in messagesToSet
void MessagesModel::setMessages(const std::vector<ChatMessageData> messagesToSet)
{
    beginResetModel();
    for(auto& message: messagesToSet){
        auto it = messages.insert(messages.cend(), message);
        messageIteratorById[message.id] = it;
    }

    endResetModel();
}

void MessagesModel::addMessages(std::vector<ChatMessageData> messagesToAdd)
{
    if(messagesToAdd.empty()){
        return;
    }

    if(messages.empty()){
        beginInsertRows(QModelIndex(), 0, messagesToAdd.size());
        messages.insert(messages.begin(), messagesToAdd.begin(), messagesToAdd.end());
        endInsertRows();
        return;
    }

    auto firstMessageToAddId = messagesToAdd.front().id.toULongLong();
    if(firstMessageToAddId > maxId){
        maxId = messagesToAdd.back().id.toULongLong();
        beginInsertRows(QModelIndex(), rowCount(), rowCount() + messagesToAdd.size());
        for(const auto& message :messagesToAdd){
            messages.push_back(std::move(message));
        }
        endInsertRows();
        return;
    }

    auto lastMessageToAddId = messagesToAdd.back().id.toULongLong();
    if(lastMessageToAddId < minId){
        minId = firstMessageToAddId;
        beginInsertRows(QModelIndex(), 0, messagesToAdd.size());
        for(const auto& message : std::ranges::views::reverse(messagesToAdd)){
            messages.push_front(std::move(message));
        }
        endInsertRows();
        return;
    }

    int i = 0;
    for(auto it = messages.begin(); it <= messages.end(); ++it, ++i){
        auto msgId = it->id.toULongLong();
        if(msgId > firstMessageToAddId){
            if(msgId <= lastMessageToAddId){
                qWarning() << "Messages to insert have unsuitable id!";
                return;
            }

            beginInsertRows(QModelIndex(), i, messagesToAdd.size());
            messages.insert(it, messagesToAdd.begin(), messagesToAdd.end());
            endInsertRows();
            break;
        }
    }
}

void MessagesModel::removeMessages(const int from, int count)
{
    if(from < 0 || from >= rowCount()){
        qWarning() << "Invalid from argument value";
        return;
    }
    if(count < 0){
        qWarning() << "Invalid count argument value";
        return;
    }
    if((from + count) > rowCount()){
        qWarning() << "Invalid arguments";
        return;
    }

    beginRemoveRows(QModelIndex(), from, from + count - 1);

    if(from == 0){
        for(int i = 0; i < count; ++i){
            messages.pop_back();
        }
        minId = messages.back().id.toULongLong();
    }
    else if(rowCount() == (from + count)){
        for(int i = 0; i < count; ++i){
            messages.pop_front();
        }
        maxId = messages.front().id.toULongLong();
    }
    else{
        messages.erase(messages.begin() + from, messages.begin() + from + count - 1);
    }

    endRemoveRows();
}

quint64 MessagesModel::findIdIndex(const QString &element)
{
    auto it = std::find_if(messages.begin(), messages.end(), [&element](const ChatMessageData& data){
        return data.id == element;
    });
    return std::distance(messages.begin(), it);
}
