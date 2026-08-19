#ifndef USERMANAGMENTWIDGET_H
#define USERMANAGMENTWIDGET_H

#include <QWidget>

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>

enum class UserRole;

class UserManagmentWidget: public QWidget{
    Q_OBJECT

public:
    explicit UserManagmentWidget();

signals:
    void newUserSubmitted(const QString& username, const QString& password, const UserRole role);

private:
    QLineEdit* usernameField;
    QLabel* usernameErrorLabel;
    QLineEdit* passwordField;
    QLabel* passwordErrorLabel;
    QComboBox* roleComboBox;

    QPushButton* submitButton;

    void onSubmitButtonPressed();
};

#endif // USERMANAGMENTWIDGET_H
