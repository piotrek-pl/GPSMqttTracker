#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWebEngineView>
#include <QMqttClient>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr); // Deklaracja domyślnego konstruktora
    explicit MainWindow(QMqttClient *client, QWidget *parent = nullptr); // Nowy konstruktor
    ~MainWindow();

private slots:
    void cleanup(); // Metoda czyszcząca

private:
    Ui::MainWindow *ui;
    QWebEngineView *view;
    QMqttClient *mqttClient; // Przechowywanie wskaźnika do QMqttClient
};

#endif // MAINWINDOW_H
