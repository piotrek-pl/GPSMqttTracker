#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWebEngineView>
#include <QMqttClient>
#include <QMqttSubscription>
#include <QMqttMessage>
#include <QTimer>
#include <QDateTime>
#include <QTabWidget>
#include <QSqlDatabase>
#include <QCalendarWidget>
#include <QVBoxLayout>
#include <QSet>
#include <QQuickWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QMqttClient *client, const QString &username, const QString &password, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void cleanup(); // Metoda czyszcząca
    void onMessageReceived(const QMqttMessage &message); // Slot do obsługi wiadomości
    void checkConnectionStatus(); // Slot do sprawdzania statusu połączenia
    void onDisconnected(); // Slot do obsługi rozłączenia
    void onTabIndexChanged(int index); // Slot do zmiany zakładki
    void onCurrentPageChanged(int year, int month); // Slot do zmiany miesiąca w kalendarzu
    void onDateClicked(const QDate &date); // Slot do obsługi kliknięcia w datę

private:
    Ui::MainWindow *ui;
    QWebEngineView *view;
    QWebEngineView *viewTimeline; // Nowy widok dla zakładki Timeline
    QMqttClient *mqttClient;
    QMqttSubscription *subscription; // Subskrypcja tematu
    QTimer *timer; // Timer do sprawdzania statusu połączenia
    QDateTime lastMessageTime; // Czas ostatniego odebranego sygnału
    static const int connectionTimeout; // Czas sprawdzania statusu połączenia w sekundach

    QTabWidget *tabWidget; // Zakładki
    QWidget *liveTrackingTab; // Zakładka Live Tracking
    QWidget *timelineTab; // Zakładka Timeline

    QVBoxLayout *timelineLayout; // Layout dla zakładki Timeline
    QCalendarWidget *calendarWidget; // Kalendarz do zakładki Timeline

    QSqlDatabase db; // Połączenie z bazą danych
    QString dbUsername;
    QString dbPassword;
    static const QString DATABASE_NAME; // Stała dla nazwy bazy danych

    QSet<QDate> availableDates; // Zbiór dostępnych dat

    QQuickWidget *slider = nullptr;

    void connectToDatabase();
    void disconnectFromDatabase();
    QSet<QDate> getDatesFromDatabase(int year, int month); // Funkcja pobierająca daty z bazy danych
    void updateCalendar(int year, int month); // Funkcja aktualizująca kalendarz
    QDateTime convertUtcToLocal(const QDateTime &utcDateTime); // Funkcja konwertująca czas z UTC na czas lokalny
    void disableUnavailableDates(); // Funkcja wyłączająca niedostępne daty
    void showMapInTimeline();

    void loadDataForSelectedDate(const QDate &selectedDate);

    void drawRouteOnMap();
};

#endif // MAINWINDOW_H
