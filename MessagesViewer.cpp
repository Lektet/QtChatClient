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
      mainWidget(nullptr)
{

}

void MessagesViewer::setDataFromModel(const QAbstractItemModel * const model)
{    
    auto oldMainwidget = takeWidget();
    if(oldMainwidget != nullptr){
        oldMainwidget->deleteLater();
    }
    verticalLabelsList.clear();

    mainWidget = new QWidget();
    mainWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    auto mainLayout = new QVBoxLayout(mainWidget);
    mainLayout->setSizeConstraint(QLayout::SetMinimumSize);

    for (int i = 0; i < model->rowCount() ; ++i) {
        auto modelIndex = model->index(i, 0);

//        auto messageWidget = new DependingWidthWidget();
        auto messageWidget = new QWidget();
        messageWidget->setObjectName("messageWidget");
        messageWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
        messageWidget->setStyleSheet("QWidget#messageWidget{"
                                     "background-color: #E0E0E0;"
                                     "border: 1px solid #AAAAAA;"
                                     "border-radius: 5px;"
                                     "}");
        auto messageLayout = new QVBoxLayout();
        messageWidget->setLayout(messageLayout);
        messageLayout->setSizeConstraint(QLayout::SetMinimumSize);
        auto messageHeaderLayout = new QHBoxLayout();

        auto usernameLabel = new QLabel(modelIndex.data(MessageDataRole::Username).toString());
        verticalLabelsList.push_back(usernameLabel);
        auto messageDateTime = new QLabel(modelIndex.data(MessageDataRole::Time).toDateTime().toString(dateTimeFormat));
        messageHeaderLayout->addWidget(usernameLabel);
        messageHeaderLayout->addWidget(messageDateTime, 0, Qt::AlignRight);

//        auto messageTextLabel = new MessageLabel(modelIndex.data(MessageDataRole::Text).toString());
        auto messageTextLabel = new QLabel(modelIndex.data(MessageDataRole::Text).toString());
        messageTextLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);
        messageTextLabel->setWordWrap(true);
        verticalLabelsList.push_back(messageTextLabel);

//        messageWidget->setWidthSourceWidget(messageTextLabel);

        messageLayout->addLayout(messageHeaderLayout);
        messageLayout->addWidget(messageTextLabel);
//        mainLayout->addLayout(messageLayout);
        mainLayout->addWidget(messageWidget);
    }

    setWidget(mainWidget);
}

void MessagesViewer::resizeEvent(QResizeEvent *event)
{
    QScrollArea::resizeEvent(event);

    int verticalScrollBarWidth = 0;
    if(verticalScrollBar()->isVisible()){
        verticalScrollBarWidth = verticalScrollBar()->width();
    }
    mainWidget->setFixedWidth(this->width() - verticalScrollBarWidth);

    mainWidget->adjustSize();
}
