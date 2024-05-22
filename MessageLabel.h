#ifndef MESSAGELABEL_H
#define MESSAGELABEL_H

#include <QLabel>

class MessageLabel : public QLabel
{
    Q_OBJECT
public:
    MessageLabel(const QString &text, QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

    virtual QSize sizeHint() const override;

signals:
    void requiredWidth(int width);

protected:
    virtual void resizeEvent(QResizeEvent *event) override;

};

#endif // MESSAGELABEL_H
