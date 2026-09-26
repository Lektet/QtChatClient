#ifndef MESSAGESVIEWER_H
#define MESSAGESVIEWER_H

#include <QScrollArea>

#include <QVBoxLayout>

#include <QAbstractItemModel>
#include <QAbstractListModel>
#include <QLabel>

#include <list>

class MessagesViewer : public QScrollArea
{
    Q_OBJECT
public:
    explicit MessagesViewer(QWidget *parent = nullptr);

    void setModel(const QAbstractListModel *modelToSet);

    std::vector<QString> viewedIds() const;
    std::vector<int> viewedIndexes() const;

signals:
    void messagesIdsAbouttoBeViewed(const quint64 fromId, const quint64 toId);
    void resized();
    void viewedMessagesChanged();

protected:
    virtual void resizeEvent(QResizeEvent *event) override;

private:
    const QAbstractListModel* model;

    QWidget* mainWidget;
    QVBoxLayout* mainWidgetLayout;


    std::list<QWidget*> messageWidgets;
    std::vector<QWidget*> viewedMessages;

    void fillFromModel();
    void onRowsInserted(const QModelIndex &parent, int first, int last);
    void onRowsRemoved(const QModelIndex &parent, int first, int last);
    void onDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles = QVector<int>());

    void onScrollbarValueChanged(int value);

    QWidget* createMessageWidget(const QModelIndex& modelIndex);
};

#endif // MESSAGESVIEWER_H
