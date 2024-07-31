#include "mainwindow.h"

MainWindow::MainWindow(QMqttClient *client, QWidget *parent)
    : QMainWindow(parent)
    , mqttClient(client)
{
    setWindowTitle("Main Window");
}

MainWindow::~MainWindow() {
}
