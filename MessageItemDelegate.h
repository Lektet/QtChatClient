#ifndef MESSAGEDELEGATE_H
#define MESSAGEDELEGATE_H

#include <QStyledItemDelegate>

class MessageItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit MessageItemDelegate(QObject *parent = nullptr);

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void setWidth(const int width);
    int getWidth() const;

private:
    int width;
};

#endif // MESSAGEDELEGATE_H
