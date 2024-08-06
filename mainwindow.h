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

private:
    Ui::MainWindow *ui;
    QWebEngineView *view;
    QMqttClient *mqttClient;
    QMqttSubscription *subscription; // Subskrypcja tematu
    QTimer *timer; // Timer do sprawdzania statusu połączenia
    QDateTime lastMessageTime; // Czas ostatniego odebranego sygnału
    static const int connectionTimeout; // Czas sprawdzania statusu połączenia w sekundach

    QTabWidget *tabWidget; // Zakładki
    QWidget *liveTrackingTab; // Zakładka Live Tracking
    QWidget *timelineTab; // Zakładka Timeline

    QSqlDatabase db; // Połączenie z bazą danych
    QString dbUsername;
    QString dbPassword;
    static const QString DATABASE_NAME; // Stała dla nazwy bazy danych

    void connectToDatabase();
    void disconnectFromDatabase();
};

#endif // MAINWINDOW_H
