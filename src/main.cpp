#include "mainwindow.h"
#include "traywidget.h"
#include "robothelper.h"
#include <iostream>
#include <QApplication>

#ifdef Q_OS_MACOS
# include <unistd.h>
# include <sys/wait.h>
#endif

#include <Robot.h>
#include <QDebug>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>
#include <QClipboard>
#include <QMessageBox>
#include <QLocalSocket>
#include <QIODevice>
ROBOT_NS_USE_ALL;


void show_window(int argc, char *argv[], int fd = 0, bool child = false) {
    qDebug() << "Creating QApplication";
    QApplication a(argc, argv);
    qDebug() << "Creating Main Window";
    std::unique_ptr<MainWindow> wPtr;
    std::unique_ptr<TrayWidget> twPtr;

    QLocalSocket *p = new QLocalSocket();
    p->connectToServer("SKTrayApp", QIODevice::WriteOnly);
    bool connected = p->waitForConnected(1000);
    qDebug() << "Connected to server " << connected;
    if (!connected) {
        qDebug() << "Connected";
        twPtr.reset(new TrayWidget());
    } else {

        wPtr.reset(new MainWindow());

    if (child) {
        wPtr->setWriteFd(fd);
    }

    wPtr->setAttribute(Qt::WA_DeleteOnClose);

    wPtr->show();

    wPtr->raise();  // for MacOS
    wPtr->activateWindow(); // for Windows
    }

    qDebug() << "Setting qApp event loop";
    qDebug() << a.exec();
}

#ifdef SK_UI_FORK
void type_text(std::string dbVal) {

    qDebug() << "end of application workflow";

    if (dbVal.length()) {
        qDebug() << dbVal.c_str();

        Keyboard keyboard;

        while (!keyboard.GetState(KeySystem)) {
            keyboard.Press(KeySystem);
            qDebug() << "Command key state " << keyboard.GetState(KeySystem);
        }
        keyboard.Click("v");
        keyboard.Release(KeySystem);
    }
}
#endif

#ifdef SK_UI_FORK
void main_with_fork(int argc, char *argv[]) {
    int fd[2];
    pipe(fd);
    pid_t pid = fork();

    if (pid == 0) {
        close(fd[0]);
        show_window(argc, argv, fd[1], true);
        qDebug() << "Stopped qApp loop";
        close(fd[1]);
    } else {
        close(fd[1]);
        int status;
        wait(&status);
        qDebug() << "Child status: " << status;

        std::string dbVal;
        char ch;
        while (read(fd[0], &ch, 1) > 0)
        {
            if (ch != 0)
                dbVal.push_back(ch);

        }
        type_text(dbVal);
        close(fd[0]);
    }
}
#endif // SK_UI_FORK



int main(int argc, char *argv[])
{
#ifdef SK_UI_FORK
    main_with_fork(argc, argv);
#else
    show_window(argc, argv);
#endif // SK_UI_FORK
    return EXIT_SUCCESS;
}
