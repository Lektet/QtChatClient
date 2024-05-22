#include "DependingWidthWidget.h"

#include <QLayout>

#include "MessageLabel.h"

DependingWidthWidget::DependingWidthWidget(QWidget *parent)
    : QWidget{parent},
      widthSourceWidget(nullptr),
      widthForSizeHint(0)
{

}

QSize DependingWidthWidget::sizeHint() const
{
    if(widthSourceWidget == nullptr){
        return QWidget::sizeHint();
    }

    int leftMargin = 0;
    int rightMargin = 0;
    if(layout() != nullptr && !layout()->contentsMargins().isNull()){
        leftMargin = layout()->contentsMargins().left();
        rightMargin = layout()->contentsMargins().right();
    }

    auto widgetSize = QWidget::sizeHint();
    widgetSize.scale(widthSourceWidget->width() + leftMargin + rightMargin, widgetSize.height(), Qt::IgnoreAspectRatio);
    return widgetSize;
}

void DependingWidthWidget::setWidthSourceWidget(MessageLabel *source)
{
    if(widthSourceWidget == source){
        return;
    }
    else if(widthSourceWidget != nullptr){
        disconnect(widthSourceWidget);
    }

    widthSourceWidget = source;
    if(widthSourceWidget == nullptr){
        return;
    }

    connect(widthSourceWidget, &MessageLabel::requiredWidth,
            this, [this](){
        updateGeometry();
    });
}

void DependingWidthWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    updateGeometry();
}
