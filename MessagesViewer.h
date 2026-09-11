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

signals:
    void messagesIdsAbouttoBeViewed(const quint64 fromId, const quint64 toId);
    void resized();

protected:
    virtual void resizeEvent(QResizeEvent *event) override;

private:
    const QAbstractListModel* model;

    QWidget* mainWidget;
    QVBoxLayout* mainWidgetLayout;


    std::list<QWidget*> messageWidgets;

    void fillFromModel();
};

#endif // MESSAGESVIEWER_H
