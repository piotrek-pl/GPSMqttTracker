#include "mainwindow.h"
#include "ui_mainwindow.h"

const int MainWindow::connectionTimeout = 5; // Inicjalizacja zmiennej statycznej

MainWindow::MainWindow(QMqttClient *client, QWidget *parent)
    : QMainWindow(parent), mqttClient(client), ui(new Ui::MainWindow), subscription(nullptr), timer(new QTimer(this))
{
    ui->setupUi(this);

    view = new QWebEngineView(this);
    setCentralWidget(view);

    // Załaduj plik HTML zawierający mapę Google z zasobów
    view->setUrl(QUrl("qrc:/map.html"));

    connect(qApp, &QCoreApplication::aboutToQuit, this, &MainWindow::cleanup);

    // Subskrybuj temat client1/gps/location
    QMqttTopicFilter topic("client1/gps/location");
    subscription = mqttClient->subscribe(topic);
    connect(subscription, &QMqttSubscription::messageReceived, this, &MainWindow::onMessageReceived);

    // Podłącz sygnał rozłączenia
    connect(mqttClient, &QMqttClient::disconnected, this, &MainWindow::onDisconnected);

    // Skonfiguruj timer do sprawdzania statusu połączenia
    connect(timer, &QTimer::timeout, this, &MainWindow::checkConnectionStatus);
    timer->start(1000); // Sprawdzaj co sekundę
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

void MainWindow::onMessageReceived(const QMqttMessage &message)
{
    // Obsługa odebranej wiadomości
    QString payload = message.payload();
    qDebug() << "Received message:" << payload;

    // Ukryj komunikaty, jeśli są widoczne
    QString scriptHideMessages = "hideDisconnectedMessage(); hideUnknownLocationMessage();";
    view->page()->runJavaScript(scriptHideMessages);

    // Przetwarzanie odebranej wiadomości
    QStringList data = payload.split(',');

    if (data.size() >= 4) {
        double latitude = data[2].toDouble();
        double longitude = data[3].toDouble();

        // Aktualizuj czas ostatniego odebranego sygnału
        lastMessageTime = QDateTime::currentDateTime();

        // Sprawdź, czy współrzędne są różne od 99
        if (latitude != 99 && longitude != 99) {
            // Wywołaj funkcję JavaScript, aby zaktualizować mapę i ukryć komunikat o nieznanej lokalizacji
            QString script = QString("hideUnknownLocationMessage(); updateMap(%1, %2);").arg(latitude).arg(longitude);
            view->page()->runJavaScript(script);
        } else {
            // Wyświetl informację, że lokalizacja jest nieznana
            QString scriptShowUnknownLocation = "showUnknownLocationMessage();";
            view->page()->runJavaScript(scriptShowUnknownLocation);
        }
    }
}

void MainWindow::onDisconnected()
{
    // Pokaż komunikat o zerwaniu połączenia
    QString scriptShowMessage = "showDisconnectedMessage();";
    view->page()->runJavaScript(scriptShowMessage);
}

void MainWindow::checkConnectionStatus()
{
    // Sprawdź, czy minęło więcej niż connectionTimeout sekund od ostatniego odebranego sygnału
    if (lastMessageTime.isValid() && lastMessageTime.secsTo(QDateTime::currentDateTime()) > connectionTimeout) {
        QString scriptShowMessage = "showDisconnectedMessage();";
        view->page()->runJavaScript(scriptShowMessage);
    }
}
