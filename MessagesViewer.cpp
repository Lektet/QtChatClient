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
    mainWidget(new QWidget(this)),
    mainWidgetLayout(new QVBoxLayout(mainWidget))
{
    setWidgetResizable(true);
    setWidget(mainWidget);

    mainWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
}

void MessagesViewer::setModel(const QAbstractListModel * modelToSet)
{
    model = modelToSet;

    fillFromModel();
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

        messageWidgets.push_back(messageWidget);

        messageLayout->addLayout(messageHeaderLayout);
        messageLayout->addWidget(messageTextLabel);
        mainWidgetLayout->addWidget(messageWidget);
    }
}
