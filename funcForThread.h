#ifndef FUNCFORTHREAD_H
#define FUNCFORTHREAD_H

#include <unistd.h>
#include <QMutex>
#include <queue>
#include <QImage>
#include "opencv2/opencv.hpp"
#include "IMGWidget.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define FPS 30
#define IMAGE_QUALITY 50    //0-100, 0 will obtain a smallest compressed image file
#define REMOTE_ADDR "192.168.1.101"
#define REMOTE_PORT 40000

struct node
{
    IMGWidget *ptr1;
    IMGWidget *ptr2;
};


void* localIMGProducer(void *ptr);    //capture frame from local camera, insert QImage into local queue, and send it to remote endpoint
//void* remoteIMGProducer(void *ptr);   //recvFrom from remote endPoint, insert QImage into remote queue
//void sendToRemote(void *buffer);    //send
void sendImgToRemote(int sockfd, struct sockaddr_in &remoteAddr, QImage &curImg, int frameTimeStamp);    //compress, fragment and send img to remote endPoint
#endif // FUNCFORTHREAD_H
