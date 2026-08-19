#include "UserRole.h"
#include <UserManagmentWidget.h>

#include <QVBoxLayout>
#include <QFormLayout>

#include <QRegularExpression>
#include <QRegularExpressionValidator>

#include "Constants.h"

const QString ERROR_LABEL_STYLE = "QLabel{"
                                  "color: red;"
                                  "}";

UserManagmentWidget::UserManagmentWidget(): QWidget(),
    usernameField(new QLineEdit()),
    usernameErrorLabel(new QLabel(tr("Field is empty!"))),
    passwordField(new QLineEdit()),
    passwordErrorLabel(new QLabel(tr("Field is empty!"))),
    roleComboBox(new QComboBox()),
    submitButton(new QPushButton(tr("Add user")))
{
    auto layout = new QVBoxLayout();

    auto formLayout = new QFormLayout();

    auto usernameFieldLayout = new QHBoxLayout();
    QRegularExpression usernameRegExp(QString("^(\\w{1,%1})$").arg(Constants::USERNAME_MAX_LENGTH));
    auto usernameValidator = new QRegularExpressionValidator(usernameRegExp, usernameField);
    usernameField->setValidator(usernameValidator);
    usernameFieldLayout->addWidget(usernameField);
    usernameErrorLabel->setStyleSheet(ERROR_LABEL_STYLE);
    usernameErrorLabel->hide();
    usernameFieldLayout->addWidget(usernameErrorLabel);
    formLayout->addRow(tr("Username:"), usernameFieldLayout);

    auto passwordFeldLayout = new QHBoxLayout();
    passwordField->setEchoMode(QLineEdit::EchoMode::Password);
    QRegularExpression passwordRegExp(QString("^(\\w{1,%1})$").arg(Constants::PASSWORD_MAX_LENGTH));
    auto passwordValidator = new QRegularExpressionValidator(passwordRegExp, passwordField);
    passwordField->setValidator(passwordValidator);
    passwordFeldLayout->addWidget(passwordField);
    passwordErrorLabel->setStyleSheet(ERROR_LABEL_STYLE);
    passwordErrorLabel->hide();
    passwordFeldLayout->addWidget(passwordErrorLabel);
    formLayout->addRow(tr("Password:"), passwordFeldLayout);

    roleComboBox->addItem(tr("User"), static_cast<int>(UserRole::User));
    roleComboBox->addItem(tr("Admin"), static_cast<int>(UserRole::Admin));
    formLayout->addRow(tr("Role:"), roleComboBox);


    formLayout->addRow(submitButton);

    layout->addLayout(formLayout);

    setLayout(layout);

    connect(submitButton, &QPushButton::pressed,
            this, &UserManagmentWidget::onSubmitButtonPressed);
}

void UserManagmentWidget::onSubmitButtonPressed()
{
    bool canSubmit = true;
    if(usernameField->text().isEmpty()){
        canSubmit = false;
        usernameErrorLabel->show();
    }
    if(passwordField->text().isEmpty()){
        canSubmit = false;
        passwordErrorLabel->show();
    }

    if(canSubmit){
        emit newUserSubmitted(usernameField->text(), passwordField->text(), UserRole(roleComboBox->currentData().toInt()));
    }
}
