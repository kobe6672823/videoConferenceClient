#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "IMGWidget.h"
#include <QByteArray>
#include <QtNetwork/QUdpSocket>

#define RECEIVING_PORT 40000

#define MAINWINDOW_WIDTH 900
#define MAINWINDOW_HEIGHT 500

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
        void processPendingDatagram();
private:
    Ui::MainWindow *ui;

    //widget for localImage and remoteImage
    IMGWidget *localWidget, *remoteWidget;

    pthread_t localProducerId, remoteProducerId;

    QUdpSocket *receiver;
    int preFrameTimeStamp;
    QByteArray receivingFrameBuffer;
};

#endif // MAINWINDOW_H
