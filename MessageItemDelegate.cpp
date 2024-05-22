#include "MessageItemDelegate.h"

#include <QPainter>
#include <QFontMetrics>
#include <QDateTime>

#include "MessageDataRole.h"

#include <QDebug>

const int HEADER_HEIGHT = 20;
const QString dateFormat = "dd.MM.yyyy hh:mm:ss";

MessageItemDelegate::MessageItemDelegate(QObject *parent) :
    QStyledItemDelegate(parent),
    width(0)
{

}

void MessageItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem styledOption(option);
    initStyleOption(&styledOption, index);
    painter->save();

    painter->drawText(option.rect, index.data(MessageDataRole::Time).toDateTime().toString(dateFormat));

    auto messageTextDrawRect = option.rect.translated(0, HEADER_HEIGHT);
    painter->drawText(messageTextDrawRect, index.data(MessageDataRole::Text).toString());

    painter->restore();

    auto font = painter->font();
    QFontMetrics fontMetrics(font);
    auto textRect = fontMetrics.boundingRect(styledOption.rect,
                                             Qt::AlignLeft | Qt::TextWordWrap,
                                             index.data(MessageDataRole::Text).toString());

//    qDebug() << "text: " << index.data(MessageDataRole::Text).toString();
//    qDebug() << "Styled option rect: " << styledOption.rect;
//    qDebug() << "text bounding rect " << textRect;
}

QSize MessageItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QRect boundingRect(0, 0, width, 10);


//    qDebug() << "width: " << width;
//    qDebug() << "boundingRect: " << boundingRect;
//    auto boundingRect = option.rect;
//    boundingRect.translate(-boundingRect.x(), -boundingRect.y());
    auto text = index.data(MessageDataRole::Text).toString();
    auto textRect = option.fontMetrics.boundingRect(boundingRect,
                                                    Qt::AlignLeft | Qt::TextWordWrap,
                                                    text);

    auto size = textRect.size();
    size.setHeight(size.height() + HEADER_HEIGHT);

//    qDebug() << "Computed size: " << size;

//    auto size = option.rect.size();
//    size.setHeight(40);
    return size;
}

void MessageItemDelegate::setWidth(const int width)
{
//    qDebug() << "setWidth() width: " << width;
    this->width = width;
}

int MessageItemDelegate::getWidth() const
{
    return width;
}
