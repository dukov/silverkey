#ifndef TRAYWIDGET_H
#define TRAYWIDGET_H

#include <QObject>
#include <QDialog>
#include <QMenu>
#include <QSystemTrayIcon>
#include <QTemporaryDir>
#include <QNetworkDiskCache>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMainWindow>

#include "requester.h"

class TrayConnectionProbe : public QObject
{
    Q_OBJECT

public slots:
    void successProbe();
    void errorProbe(QLocalSocket::LocalSocketError socketError);

Q_SIGNALS:
    void probeFinished();

public:
    explicit TrayConnectionProbe(QObject *parent = nullptr);
    bool isServerStarted();
    void doProbe();

private:
    QLocalSocket *probeSocket;
    bool serverStarted = false;
};


class TrayWidget : public QDialog
{
    Q_OBJECT
public:
    explicit TrayWidget(QWidget *parent = nullptr);

signals:

protected:
    void closeEvent(QCloseEvent *event) override;

public slots:
    void quitApp();
    void showSettings();
    void updateCache();
    void clientConnected();
    void readKey();


private:
    void createTrayIcon();
    void createActions();
    void setUpServer();
    void initDbConnection();
    void getAllData();
    static QHash<QString, QString> fillDbData(const QJsonObject &o);
    void sendData(const QString &data);

    Requester *httpClient;

    QAction *settingsAction;
    QAction *quitAction;

    QSystemTrayIcon *trayIcon;
    QMenu *trayIconMenu;
    QHash<QString, QString> dbData;
    QLocalServer *server;
    QLocalSocket *clientConnection;
    QDataStream in;


};

#endif // TRAYWIDGET_H
