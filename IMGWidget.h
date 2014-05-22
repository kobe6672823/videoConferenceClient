#ifndef IMGWIDGET_H
#define IMGWIDGET_H

#include <QtGui/QWidget>
#include <QtGui/QPaintEvent>
#include <QtGui/QImage>
#include <QtCore/QTimer>
#include <QMutex>
#include <queue>

#define TIMER_FOR_GET_IMG   100 //100ms
#define WIDGET_WIDTH 400
#define WIDGET_HEIGHT 450

#define IMG_QUEUE_MAX_SIZE 15

class IMGWidget:public QWidget
{
Q_OBJECT

    //friend void* localIMGProducer(void *ptr);
    //friend void* remoteIMGProducer(void *ptr);
public:
    IMGWidget(QWidget *parent=0);
    std::queue<QImage>* getIMGQueuePtr()    {return &IMGQueue;}
    QMutex* getQueueLockPtr()   {return &IMGQueueLock;}
    void pushImg(QImage &curImg);
    ~IMGWidget();

protected:
    void paintEvent(QPaintEvent *e);

private slots:
    void paintNextIMG();

private:
    std::queue<QImage> IMGQueue;
    QMutex IMGQueueLock;
    QTimer *timerForPaintIMG;
};
#endif // IMGWIDGET_H
