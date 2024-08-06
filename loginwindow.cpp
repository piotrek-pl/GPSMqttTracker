#include "loginwindow.h"
#include "mainwindow.h"

const QString HOSTNAME = "51.20.193.191"; // Ustawienie hostname na sztywno
const int PORT = 1883; // Ustawienie portu na sztywno

LoginWindow::LoginWindow(QWidget *parent)
    : QWidget(parent)
    , mqttClient(new QMqttClient(this))
    , mainWindow(nullptr)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    usernameLineEdit = new QLineEdit(this);
    usernameLineEdit->setPlaceholderText("Username");
    layout->addWidget(usernameLineEdit);

    passwordLineEdit = new QLineEdit(this);
    passwordLineEdit->setPlaceholderText("Password");
    passwordLineEdit->setEchoMode(QLineEdit::Password);
    layout->addWidget(passwordLineEdit);

    connectButton = new QPushButton("Connect", this);
    layout->addWidget(connectButton);

    connect(connectButton, &QPushButton::clicked, this, &LoginWindow::onConnectButtonClicked);

    // Ustawienie focusu na usernameLineEdit przy starcie
    usernameLineEdit->setFocus();

    // Ustawienie layoutu i domyślnego przycisku
    this->setLayout(layout);
    this->setFocusPolicy(Qt::StrongFocus);
    this->setFocus();

    // Podłączenie sygnałów
    connect(mqttClient, &QMqttClient::connected, this, &LoginWindow::onConnected);
    connect(mqttClient, &QMqttClient::disconnected, this, &LoginWindow::onDisconnected);
}

LoginWindow::~LoginWindow() {
    delete mqttClient;
}

void LoginWindow::onConnectButtonClicked() {
    //username = usernameLineEdit->text();
    //password = passwordLineEdit->text();
    mqttUsername = "sub1";
    mqttPassword = "a4Yg3u8W";

    mqttClient->setHostname(HOSTNAME); // Stały hostname
    mqttClient->setPort(PORT);
    mqttClient->setUsername(mqttUsername);
    mqttClient->setPassword(mqttPassword);

    mqttClient->connectToHost();
}

void LoginWindow::onConnected() {
    // Przekaż dane logowania do MainWindow
    mainWindow = new MainWindow(mqttClient, mqttUsername, mqttPassword);
    mainWindow->show();
    this->close();
}

void LoginWindow::onDisconnected() {
    QMessageBox::critical(this, "Error", "Failed to connect to the broker.");
}

bool LoginWindow::event(QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            onConnectButtonClicked();
            return true;
        }
    }
    return QWidget::event(event);
}
