#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QWidget>
#include <QtMqtt/QMqttClient>
#include <QMessageBox>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QKeyEvent>
#include "mainwindow.h"

class LoginWindow : public QWidget {
    Q_OBJECT

public:
    LoginWindow(QWidget *parent = nullptr);
    ~LoginWindow();

    QString getUsername() const { return mqttUsername; }
    QString getPassword() const { return mqttPassword; }

protected:
    bool event(QEvent *event) override;

private slots:
    void onConnectButtonClicked();
    void onConnected();
    void onDisconnected();

private:
    QLineEdit *usernameLineEdit;
    QLineEdit *passwordLineEdit;
    QPushButton *connectButton;

    QString mqttUsername;
    QString mqttPassword;

    QMqttClient *mqttClient;
    MainWindow *mainWindow;
};

#endif // LOGINWINDOW_H
