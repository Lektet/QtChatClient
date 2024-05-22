#ifndef MESSAGESVIEWER_H
#define MESSAGESVIEWER_H

#include <QScrollArea>

#include <QAbstractItemModel>
#include <QLabel>

#include <list>

class MessagesViewer : public QScrollArea
{
    Q_OBJECT
public:
    explicit MessagesViewer(QWidget *parent = nullptr);

    void setDataFromModel(const QAbstractItemModel * const model);

signals:

protected:
    virtual void resizeEvent(QResizeEvent *event) override;

private:
    QWidget* mainWidget;

    std::list<QLabel*> verticalLabelsList;
};

#endif // MESSAGESVIEWER_H
