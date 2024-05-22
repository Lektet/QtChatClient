#include "MessageLabel.h"

#include <QFontMetrics>
#include <QResizeEvent>

#include <QDebug>

MessageLabel::MessageLabel(const QString &text, QWidget *parent, Qt::WindowFlags f) :
    QLabel(text, parent, f)
{

}

QSize MessageLabel::sizeHint() const
{
    QRect boundingRect(0, 0, width() - margin() * 2, 10);

    QFontMetrics fontMetrics(font());
    auto textRect = fontMetrics.boundingRect(boundingRect,
                                                    Qt::AlignLeft | Qt::TextWordWrap,
                                                    text());
    qDebug() << "SizeHint: " << textRect.size();
    auto widgetSize = textRect.size();
    widgetSize += QSize(margin(), margin());
    return widgetSize;
}

void MessageLabel::resizeEvent(QResizeEvent *event)
{
    QLabel::resizeEvent(event);

    updateGeometry();

    if(event->oldSize().width() != event->size().width()){
//        emit requiredWidth();
    }
}
