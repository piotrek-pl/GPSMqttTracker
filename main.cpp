#include <QApplication>
#include "loginwindow.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    LoginWindow loginWindow;
    loginWindow.show();
    return a.exec();
}
