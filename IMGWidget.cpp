#include "IMGWidget.h"
#include <QtGui/QPainter>
#include <QtCore/QPoint>
#include <iostream>
IMGWidget::IMGWidget(QWidget *parent): QWidget(parent)
{
    timerForPaintIMG = new QTimer(this);
    timerForPaintIMG->setInterval(TIMER_FOR_GET_IMG);
    connect(timerForPaintIMG, SIGNAL(timeout()), this, SLOT(paintNextIMG()));
    timerForPaintIMG->start();
}

IMGWidget::~IMGWidget()
{
    delete timerForPaintIMG;
}

void IMGWidget::paintEvent(QPaintEvent *e)
{
    QImage img;
    bool shouldPaint = false;

    IMGQueueLock.lock();
    if (!IMGQueue.empty())
    {
        img = IMGQueue.back();
        shouldPaint = true;
        IMGQueue.pop();
    }
    IMGQueueLock.unlock();


    if (shouldPaint == false)   return;
    QPainter *pp = new QPainter(this);
    pp->drawImage(0, 0, img);
    delete pp;
}

void IMGWidget::paintNextIMG()
{
    this->update();
}

void IMGWidget::pushImg(QImage &curImg)
{
    IMGQueueLock.lock();
    //std::cout << IMGQueue.size() << std::endl;
    IMGQueue.push(curImg.scaled(WIDGET_WIDTH, WIDGET_HEIGHT));
    //avoid the queue becoming too large
    if (IMGQueue.size() == IMG_QUEUE_MAX_SIZE)
    {
        IMGQueue.pop();
        IMGQueue.pop();
        IMGQueue.pop();
        IMGQueue.pop();
        IMGQueue.pop();
    }
    IMGQueueLock.unlock();
}
