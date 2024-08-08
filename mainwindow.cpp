#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTabWidget>
#include <QVBoxLayout>
#include <QIcon>
#include <QFile>
#include <QSqlError>
#include <QMessageBox>
#include <QCalendarWidget>

const int MainWindow::connectionTimeout = 5; // Inicjalizacja zmiennej statycznej
const QString MainWindow::DATABASE_NAME = "GPS"; // Stała dla nazwy bazy danych

MainWindow::MainWindow(QMqttClient *client, const QString &username, const QString &password, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , mqttClient(client)
    , dbUsername(username)
    , dbPassword(password)
    , subscription(nullptr)
    , timer(new QTimer(this))
    , calendarWidget(new QCalendarWidget(this))
{
    ui->setupUi(this);

    // Ustaw tytuł okna
    setWindowTitle("GPS Tracker");

    // Ustaw ikonę okna
    setWindowIcon(QIcon(":/img/icon.png"));

    // Załaduj styl z pliku .qss
    QFile file(":/styles.qss");
    if (file.open(QFile::ReadOnly)) {
        QString styleSheet = QLatin1String(file.readAll());
        qApp->setStyleSheet(styleSheet);
        file.close();
    }

    // Tworzenie QTabWidget i zakładek
    tabWidget = new QTabWidget(this);
    liveTrackingTab = new QWidget();
    timelineTab = new QWidget();

    // Ustawianie layoutu dla zakładki Live Tracking
    QVBoxLayout *liveTrackingLayout = new QVBoxLayout(liveTrackingTab);
    view = new QWebEngineView(this);
    liveTrackingLayout->addWidget(view);
    liveTrackingTab->setLayout(liveTrackingLayout);

    // Ustawianie layoutu dla zakładki Timeline
    timelineLayout = new QVBoxLayout(timelineTab);
    calendarWidget->setVisible(false); // Początkowo ukryj kalendarz
    timelineLayout->addWidget(calendarWidget);
    timelineTab->setLayout(timelineLayout);

    // Dodawanie zakładek do QTabWidget
    tabWidget->addTab(liveTrackingTab, "Live Tracking");
    tabWidget->addTab(timelineTab, "Timeline");

    // Ustawianie QTabWidget jako centralny widget
    setCentralWidget(tabWidget);

    // Załaduj plik HTML zawierający mapę Google z zasobów
    view->setUrl(QUrl("qrc:/html/map.html"));

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

    // Podłącz slot do zmiany zakładki
    connect(tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabIndexChanged);
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

void MainWindow::connectToDatabase()
{
    QString connectionName = "gps_connection";

    // Sprawdź, czy połączenie o tej nazwie już istnieje
    if (QSqlDatabase::contains(connectionName)) {
        db = QSqlDatabase::database(connectionName);
    } else {
        // Stwórz nowe połączenie
        db = QSqlDatabase::addDatabase("QMYSQL", connectionName);
        db.setHostName("51.20.193.191");
        db.setDatabaseName(DATABASE_NAME);
        db.setUserName(dbUsername);
        db.setPassword(dbPassword);
    }

    if (!db.open()) {
        QMessageBox::critical(this, "Błąd bazy danych", "Nie można połączyć się z bazą danych. " + db.lastError().text());
        return;
    }

    // Po udanym połączeniu z bazą danych, pokaż kalendarz
    calendarWidget->setVisible(true);
}

void MainWindow::disconnectFromDatabase()
{
    QString connectionName = "gps_connection";

    if (QSqlDatabase::contains(connectionName)) {
        {
            // Pobierz połączenie z bazą danych
            QSqlDatabase db = QSqlDatabase::database(connectionName);

            // Sprawdź, czy połączenie jest otwarte
            if (db.isOpen()) {
                db.close();
                qDebug() << "Połączenie z bazą danych zostało zamknięte.";
            }
        }

        // Po wyjściu z bloku powyżej, obiekt db jest niszczony
        // Usuń połączenie z bazy danych
        db = QSqlDatabase::database(); // usunięcie uchwytów QSqlDatabase
                                       // konieczne przed usunięciem!
        QSqlDatabase::removeDatabase(connectionName);
        qDebug() << "Połączenie z bazą danych zostało usunięte.";
    }

    // Po rozłączeniu z bazą danych, ukryj kalendarz
    calendarWidget->setVisible(false);
}

void MainWindow::onTabIndexChanged(int index)
{
    if (index == 1) { // Zakładka Timeline ma indeks 1
        connectToDatabase();
    } else {
        disconnectFromDatabase();
    }
}
