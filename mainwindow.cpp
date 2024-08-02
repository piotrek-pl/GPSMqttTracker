#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), mqttClient(nullptr), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    view = new QWebEngineView(this);
    setCentralWidget(view);

    // Załaduj plik HTML zawierający mapę Google z zasobów
    view->setUrl(QUrl("qrc:/map.html"));
}

MainWindow::MainWindow(QMqttClient *client, QWidget *parent)
    : QMainWindow(parent), mqttClient(client), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    view = new QWebEngineView(this);
    setCentralWidget(view);

    // Załaduj plik HTML zawierający mapę Google z zasobów
    view->setUrl(QUrl("qrc:/map.html"));
}

MainWindow::~MainWindow()
{
    delete ui;
}
