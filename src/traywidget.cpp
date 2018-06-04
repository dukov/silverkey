#include "traywidget.h"
#include "sksettings.h"
#include "requester.h"
#include <QApplication>
#include <QSettings>
#include <QCloseEvent>
#include <QLocalSocket>

TrayWidget::TrayWidget(QWidget *parent) : QDialog(parent)
{
    httpClient = new Requester(this);

    connect(this, &TrayWidget::dataUpdated, this, &TrayWidget::getAllData);

    createActions();
    createTrayIcon();
    initDbConnection();

    trayIcon->show();

    getAllData();
    setUpServer();
    qDebug() << "Server is listening: " << server->isListening();
}


void TrayWidget::closeEvent(QCloseEvent *event)
{
#ifdef Q_OS_OSX
    if (!event->spontaneous() || !isVisible()) {
        return;
    }
#endif
    if (trayIcon->isVisible()) {
        hide();
        event->ignore();
    }

}


void TrayWidget::quitApp()
{
    qApp->exit();
}

void TrayWidget::showSettings()
{
    SKSettings s;
    int r = s.exec();
}

void TrayWidget::updateCache()
{

}

void TrayWidget::clientConnected()
{
    qDebug() << "Got client connection";

    clientConnection = server->nextPendingConnection();
    connect(clientConnection, &QLocalSocket::disconnected,
            clientConnection, &QLocalSocket::deleteLater);
    connect(clientConnection, &QLocalSocket::readyRead, this, &TrayWidget::readKey);

}

void TrayWidget::readKey()
{
    qDebug() << "Got somethig to read";

    in.setDevice(clientConnection);
    in.setVersion(QDataStream::Qt_5_10);

    QByteArray resp;
    QDataStream out(&resp, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_10);

    QString method;
    in >> method;
    qDebug() << "Requested method: " << method;

    if (method == "get") {
        QString key;
        in >> key;
        qDebug() << "Senfing value: " << dbData[key];
        out << dbData[key];
    } else if (method == "put") {
        out << QString("");
        QString key;
        QString val;
        in >> key;
        in >> val;
        setVal(key, val);
    } else if (method == "reload") {
        initDbConnection();
        getAllData();
    } else if (method == "getall") {
        qDebug() << "Sending all keys: " << dbData.keys().join(" ");
        int dataLen = dbData.keys().length();
        out << dataLen;
        foreach (QString key, dbData.keys()) {
            out << key;
        }
    } else {
        out << QString("");
    }
    clientConnection->write(resp);
    clientConnection->flush();
}

void TrayWidget::setVal(QString key, QString val)
{
    Requester::handleFunc getData = [this](const QJsonObject &o) {
        QString resp = o.value("node").toObject().value("value").toString();
        qDebug() << "Successfully written data"<< resp;
        emit this->dataUpdated();
    };

    Requester::handleFunc errData = [](const QJsonObject &o) {
        qDebug() << "Error writing data";
    };
    QString path;
    if (key[0] != '/') {
        path = "v2/keys/" + key;
    } else {
        path = "v2/keys" + key;
    }

    QByteArray encodedVal = QUrl::toPercentEncoding(val);
    httpClient->sendRequest(path,
                            getData,
                            errData,
                            Requester::Type::PUT,
                            "value=" + encodedVal);
}

void TrayWidget::createTrayIcon()
{

    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(settingsAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);

    QIcon icon = QIcon(":/images/if_edit_add_7710.png");
    trayIcon->setIcon(icon);
    setWindowIcon(icon);
}

void TrayWidget::createActions()
{
    settingsAction = new QAction(tr("&Settings"), this);
    quitAction = new QAction(tr("&Quit"), this);
    connect(settingsAction, &QAction::triggered, this, &TrayWidget::showSettings);
    connect(quitAction, &QAction::triggered, this, &TrayWidget::quitApp);
}

void TrayWidget::setUpServer()
{
    server = new QLocalServer();
    QLocalServer::removeServer("SKTrayApp");
    qDebug() << "Server started listening: " << server->listen("SKTrayApp");
    qDebug() << "Listen address is: " << server->fullServerName();
    connect(server, &QLocalServer::newConnection, this, &TrayWidget::clientConnected);

}

void TrayWidget::initDbConnection()
{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    qDebug() << settings.fileName();
    httpClient->initRequester(settings.value("server", "nseha.linkpc.net").toString(),
                              settings.value("port", 22379).toInt(),
                              nullptr);
}

void TrayWidget::getAllData()
{
    Requester::handleFunc getData = [this](const QJsonObject &o){
        //this->wordlist = MainWindow::getKeys(o.value("node").toObject());
        dbData = TrayWidget::fillDbData(o.value("node").toObject());
        qDebug() << "Got keys" << dbData.keys().join(" ");
        //qDebug() << "Got values" << dbData.values().join(" ");
    };
    Requester::handleFunc errData = [this](const QJsonObject &o){
        qDebug() << "Got err obj";
    };

    httpClient->sendRequest(
                "v2/keys/?recursive=true&sorted=true",
                getData,
                errData);
}

QHash<QString, QString> TrayWidget::fillDbData(const QJsonObject &o)
{
    QHash<QString, QString> res;
    if (o.value("dir") == QJsonValue::Undefined) {
        res[o.value("key").toString()] = o.value("value").toString();
    } else {
        QJsonArray nodes = o.value("nodes").toArray();
        foreach(const QJsonValue &n, nodes) {
            res.unite(fillDbData(n.toObject()));
        }
    }
    return res;
}

void TrayWidget::sendData(const QString &data)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_10);
    out << qint32(data.size());
    out << data;

    QLocalSocket *clientConnection = server->nextPendingConnection();
    connect(clientConnection, &QLocalSocket::disconnected,
            clientConnection, &QLocalSocket::deleteLater);

    clientConnection->write(block);
    clientConnection->flush();
    clientConnection->disconnectFromServer();
}


TrayConnectionProbe::TrayConnectionProbe(QObject *parent) : QObject(parent)
{
    probeSocket = new QLocalSocket();
    connect(probeSocket, &QLocalSocket::connected, this, &TrayConnectionProbe::successProbe);
    connect(probeSocket, SIGNAL(error(QLocalSocket::LocalSocketError)), this, SLOT(errorProbe(QLocalSocket::LocalSocketError)));

}

void TrayConnectionProbe::successProbe()
{
    serverStarted = true;
    emit probeFinished();
}

void TrayConnectionProbe::errorProbe(QLocalSocket::LocalSocketError socketError)
{
    emit probeFinished();
}

bool TrayConnectionProbe::isServerStarted()
{
    return serverStarted;
}

void TrayConnectionProbe::doProbe()
{
    probeSocket->connectToServer("SKTrayApp");
}
