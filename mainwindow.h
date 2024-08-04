#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWebEngineView>
#include <QMqttClient>
#include <QMqttSubscription>
#include <QMqttMessage>
#include <QTimer>
#include <QDateTime>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QMqttClient *client, QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void cleanup(); // Metoda czyszcząca
    void onMessageReceived(const QMqttMessage &message); // Slot do obsługi wiadomości
    void checkConnectionStatus(); // Slot do sprawdzania statusu połączenia
    void onDisconnected(); // Slot do obsługi rozłączenia

private:
    Ui::MainWindow *ui;
    QWebEngineView *view;
    QMqttClient *mqttClient;
    QMqttSubscription *subscription; // Subskrypcja tematu
    QTimer *timer; // Timer do sprawdzania statusu połączenia
    QDateTime lastMessageTime; // Czas ostatniego odebranego sygnału
    static const int connectionTimeout; // Deklaracja zmiennej statycznej
};

#endif // MAINWINDOW_H
