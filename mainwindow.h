#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtMqtt/QMqttClient>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QMqttClient *client, QWidget *parent = nullptr);
    ~MainWindow();

private:
    QMqttClient *mqttClient;
};

#endif // MAINWINDOW_H
