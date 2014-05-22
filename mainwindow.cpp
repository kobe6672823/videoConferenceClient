#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QHBoxLayout>
#include <iostream>
#include "funcForThread.h"
#include <pthread.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->resize(MAINWINDOW_WIDTH, MAINWINDOW_HEIGHT);

    QHBoxLayout *layOutForWidget = new QHBoxLayout(ui->centralWidget);
    localWidget = new IMGWidget(this);
    remoteWidget = new IMGWidget(this);
    localWidget->resize(WIDGET_WIDTH, WIDGET_HEIGHT);
    remoteWidget->resize(WIDGET_WIDTH, WIDGET_HEIGHT);
    layOutForWidget->addWidget(localWidget);
    layOutForWidget->addWidget(remoteWidget);

    receiver = new QUdpSocket(this);
    //receiver->bind(RECEIVING_PORT, QUdpSocket::ShareAddress);
    receiver->bind(RECEIVING_PORT);
    connect(receiver, SIGNAL(readyRead()), this, SLOT(processPendingDatagram()));
    preFrameTimeStamp = -1;

    //create thread1(localIMGProducer): loop for capturing frame and translating it into localImageQueue, and send the frame to remote endpoint
    //struct ThreadParam localParam, remoteParam;

    //localParam.imageQueuePtr = localWidget->getIMGQueuePtr();
    //localParam.mutexLockPtr = localWidget->getQueueLockPtr();

    //struct node *param = new struct node;
    //param->ptr1 = localWidget;
    //param->ptr2 = remoteWidget;
    int ret = pthread_create(&localProducerId, NULL, localIMGProducer, localWidget);
    if (ret)
        std::cout << "Create pthread error!" << std::endl;

    //create thread2(remoteIMGProducer): loop for receiving frame from remote endPoint and insert it into remote queue
    //remoteParam.imageQueuePtr = remoteWidget->getIMGQueuePtr();
    //remoteParam.mutexLockPtr = remoteWidget->getQueueLockPtr();
    /*ret = pthread_create(&remoteProducerId, NULL, remoteIMGProducer, remoteWidget);
    if (ret)
        std::cout << "Create pthread error!" << std::endl;*/
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::processPendingDatagram()
{
    qint64 receivingSize = receiver->pendingDatagramSize();

    if (receivingSize < sizeof (int)*2)
    {
        QByteArray buffer;
        buffer.resize(receivingSize);
        receiver->readDatagram((char *)buffer.data(), receivingSize); //though the data is too short, still need to read it out of the pending buffer
                                                                      //otherwise, the "readyRead" signal won't be emitted again

        int curFrameTimeStamp;
        memcpy((char *)&curFrameTimeStamp, (char *)buffer.data(), sizeof (int));
        //std::cout << receivingSize << " :small:" << curFrameTimeStamp << std::endl;
        return;
    }

    QByteArray buffer;
    buffer.resize(receivingSize);
    receiver->readDatagram((char *)buffer.data(), receivingSize);

    int curFrameTimeStamp;
    memcpy((char *)&curFrameTimeStamp, (char *)buffer.data(), sizeof (int));

    if (curFrameTimeStamp == preFrameTimeStamp) //append receiving data to frameBuffer
        receivingFrameBuffer.append(&(buffer.data()[4]), (buffer.size()-sizeof (int)));
    else    //flush buffer into an img, and push the img into queue
    {
        preFrameTimeStamp = curFrameTimeStamp;
        QImage *curImg = new QImage();
        bool ret = curImg->loadFromData((uchar *)receivingFrameBuffer.data(), receivingFrameBuffer.size(), "JGP");
        receivingFrameBuffer.clear();//clear the buffer
        receivingFrameBuffer.append(&(buffer.data()[4]), (buffer.size()-sizeof (int)));

        if (ret == false)
        {
            std::cout << "fail to load!" << std::endl;
            delete curImg;
            return;
        }
        remoteWidget->pushImg(*curImg);
        delete curImg;
    }
}
