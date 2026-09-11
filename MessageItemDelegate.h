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

    void setRightMargin(const int width);
    int getRightMargin() const;

private:
    int rightMargin;
};

#endif // MESSAGEDELEGATE_H
