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

struct LocationData {
    QDateTime localTime;
    double latitude;
    double longitude;
};

QList<LocationData> locationDataList;

MainWindow::MainWindow(QMqttClient *client, const QString &username, const QString &password, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , mqttClient(client)
    , dbUsername(username)
    , dbPassword(password)
    , subscription(nullptr)
    , timer(new QTimer(this))
    , calendarWidget(new QCalendarWidget(this))
    , view(new QWebEngineView(this))
    , viewTimeline(new QWebEngineView(this))
    , slider(nullptr)
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

    QVBoxLayout *liveTrackingLayout = new QVBoxLayout(liveTrackingTab);
    liveTrackingLayout->addWidget(view);
    liveTrackingTab->setLayout(liveTrackingLayout);

    timelineLayout = new QVBoxLayout(timelineTab);
    calendarWidget->setVisible(false);
    timelineLayout->addWidget(calendarWidget);
    timelineTab->setLayout(timelineLayout);

    tabWidget->addTab(liveTrackingTab, "Live Tracking");
    tabWidget->addTab(timelineTab, "Timeline");

    setCentralWidget(tabWidget);

    view->setUrl(QUrl("qrc:/html/map.html"));

    connect(qApp, &QCoreApplication::aboutToQuit, this, &MainWindow::cleanup);

    QMqttTopicFilter topic("client1/gps/location");
    subscription = mqttClient->subscribe(topic);
    connect(subscription, &QMqttSubscription::messageReceived, this, &MainWindow::onMessageReceived);

    connect(mqttClient, &QMqttClient::disconnected, this, &MainWindow::onDisconnected);

    connect(timer, &QTimer::timeout, this, &MainWindow::checkConnectionStatus);
    timer->start(1000);

    connect(tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabIndexChanged);

    connect(calendarWidget, &QCalendarWidget::currentPageChanged, this, &MainWindow::onCurrentPageChanged);

    connect(calendarWidget, &QCalendarWidget::clicked, this, &MainWindow::onDateClicked);

    viewTimeline->setVisible(false);
}

MainWindow::~MainWindow()
{
    cleanup();

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
    if (view) {
        view->setPage(nullptr);
    }

    if (viewTimeline) {
        viewTimeline->setPage(nullptr);
    }
}

void MainWindow::onMessageReceived(const QMqttMessage &message)
{
    QString payload = message.payload();
    qDebug() << "Received message:" << payload;

    QString scriptHideMessages = "hideDisconnectedMessage(); hideUnknownLocationMessage();";
    view->page()->runJavaScript(scriptHideMessages);

    QStringList data = payload.split(',');

    if (data.size() >= 4) {
        double latitude = data[2].toDouble();
        double longitude = data[3].toDouble();

        lastMessageTime = QDateTime::currentDateTime();

        if (latitude != 99 && longitude != 99) {
            QString script = QString("hideUnknownLocationMessage(); updateMap(%1, %2);").arg(latitude).arg(longitude);
            view->page()->runJavaScript(script);
        } else {
            QString scriptShowUnknownLocation = "showUnknownLocationMessage();";
            view->page()->runJavaScript(scriptShowUnknownLocation);
        }
    }
}

void MainWindow::onDisconnected()
{
    QString scriptShowMessage = "showDisconnectedMessage();";
    view->page()->runJavaScript(scriptShowMessage);
}

void MainWindow::checkConnectionStatus()
{
    if (lastMessageTime.isValid() && lastMessageTime.secsTo(QDateTime::currentDateTime()) > connectionTimeout) {
        QString scriptShowMessage = "showDisconnectedMessage();";
        view->page()->runJavaScript(scriptShowMessage);
    }
}

void MainWindow::connectToDatabase()
{
    QString connectionName = "gps_connection";

    if (QSqlDatabase::contains(connectionName)) {
        db = QSqlDatabase::database(connectionName);
    } else {
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

    calendarWidget->setVisible(true);

    QDate currentDate = calendarWidget->selectedDate();
    updateCalendar(currentDate.year(), currentDate.month());
}

void MainWindow::disconnectFromDatabase()
{
    QString connectionName = "gps_connection";

    if (QSqlDatabase::contains(connectionName)) {
        {
            QSqlDatabase db = QSqlDatabase::database(connectionName);

            if (db.isOpen()) {
                db.close();
                qDebug() << "Połączenie z bazą danych zostało zamknięte.";
            }
        }

        db = QSqlDatabase::database();
        QSqlDatabase::removeDatabase(connectionName);
        qDebug() << "Połączenie z bazą danych zostało usunięte.";
    }

    calendarWidget->setVisible(false);
}

QDateTime MainWindow::convertUtcToLocal(const QDateTime &utcDateTime) {
    QTimeZone timeZone("Europe/Warsaw");
    return utcDateTime.toTimeZone(timeZone);
}

void MainWindow::loadDataForSelectedDate(const QDate &selectedDate) {
    locationDataList.clear();

    QSqlQuery query(db);
    query.prepare("SELECT `date`, `time`, `latitude`, `longitude` FROM `client1` WHERE `date` = :date");
    query.bindValue(":date", selectedDate.toString("yyyy-MM-dd"));

    if (query.exec()) {
        while (query.next()) {
            QDate date = query.value(0).toDate();
            QTime time = query.value(1).toTime();
            QDateTime utcDateTime(date, time, Qt::UTC);
            QDateTime localDateTime = convertUtcToLocal(utcDateTime);

            if (localDateTime.date() == selectedDate) {
                LocationData data;
                data.localTime = localDateTime;
                data.latitude = query.value(2).toDouble();
                data.longitude = query.value(3).toDouble();
                locationDataList.append(data);
            }
        }

        // Wyświetl wczytane dane w konsoli z dokładnością do 6 miejsc po przecinku
        qDebug() << "Wczytane dane dla wybranego dnia:";
        for (const auto &data : locationDataList) {
            qDebug() << data.localTime.toString("yyyy-MM-dd hh:mm:ss")
                     << QString::number(data.latitude, 'f', 6)
                     << QString::number(data.longitude, 'f', 6);
        }

        // Przygotuj skrypt JavaScript do rysowania trasy
        if (!locationDataList.isEmpty()) {
            QStringList coordinates;
            for (const auto &data : locationDataList) {
                QString coord = QString("[%1, %2]").arg(data.latitude, 0, 'f', 6).arg(data.longitude, 0, 'f', 6);
                coordinates.append(coord);
            }

            QString jsScript = QString("drawPath([%1]);").arg(coordinates.join(", "));
            connect(viewTimeline, &QWebEngineView::loadFinished, this, [this, jsScript](bool ok) {
                if (ok) {
                    viewTimeline->page()->runJavaScript(jsScript);
                } else {
                    qDebug() << "Failed to load the map.";
                }
            });
        }
    } else {
        qDebug() << "Błąd podczas wykonywania zapytania SQL: " << query.lastError().text();
    }
}



void MainWindow::onDateClicked(const QDate &date) {
    if (!availableDates.contains(date)) {
        QMessageBox::warning(this, "Niepoprawny wybór", "Wybrany dzień nie jest dostępny.");
        return;
    }

    loadDataForSelectedDate(date);

    // Ukryj kalendarz po wybraniu daty
    calendarWidget->setVisible(false);

    // Usuń istniejące widżety, aby upewnić się, że są one dodane w odpowiedniej kolejności
    while (timelineLayout->count() > 0) {
        QWidget *widget = timelineLayout->takeAt(0)->widget();
        if (widget) {
            widget->setVisible(false);
            timelineLayout->removeWidget(widget);
        }
    }

    // Dodaj mapę do layoutu
    if (timelineLayout->indexOf(viewTimeline) == -1) {
        timelineLayout->addWidget(viewTimeline);
    }

    viewTimeline->setVisible(true);
    viewTimeline->setUrl(QUrl("qrc:/html/map.html"));

    // Dodaj suwak pod mapą
    if (!slider) {
        slider = new QQuickWidget;
        slider->setSource(QUrl(QStringLiteral("qrc:/qml/RangeSlider.qml")));
        slider->setResizeMode(QQuickWidget::SizeRootObjectToView);

        connect(slider, &QQuickWidget::statusChanged, this, [this]() {
            if (slider->status() == QQuickWidget::Ready) {
                QObject *sliderObject = slider->rootObject();
                if (sliderObject) {
                    QMetaObject::invokeMethod(sliderObject, "setFirstValue", Q_ARG(QVariant, QVariant(0)));
                    QMetaObject::invokeMethod(sliderObject, "setSecondValue", Q_ARG(QVariant, QVariant(100)));
                }
                slider->setVisible(true);
            }
        });
    } else {
        QObject *sliderObject = slider->rootObject();
        if (sliderObject) {
            QMetaObject::invokeMethod(sliderObject, "setFirstValue", Q_ARG(QVariant, QVariant(0)));
            QMetaObject::invokeMethod(sliderObject, "setSecondValue", Q_ARG(QVariant, QVariant(100)));
        }

        slider->setVisible(true);
    }

    if (timelineLayout->indexOf(slider) == -1) {
        timelineLayout->addWidget(slider);
    }

    slider->setMinimumHeight(50);
    slider->setMaximumHeight(50);
}



void MainWindow::onTabIndexChanged(int index)
{
    if (index == 1) {
        connectToDatabase();
        if (!calendarWidget->isVisible()) {
            calendarWidget->setVisible(true);
        }
        if (slider) {
            slider->setVisible(false);
        }
    } else {
        disconnectFromDatabase();
        if (slider) {
            slider->setVisible(false);
        }
        if (timelineLayout->indexOf(viewTimeline) != -1) {
            timelineLayout->removeWidget(viewTimeline);
            viewTimeline->setVisible(false);
        }
        calendarWidget->setVisible(true);
    }
}

QSet<QDate> MainWindow::getDatesFromDatabase(int year, int month)
{
    QSet<QDate> dates;
    QSqlQuery query(db);

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
    availableFormat.setBackground(Qt::yellow);

    QTextCharFormat unavailableFormat;
    unavailableFormat.setForeground(Qt::gray);

    QTextCharFormat currentDayFormat;
    currentDayFormat.setFontWeight(QFont::Bold);

    QDate currentDate = QDate::currentDate();

    QDate firstDayOfMonth(year, month, 1);
    QDate lastDayOfMonth = firstDayOfMonth.addMonths(1).addDays(-1);

    for (QDate date = firstDayOfMonth; date <= lastDayOfMonth; date = date.addDays(1)) {
        if (date == currentDate) {
            if (availableDates.contains(date)) {
                currentDayFormat.setBackground(Qt::yellow);
            } else {
                currentDayFormat.setBackground(Qt::white);
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

