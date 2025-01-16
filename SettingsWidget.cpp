#include "SettingsWidget.h"

#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QRegularExpressionValidator>
#include <QRegularExpression>
#include <QSettings>
#include <QMessageBox>

#include "Settings.h"

SettingsWidget::SettingsWidget(QWidget *parent)
    : QWidget{parent},
    usernameField(new QLineEdit()),
    hostField(new QLineEdit()),
    portPicker(new QSpinBox()),
    saved(false)
{
    auto widgetLayout = new QVBoxLayout(this);
    
    auto usernameLabel = new QLabel(tr("Username: "));
    widgetLayout->addWidget(usernameLabel);
    widgetLayout->addWidget(usernameField);
    
    auto hostPortLayout = new QHBoxLayout();
    
    auto hostLayout = new QVBoxLayout();

    auto hostLabel = new QLabel(tr("Host:"));
    QRegularExpression hostRegExp("^((25[0-5]|(2[0-4]|1\\d|[1-9]|)\\d)\\.){3}(25[0-5]|(2[0-4]|1\\d|[1-9]|)\\d)$");
    auto hostValidator = new QRegularExpressionValidator(hostRegExp, hostField);
    hostField->setValidator(hostValidator);
    hostLayout->addWidget(hostLabel);
    hostLayout->addWidget(hostField);

    hostPortLayout->addLayout(hostLayout);

    auto portLayout = new QVBoxLayout();

    auto portLabel = new QLabel(tr("Port:"));
    portPicker->setMinimum(0);
    portPicker->setMaximum(65535);
    portLayout->addWidget(portLabel);
    portLayout->addWidget(portPicker);

    hostPortLayout->addLayout(portLayout);

    widgetLayout->addLayout(hostPortLayout);

    auto buttonsLayout = new QHBoxLayout();

    auto cancelButton = new QPushButton(tr("Cancel"));
    auto saveButton = new QPushButton(tr("Save"));
    connect(cancelButton, &QPushButton::clicked,
            this, &SettingsWidget::close);
    connect(saveButton, &QPushButton::clicked,
            this, &SettingsWidget::onSaveButtonPressed);

    buttonsLayout->addWidget(cancelButton);
    buttonsLayout->addWidget(saveButton);

    widgetLayout->addLayout(buttonsLayout);
}

void SettingsWidget::closeEvent(QCloseEvent *event)
{
    QWidget::closeEvent(event);

    if(saved){
        saved = false;
        return;
    }
    emit canceled();
}

void SettingsWidget::showEvent(QShowEvent *event)
{
    QSettings settings;
    auto username = settings.value("username").toString();
    auto host = settings.value("serverHost").toString();
    auto port = settings.value("serverPort").toInt();
    usernameField->setText(username);
    hostField->setText(host);
    portPicker->setValue(port);

    QWidget::showEvent(event);
}

void SettingsWidget::onSaveButtonPressed()
{
    QSettings settings;
    auto oldUsername = settings.value("username").toString();
    auto oldServerHost = settings.value("serverHost").toString();
    auto oldServerPort = settings.value("serverPort").toInt();

    auto newUsername = usernameField->text();
    auto newServerHost = hostField->text();
    auto newServerPort = portPicker->text().toInt();

    std::set<Settings> changedSettings;
    if(oldUsername != newUsername){
        changedSettings.emplace(Settings::Username);
        settings.setValue("username", newUsername);
    }
    if(oldServerHost != newServerHost){
        changedSettings.emplace(Settings::Host);
        settings.setValue("serverHost", newServerHost);
    }
    if(oldServerPort != newServerPort){
        changedSettings.emplace(Settings::Port);
        settings.setValue("serverPort", newServerPort);
    }

    saved = true;
    close();

    emit settingsSaved(changedSettings);
}

void SettingsWidget::onCancelButtonPressed()
{
    close();
}
