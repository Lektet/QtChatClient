#include "MessagesViewer.h"

#include <MessageDataRole.h>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDateTime>
#include <QScrollBar>

#include "DependingWidthWidget.h"
#include "MessageLabel.h"

#include <QDebug>

const QString dateTimeFormat = "dd.MM.yyyy hh:mm:ss";

MessagesViewer::MessagesViewer(QWidget *parent)
    : QScrollArea{parent},
    model(nullptr),
    mainWidget(new QWidget(this)),
    mainWidgetLayout(new QVBoxLayout(mainWidget))
{
    setWidgetResizable(true);
    setWidget(mainWidget);

    mainWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

    connect(verticalScrollBar(), &QScrollBar::valueChanged,
            this, &MessagesViewer::onScrollbarValueChanged);
}

void MessagesViewer::setModel(const QAbstractListModel * modelToSet)
{
    if(model != nullptr){
        disconnect(model, &QAbstractListModel::modelReset, this, &MessagesViewer::fillFromModel);
        disconnect(model, &QAbstractListModel::rowsInserted, this, &MessagesViewer::onRowsInserted);
        disconnect(model, &QAbstractListModel::rowsRemoved, this, &MessagesViewer::onRowsRemoved);
        disconnect(model, &QAbstractListModel::dataChanged, this, &MessagesViewer::onDataChanged);
        disconnect(model, &QAbstractListModel::layoutChanged, this, &MessagesViewer::fillFromModel);
    }

    model = modelToSet;

    connect(model, &QAbstractListModel::modelReset, this, &MessagesViewer::fillFromModel);
    connect(model, &QAbstractListModel::rowsInserted, this, &MessagesViewer::onRowsInserted);
    connect(model, &QAbstractListModel::rowsRemoved, this, &MessagesViewer::onRowsRemoved);
    connect(model, &QAbstractListModel::dataChanged, this, &MessagesViewer::onDataChanged);
    connect(model, &QAbstractListModel::layoutChanged, this, &MessagesViewer::fillFromModel);

    fillFromModel();
}

std::vector<QString> MessagesViewer::viewedIds() const
{
    std::vector<QString> ids;
    for(const auto widget: viewedMessages){
        ids.push_back(widget->property("id").toString());
    }
    return ids;
}

std::vector<int> MessagesViewer::viewedIndexes() const
{
    std::vector<int> ids;
    for(const auto widget: viewedMessages){
        auto modelIndex = widget->property("modelIndex").toPersistentModelIndex();
        if(modelIndex.isValid()){
            ids.push_back(widget->property("modelIndex").toPersistentModelIndex().row());
        }
    }
    return ids;
}

void MessagesViewer::resizeEvent(QResizeEvent *event)
{
    QScrollArea::resizeEvent(event);

    emit resized();
}

void MessagesViewer::fillFromModel()
{
    for(auto& widget: messageWidgets){
        widget->deleteLater();
    }
    messageWidgets.clear();

    for (int i = 0; i < model->rowCount() ; ++i) {
        auto modelIndex = model->index(i, 0);        
        auto messageWidget = createMessageWidget(modelIndex);

        messageWidgets.push_back(messageWidget);
        mainWidgetLayout->addWidget(messageWidget);
    }
}

void MessagesViewer::onRowsInserted(const QModelIndex &parent, int first, int last)
{
    assert(first <= messageWidgets.size());

    auto itToInsert = std::next(messageWidgets.begin(), first);
    for(auto i = last; i >= first; --i){
        auto newMessageWidget = createMessageWidget(model->index(i,0));
        itToInsert = messageWidgets.insert(itToInsert, newMessageWidget);
        mainWidgetLayout->insertWidget(first, newMessageWidget);
    }
}

void MessagesViewer::onRowsRemoved(const QModelIndex &parent, int first, int last)
{
    assert(last <= messageWidgets.size());

    auto itToDelete = std::next(messageWidgets.begin(), first);
    for(auto i = 0; i < last - first; ++i){
        (*itToDelete)->deleteLater();
        itToDelete = messageWidgets.erase(itToDelete);
    }
}

void MessagesViewer::onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles)
{
    assert(topLeft.column() == 0 && bottomRight.column() == 0);
    assert(topLeft.isValid() && bottomRight.isValid());
    assert(bottomRight.row() <= messageWidgets.size());

    //TODO: Implement message widget
    // auto itToUpdate = std::next(messageWidgets.begin(), topLeft.row());
    // for(int i = 0; i <= bottomRight.row() - topLeft.row(); i++){
    //     auto widget = (*itToUpdate);this
    //     if(roles.contains(MessageDataRole::Id)){
    //         widget.
    //     }
    // }
}

void MessagesViewer::onScrollbarValueChanged(int value)
{
    std::vector<QWidget*> newViwedWidgets;
    auto messages =  mainWidget->findChildren<QWidget*>();
    bool viewedWidgetsChanged = false;
    auto oldViwedIt = viewedMessages.begin();
    for(const auto widget: messages){
        if(!widget->visibleRegion().isEmpty()){
            newViwedWidgets.push_back(widget);

            if((*oldViwedIt) != widget){
                viewedWidgetsChanged = true;
            }
            oldViwedIt++;
        }
    }

    if(viewedWidgetsChanged){
        viewedMessages = newViwedWidgets;
        emit viewedMessagesChanged();
    }
}

QWidget* MessagesViewer::createMessageWidget(const QModelIndex &modelIndex)
{
    auto messageWidget = new QWidget();
    messageWidget->setObjectName("messageWidget");
    messageWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    messageWidget->setStyleSheet("QWidget#messageWidget{"
                                 "background-color: #E0E0E0;"
                                 "border: 1px solid #AAAAAA;"
                                 "border-radius: 5px;"
                                 "}");
    auto messageLayout = new QVBoxLayout();
    messageWidget->setLayout(messageLayout);
    auto messageHeaderLayout = new QHBoxLayout();

    auto usernameLabel = new QLabel(modelIndex.data(MessageDataRole::Username).toString());
    auto messageDateTime = new QLabel(modelIndex.data(MessageDataRole::Time).toDateTime().toString(dateTimeFormat));
    messageHeaderLayout->addWidget(usernameLabel);
    messageHeaderLayout->addWidget(messageDateTime, 0, Qt::AlignRight);

    //        auto messageTextLabel = new MessageLabel(modelIndex.data(MessageDataRole::Text).toString());
    auto messageTextLabel = new QLabel(modelIndex.data(MessageDataRole::Text).toString());
    messageTextLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    messageTextLabel->setWordWrap(true);

    messageLayout->addLayout(messageHeaderLayout);
    messageLayout->addWidget(messageTextLabel);

    messageWidget->setProperty("id", modelIndex.data(MessageDataRole::Id));
    messageWidget->setProperty("modelIndex", QPersistentModelIndex(modelIndex));

    return messageWidget;
}
