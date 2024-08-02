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

    connect(qApp, &QCoreApplication::aboutToQuit, this, &MainWindow::cleanup);
}

MainWindow::MainWindow(QMqttClient *client, QWidget *parent)
    : QMainWindow(parent), mqttClient(client), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    view = new QWebEngineView(this);
    setCentralWidget(view);

    // Załaduj plik HTML zawierający mapę Google z zasobów
    view->setUrl(QUrl("qrc:/map.html"));

    connect(qApp, &QCoreApplication::aboutToQuit, this, &MainWindow::cleanup);
}

void MainWindow::cleanup()
{
    // Czyszczenie widoku i jego strony
    if (view) {
        view->setPage(nullptr);
        delete view;
        view = nullptr;
    }
}

MainWindow::~MainWindow()
{
    cleanup(); // Upewnij się, że widok jest wyczyszczony
    delete ui;
}
