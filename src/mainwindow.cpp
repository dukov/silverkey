#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "fuzzycompleter.h"
#include "sksettings.h"
#include "hotkeys.h"
#include <Robot.h>
#ifdef Q_OS_MACOS
# include <unistd.h>
# include <sys/wait.h>
#endif

#include <QKeyEvent>
#include <QDebug>
#include <QAbstractItemView>
#include <QMenu>
#include <QSettings>
#include <QDesktopWidget>
#include <QFuture>
#include <QFutureWatcher>
#include <QMessageBox>
#include <QPushButton>
#include <QClipboard>
#include <QMimeData>
#include <QSizePolicy>
ROBOT_NS_USE_ALL;


MainWindow::MainWindow(QWidget *parent) :
    QDialog(parent)
{
    clientSocket = new QLocalSocket();
    clientSocket->connectToServer("SKTrayApp");
    in.setDevice(clientSocket);
    in.setVersion(QDataStream::Qt_5_10);

    setObjectName("skDialog");

    //setStyleSheet("#skDialog {background:transparent;}");
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::FramelessWindowHint);

    createSettingsButton();
    createLineEdit();
    createAddButton();
    createTextEdit();

    activateWindow();

    QFocusEvent* eventFocus = new QFocusEvent(QEvent::FocusIn);
    qApp->postEvent(this, (QEvent *)eventFocus, Qt::LowEventPriority);

    QPoint pos(lineEdit->width()-5, 5);
    QMouseEvent e(QEvent::MouseButtonPress, pos, Qt::LeftButton, Qt::LeftButton, 0);
    qApp->sendEvent(lineEdit, &e);
    QMouseEvent f(QEvent::MouseButtonRelease, pos, Qt::LeftButton, Qt::LeftButton, 0);
    qApp->sendEvent(lineEdit, &f);

    QWidget::setFocusProxy(this);

    QPoint globalCursorPos = QCursor::pos();
    int mouseScreen = qApp->desktop()->screenNumber(globalCursorPos);
    qDebug() << "Screen " << mouseScreen;
    QRect ag = qApp->desktop()->screen(mouseScreen)->geometry();
    setGeometry(
        QStyle::alignedRect(
            Qt::LeftToRight,
            Qt::AlignCenter,
            size(),
            ag
        )
    );

    int w = settingsButton->width() +
            widgetPadding +
            lineEdit->width() +
            widgetPadding +
            addDataButton->width();
    resize(w,lineEdit->height());

    getDbData();
    registerService();
}

QStringList MainWindow::getKeys(const QJsonObject &o) {
    QStringList res;
    if (o.value("dir") == QJsonValue::Undefined) {
        res.append(o.value("key").toString());
    } else {
        QJsonArray nodes = o.value("nodes").toArray();
        foreach(const QJsonValue &n, nodes) {
            res += getKeys(n.toObject());
        }
    }
    return res;
}

void MainWindow::handleDataLoad() {
    if (!lineEdit->completer()->isDataSet()) {
        //QMessageBox::StandardButton reply;
        int reply = QMessageBox::question(this,
                                      "Error", "Failed to load data from database",
                                      "Quit", "Open Settings");
        qDebug() << "Reply: " << reply;
        if (reply == 0) {
            lineEdit->setSelectedItem("");
            doHide();
        } else {
            showSettings();
        }
    } else {
        unlockInput();
    }

}

void MainWindow::doHide()
{
    qDebug() << "Hiding window";
    this->hide();
}

void MainWindow::createSettingsButton()
{
    QSizePolicy *p = new QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    settingsButton = new QPushButton("",this);
    settingsButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    settingsButton->setObjectName("settings");
    settingsButton->setStyleSheet("#settings {"
                                  "border-image:url(:/images/if_cog_35873.png);"
                                  "}"
                                  "#settings:pressed {"
                                  "border-image:url(:/images/if_cog_35873_pressed.png);"
                                  "}");
    settingsButton->resize(QImage(":/images/if_cog_35873.png").size());
    settingsButton->setGeometry(
                QStyle::alignedRect(
                    Qt::LeftToRight,
                    Qt::AlignTop|Qt::AlignLeft,
                    settingsButton->size(),
                    this->geometry()
                ));
    connect(settingsButton, &QPushButton::clicked, this, &MainWindow::showSettings);
}

void MainWindow::createLineEdit()
{
    lineEdit = new FuzzyLineEdit(this);
    lineEdit->setObjectName("skInput");
    lineEdit->setFocusPolicy(Qt::StrongFocus);
    lineEdit->setFocus();
    lineEdit->setGeometry(0, 0, 500, 50);
    lineEdit->setStyleSheet(
                  "#skInput {"
                    "background-color: #f6f6f6;"
                    "border-radius: 10px;"
                    "font: 30pt Courier"
                  "}"
                );
    lineEdit->setTextMargins(5, 0, 0, 0);
    lineEdit->setAttribute(Qt::WA_MacShowFocusRect, false);

    lineEdit->move(settingsButton->x() + settingsButton->width() + widgetPadding,
                   settingsButton->y());

    FuzzyCompleter *completer = new FuzzyCompleter(this);
    FuzzyPopup *popup = new FuzzyPopup();
    popup->setObjectName("skPopup");
    popup->setEditTriggers(QAbstractItemView::NoEditTriggers);
    popup->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    popup->setSelectionBehavior(QAbstractItemView::SelectRows);
    popup->setSelectionMode(QAbstractItemView::SingleSelection);
    completer->setPopup(popup);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->popup()->setStyleSheet("#skPopup {"
                                        "background-color: #f6f6f6;"
                                        "font: 20pt Courier"
                                      "}");
    lineEdit->setCompleter(completer);
    QAbstractItemView *abstractItemView = lineEdit->completer()->popup();


    connect(lineEdit, &QLineEdit::returnPressed, this, &MainWindow::enterPressed);
    connect(abstractItemView, &QAbstractItemView::clicked, this, &MainWindow::enterPressed);
    connect(popup, &FuzzyPopup::popupShow, this, &MainWindow::setAngleCorners);
    connect(popup, &FuzzyPopup::popupHide, this, &MainWindow::setRoundedCorners);

    connect(lineEdit, &QLineEdit::textEdited, this, &MainWindow::searchEvent);
    connect(lineEdit, &FuzzyLineEdit::hideApp, this, &MainWindow::escapePressed);
}

void MainWindow::createAddButton()
{
    addDataButton = new QPushButton("", this);
    addDataButton->setObjectName("addData");
    addDataButton->setStyleSheet("#addData {"
                                 "border-image:url(:/images/if_edit_add_7710.png);"
                                 "}"
                                 "#addData:pressed {"
                                 "border-image:url(:/images/if_edit_add_7710_pressed.png);"
                                 "}");
    addDataButton->resize(QImage(":/images/if_edit_add_7710.png").size());

    int x = lineEdit->x() +
            lineEdit->width() +
            widgetPadding;
    addDataButton->move(x, settingsButton->y());

    connect(addDataButton, &QPushButton::clicked, this, &MainWindow::showTextEdit);

}

void MainWindow::createTextEdit()
{
    clipboardData = new QTextEdit(this);
    clipboardData->setObjectName("clipboardData");
    int x = lineEdit->x() +
            lineEdit->width() +
            widgetPadding;
    clipboardData->setGeometry(x, settingsButton->y(), 500, 300);
    clipboardData->setStyleSheet(
                  "#clipboardData {"
                    "background-color: #f6f6f6;"
                    "border-radius: 10px;"
                    "font: 20pt Courier"
                  "}"
                );
    clipboardData->hide();
}

void MainWindow::getVal(QString key) {

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_10);
    out << QString("get");
    out << key;
    clientSocket->write(block);
    clientSocket->flush();
    if (clientSocket->waitForReadyRead()) {
        qDebug() << "Got reply from server";
        in >> data;
        doHide();
    }
}

void MainWindow::setVal(QString key, QString val) {
    qDebug() << "Setting value to DB" << val;
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_10);
    out << QString("put");
    out << key;
    out << val;
    clientSocket->write(block);
    clientSocket->flush();
    doHide();
}

void MainWindow::getDbData()
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_10);
    out << QString("getall");
    clientSocket->write(block);
    clientSocket->flush();
    if (clientSocket->waitForReadyRead()) {
        wordlist.clear();
        int dataLen = 0;
        QString key;
        in >> dataLen;
        for (int i=1; i<dataLen; ++i) {
            in >> key;
            wordlist << key;
        }
        qDebug() << "Got all data from server: " << wordlist.join(" ");
        FuzzyCompleter *c = lineEdit->completer();
        c->cleanUp();
        c->setUp(wordlist);
    }
    handleDataLoad();
}

void MainWindow::lockInput()
{
    // TODO (dukov) Use gray style here
    lineEdit->setReadOnly(true);
    lineEdit->setText("Loading data...");
}

void MainWindow::unlockInput()
{
    // TODO (dukov) Restore default style
    lineEdit->setText("");
    lineEdit->setReadOnly(false);

}

void MainWindow::setWriteFd(int fd){
    wfd = fd;
}

void MainWindow::setData(QString d) {
    data = d;
}

void MainWindow::hideEvent(QHideEvent *e) {
    if (clipboardData->toPlainText() == "") {
        qDebug() << "Hide action, value is " << data;
#ifdef Q_OS_MACOS
        write(wfd, data.toStdString().c_str(), data.toStdString().length()+1);
#endif
        if (data != "") {

            QClipboard *cb = QApplication::clipboard();
            cb->setText(data);

#ifdef Q_OS_LINUX
            cb->setText(data, QClipboard::Selection);
            qDebug() << "Selection CB data" << cb->text(QClipboard::Selection);
#endif

#ifndef SK_UI_FORK
            Keyboard keyboard;

            while (!keyboard.GetState(KeyShift)) {
                keyboard.Press(KeyShift);
                qDebug() << "Command key state " << keyboard.GetState(KeyShift);
            }
            keyboard.Click("{INSERT}");
            keyboard.Release(KeyShift);
#endif
        }
    }


    e->accept();
    qApp->closeAllWindows();
    qApp->exit();
}

void MainWindow::escapePressed() {
    clipboardData->setText("");
    lineEdit->setSelectedItem("");
    doHide();
}

void MainWindow::enterPressed() {
    qDebug() << "Enter Pressed";

    if (clipboardData->toPlainText() != "") {
        QString key = lineEdit->text();
        setVal(key, clipboardData->toPlainText());
    } else {
        QString key = lineEdit->getSelectedItem();
        getVal(key);
    }

}

void MainWindow::searchEvent() {
    FuzzyCompleter *c = lineEdit->completer();
    c->update(lineEdit->text());
    c->popup()->setCurrentIndex(c->popup()->model()->index(0,0));
}

void MainWindow::showSettings() {
    SKSettings s;
    int r = s.exec();
    qDebug() << "Settings result: " << r << QDialog::Accepted;
    if (r == QDialog::Accepted) {
        QByteArray block;
        QDataStream out(&block, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_5_10);
        out << QString("reload");
        clientSocket->write(block);
        clientSocket->flush();
        if (clientSocket->waitForReadyRead()) {
            wordlist.clear();
            int dataLen = 0;
            QString key;
            in >> dataLen;
            for (int i=1; i<=dataLen; ++i) {
                in >> key;
                wordlist.push_back(key);
            }
            qDebug() << "Got all data from server: " << wordlist.join(" ");
            FuzzyCompleter *c = lineEdit->completer();
            c->cleanUp();
            c->setUp(wordlist);
        }
        handleDataLoad();


        //this->lockInput();
        //lineEdit->completer()->cleanUp();
        //this->getDbData();
    }
}

void MainWindow::showTextEdit() {
    addDataButton->hide();
    clipboardData->show();
    int w = settingsButton->width() +
            widgetPadding +
            lineEdit->width() +
            widgetPadding +
            clipboardData->width();
    resize(w,clipboardData->height());

    const QClipboard *cb = QApplication::clipboard();
    const QMimeData *md = cb->mimeData();
    if (md->hasText()) {
        clipboardData->setText(md->text());
    }

}

void MainWindow::setAngleCorners() {
    // TODO(dukov) get rid of this in favor of dynamic styles
    lineEdit->setStyleSheet("#skInput {"
                    "background-color: #f6f6f6;"
                    "border-radius: 10px;"
                    "border-bottom-right-radius: 0;"
                    "border-bottom-left-radius: 0;"
                    "font: 30pt Courier"
                  "}");
}

void MainWindow::setRoundedCorners() {
    // TODO(dukov) get rid of this in favor of dynamic styles
    lineEdit->setStyleSheet("#skInput {"
                    "background-color: #f6f6f6;"
                    "border-radius: 10px;"
                    "font: 30pt Courier"
                  "}");
}

