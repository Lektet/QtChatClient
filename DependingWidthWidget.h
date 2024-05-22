#ifndef CHILDWIDTHWIDGET_H
#define CHILDWIDTHWIDGET_H

#include <QWidget>

class MessageLabel;

class DependingWidthWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DependingWidthWidget(QWidget *parent = nullptr);

    virtual QSize sizeHint() const override;

    void setWidthSourceWidget(MessageLabel* source);

protected:
    virtual void resizeEvent(QResizeEvent *event) override;

private:
    MessageLabel* widthSourceWidget;
    int widthForSizeHint;
};

#endif // CHILDWIDTHWIDGET_H
