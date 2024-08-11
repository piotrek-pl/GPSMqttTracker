#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTabWidget>
#include <QVBoxLayout>
#include <QIcon>
#include <QFile>
#include <QSqlError>
#include <QMessageBox>
#include <QTextCharFormat>
#include <QSqlQuery>
#include <QDateTime>
#include <QTimeZone>
#include <QDebug>
#include <QQmlContext>
#include <QQuickItem>

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
    , view(new QWebEngineView(this)) // Widok dla zakładki Live Tracking
    , viewTimeline(new QWebEngineView(this)) // Widok dla zakładki Timeline
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

    // Podłącz sygnał zmiany miesiąca w kalendarzu
    connect(calendarWidget, &QCalendarWidget::currentPageChanged, this, &MainWindow::onCurrentPageChanged);

    // Podłącz sygnał kliknięcia w kalendarzu
    connect(calendarWidget, &QCalendarWidget::clicked, this, &MainWindow::onDateClicked);

    // Na początku upewnij się, że mapa nie jest dodana do layoutu
    viewTimeline->setVisible(false);
}

MainWindow::~MainWindow()
{
    // Upewnij się, że widoki QWebEngineView są usuwane przed usunięciem profilu
    cleanup();

    // Usuń ręcznie widoki QWebEngineView i ustaw je na nullptr
    if (view) {
        delete view;
        view = nullptr;
    }

    if (viewTimeline) {
        delete viewTimeline;
        viewTimeline = nullptr;
    }

    delete ui;
}

void MainWindow::cleanup()
{
    // Czyszczenie widoku i jego strony
    if (view) {
        view->setPage(nullptr); // Usuń stronę przed usunięciem widoku
    }

    if (viewTimeline) {
        viewTimeline->setPage(nullptr); // Usuń stronę przed usunięciem widoku
    }
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

    // Zaktualizuj kalendarz na podstawie wybranego miesiąca
    QDate currentDate = calendarWidget->selectedDate();
    updateCalendar(currentDate.year(), currentDate.month());
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

        // Sprawdź, czy kalendarz powinien być widoczny
        if (!calendarWidget->isVisible()) {
            calendarWidget->setVisible(true);
        }

        // Ukryj suwak na początku, gdy wchodzimy do zakładki "Timeline"
        if (slider) {
            slider->setVisible(false);
        }
    } else {
        disconnectFromDatabase();

        // Ukryj suwak, jeśli nie jesteśmy na zakładce Timeline
        if (slider) {
            slider->setVisible(false);
        }

        // Ponownie pokaż kalendarz i usuń widok mapy, gdy użytkownik opuści zakładkę Timeline
        if (timelineLayout->indexOf(viewTimeline) != -1) {
            timelineLayout->removeWidget(viewTimeline);
            viewTimeline->setVisible(false); // Ukryj mapę
        }
        calendarWidget->setVisible(true); // Pokaż kalendarz
    }
}


QDateTime MainWindow::convertUtcToLocal(const QDateTime &utcDateTime) {
    QTimeZone timeZone("Europe/Warsaw"); // Polska strefa czasowa
    return utcDateTime.toTimeZone(timeZone);
}

QSet<QDate> MainWindow::getDatesFromDatabase(int year, int month)
{
    QSet<QDate> dates;
    QSqlQuery query(db);

    // Tworzenie zapytania SQL do pobierania dat dla wybranego miesiąca i roku
    QString queryString = QString("SELECT DISTINCT `date` FROM `client1` WHERE YEAR(`date`) = %1 AND MONTH(`date`) = %2").arg(year).arg(month);

    if (query.exec(queryString)) {
        while (query.next()) {
            QDateTime utcDateTime = query.value(0).toDateTime();
            QDateTime localDateTime = convertUtcToLocal(utcDateTime);
            dates.insert(localDateTime.date());
        }
    } else {
        qDebug() << "Błąd podczas wykonywania zapytania SQL: " << query.lastError().text();
    }

    return dates;
}

void MainWindow::updateCalendar(int year, int month)
{
    availableDates = getDatesFromDatabase(year, month);

    QTextCharFormat availableFormat;
    availableFormat.setBackground(Qt::yellow); // Ustaw kolor tła na żółty

    QTextCharFormat unavailableFormat;
    unavailableFormat.setForeground(Qt::gray);

    QTextCharFormat currentDayFormat;
    currentDayFormat.setFontWeight(QFont::Bold); // Pogrubiona czcionka dla bieżącego dnia

    QDate currentDate = QDate::currentDate();

    // Iteracja po wszystkich dniach w miesiącu
    QDate firstDayOfMonth(year, month, 1);
    QDate lastDayOfMonth = firstDayOfMonth.addMonths(1).addDays(-1);

    for (QDate date = firstDayOfMonth; date <= lastDayOfMonth; date = date.addDays(1)) {
        if (date == currentDate) {
            if (availableDates.contains(date)) {
                currentDayFormat.setBackground(Qt::yellow); // Żółte tło dla bieżącego dnia z danymi
            } else {
                currentDayFormat.setBackground(Qt::white); // Białe tło dla bieżącego dnia bez danych
            }
            calendarWidget->setDateTextFormat(date, currentDayFormat);
        } else {
            if (availableDates.contains(date)) {
                calendarWidget->setDateTextFormat(date, availableFormat);
            } else {
                calendarWidget->setDateTextFormat(date, unavailableFormat);
            }
        }
    }

    disableUnavailableDates();
}

void MainWindow::disableUnavailableDates()
{
    QDate firstDayOfMonth = calendarWidget->selectedDate().addMonths(-1);
    QDate lastDayOfMonth = calendarWidget->selectedDate().addMonths(2);
    QDate currentDate = QDate::currentDate();

    QTextCharFormat unavailableFormat;
    unavailableFormat.setForeground(Qt::gray);

    for (QDate date = firstDayOfMonth; date <= lastDayOfMonth; date = date.addDays(1)) {
        if (!availableDates.contains(date)) {
            if (date != currentDate) {
                calendarWidget->setDateTextFormat(date, unavailableFormat);
            }
        }
    }
}

void MainWindow::onCurrentPageChanged(int year, int month)
{
    updateCalendar(year, month);
}

void MainWindow::onDateClicked(const QDate &date)
{
    if (!availableDates.contains(date)) {
        // Jeśli wybrana data jest niedostępna, pokaż ostrzeżenie
        QMessageBox::warning(this, "Niepoprawny wybór", "Wybrany dzień nie jest dostępny.");
        return;
    }

    // Ukryj kalendarz, bo teraz będzie wyświetlana mapa
    calendarWidget->setVisible(false);

    // Usuń istniejące widżety (mapa, suwak) z layoutu
    if (timelineLayout->indexOf(viewTimeline) != -1) {
        timelineLayout->removeWidget(viewTimeline);
    }
    if (slider && timelineLayout->indexOf(slider) != -1) {
        timelineLayout->removeWidget(slider);
    }

    // Tworzenie QVBoxLayout, jeśli go jeszcze nie ma
    QVBoxLayout *vboxLayout = qobject_cast<QVBoxLayout*>(timelineLayout);
    if (!vboxLayout) {
        vboxLayout = new QVBoxLayout();
        timelineTab->setLayout(vboxLayout);
    }

    // Dodaj mapę do górnej części layoutu z odpowiednią wagą (1)
    vboxLayout->addWidget(viewTimeline, 1);
    viewTimeline->setVisible(true);
    viewTimeline->setUrl(QUrl("qrc:/html/map.html"));

    // Upewnij się, że mapa się załadowała
    connect(viewTimeline, &QWebEngineView::loadFinished, this, [this, vboxLayout](bool ok) {
        if (ok) {
            if (!slider) {
                slider = new QQuickWidget;
                slider->setSource(QUrl(QStringLiteral("qrc:/qml/RangeSlider.qml")));
                slider->setResizeMode(QQuickWidget::SizeRootObjectToView);
            }

            // Resetuj pozycję suwaka do wartości początkowej (np. 0) przed jego wyświetleniem
            QObject *sliderObject = slider->rootObject();
            if (sliderObject) {
                sliderObject->setProperty("value", 0);
            }

            // Dodaj suwak poniżej mapy z odpowiednią wagą (0) i stałą wysokością
            vboxLayout->addWidget(slider, 0);
            slider->setMinimumHeight(50);  // Stała wysokość suwaka
            slider->setMaximumHeight(50);  // Zapobiega rozciąganiu suwaka
            slider->setVisible(true);
        }
    });
}


void MainWindow::showMapInTimeline()
{
    // Usuń wszystkie widżety z layoutu, zachowując kalendarz
    QLayoutItem *item;
    while ((item = timelineLayout->takeAt(0)) != nullptr) {
        if (item->widget() != calendarWidget) { // Zostaw kalendarz
            delete item->widget(); // Usuwa widżet
            delete item; // Usuwa layout item
        }
    }

    // Dodaj widok mapy do zakładki "Timeline"
    timelineLayout->addWidget(viewTimeline);
    viewTimeline->setUrl(QUrl("qrc:/html/map.html")); // Upewnij się, że widok załadował mapę
    tabWidget->setCurrentWidget(timelineTab); // Przełącz zakładkę na "Timeline"
}
