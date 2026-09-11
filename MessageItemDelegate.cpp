#include "MessageItemDelegate.h"

#include <QPainter>
#include <QFontMetrics>
#include <QDateTime>

#include "MessageDataRole.h"

#include <QDebug>

const int HEADER_HEIGHT = 20;
const QString dateFormat = "dd.MM.yyyy hh:mm:ss";

const int MARGIN = 5;
const int PADDING = 4;

MessageItemDelegate::MessageItemDelegate(QObject *parent) :
    QStyledItemDelegate(parent),
    rightMargin(0)
{

}

void MessageItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem styledOption(option);
    initStyleOption(&styledOption, index);

    auto drawRect = option.rect.adjusted(MARGIN, MARGIN, -MARGIN, -MARGIN);

    painter->save();

    painter->setRenderHint(QPainter::Antialiasing);

    painter->setBrush(QBrush(QColor(Qt::GlobalColor::lightGray)));
    painter->drawRoundedRect(drawRect, 6, 6);

    auto textDrawRect = drawRect.adjusted(PADDING, PADDING, - PADDING, - PADDING);
    painter->drawText(textDrawRect, index.data(MessageDataRole::Time).toDateTime().toString(dateFormat));

    auto messageTextDrawRect = textDrawRect.translated(0, HEADER_HEIGHT);
    painter->drawText(messageTextDrawRect, index.data(MessageDataRole::Text).toString());

    painter->restore();
}

QSize MessageItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem styledOption(option);
    initStyleOption(&styledOption, index);

    QRect boundingRect(0, 0, styledOption.widget->width() - rightMargin - MARGIN * 2 - PADDING * 2, 10);
    auto text = index.data(MessageDataRole::Text).toString();
    auto textRect = option.fontMetrics.boundingRect(boundingRect,
                                                    Qt::AlignLeft | Qt::TextWordWrap,
                                                    text);

    auto size = textRect.size();
    size.setWidth(size.width() + MARGIN * 2 + PADDING * 2);
    size.setHeight(size.height() + HEADER_HEIGHT + MARGIN * 2 + PADDING * 2);
    return size;
}

void MessageItemDelegate::setRightMargin(const int width)
{
    rightMargin = width;
}

int MessageItemDelegate::getRightMargin() const
{
    return rightMargin;
}
