#ifndef SETTINGSWIDGET_H
#define SETTINGSWIDGET_H

#include <QWidget>

#include <QLineEdit>
#include <QSpinBox>

#include <set>

enum class Settings;

class SettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsWidget(QWidget *parent = nullptr);

signals:
    void settingsSaved(const std::set<Settings>& changedSettings);
    void canceled();

private:
    QLineEdit* usernameField;
    QLineEdit* hostField;
    QSpinBox* portPicker;

    virtual void closeEvent(QCloseEvent *event) override;
    virtual void showEvent(QShowEvent *event) override;

private slots:
    void onSaveButtonPressed();
    void onCancelButtonPressed();

private:
    bool saved;
};

#endif // SETTINGSWIDGET_H
