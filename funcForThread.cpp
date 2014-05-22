#include "funcForThread.h"
#include <iostream>
#include <vector>
#include <QBuffer>
#include <QByteArray>
#include <string>
#include <cstdio>

void* localIMGProducer(void *ptr)
{
    //struct ThreadParam *params = (struct ThreadParam*)ptr;
    IMGWidget *localWidget = (IMGWidget*)ptr;

    //struct node *paramPtr = (struct node *)ptr;
    //IMGWidget *localWidget = paramPtr->ptr1;
    //IMGWidget *remoteWidget = paramPtr->ptr2;

    CvCapture *capture = cvCreateCameraCapture(0);

    int sockfd;
    struct sockaddr_in remoteAddr;

    sockfd = socket(PF_INET, SOCK_DGRAM, 0);

    bzero(&remoteAddr, sizeof (remoteAddr));
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_port = htons(REMOTE_PORT);
    remoteAddr.sin_addr.s_addr = inet_addr(REMOTE_ADDR);

    int frameTimeStamp = 0;
    while (1)
    {
        usleep(FPS * 1000);
        IplImage *localFrame = cvQueryFrame(capture);
        if (localFrame == NULL)  continue;

        ++frameTimeStamp;
        cvCvtColor(localFrame, localFrame, CV_BGR2RGB);
        QImage *curImg = new QImage((unsigned char*)localFrame->imageData,localFrame->width,localFrame->height,localFrame->widthStep,QImage::Format_RGB888);

        localWidget->pushImg(*curImg);

        sendImgToRemote(sockfd, remoteAddr, *curImg, frameTimeStamp);

        delete curImg;
    }
}

void sendImgToRemote(int sockfd, struct sockaddr_in &remoteAddr, QImage &curImg, int frameTimeStamp)
{
    //compress QImage into JPG
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    curImg.save(&buffer, "JPG", IMAGE_QUALITY); // writes image into ba in JPEG format, and quality is 0(smallest compressed file)

    //fragment the imgBuffer and send to the remote endPoint
    //fragment structure: [frameTimeStamp][bufferFragment]
    //frameTimeStamp is an int, and bufferFragment should be equal or less than fragmentLen
    int startPos = 0;
    const int fragmentLen = 512;
    char ts[4];//timeStamp
    memcpy((char *)ts, (char *)&frameTimeStamp, sizeof (int));
    while (ba.size() > startPos)
    {
        QByteArray sendBuffer;
        sendBuffer.append(ts, sizeof (int));

        if ((startPos + fragmentLen) <= ba.size())
        {
            sendBuffer.append(&(ba.data()[startPos]), fragmentLen);
            startPos += fragmentLen;
        }
        else
        {
            sendBuffer.append(&(ba.data()[startPos]), ba.size()-startPos);
            startPos = ba.size();
        }
        //std::cout << "sending frame: " << frameTimeStamp << std::endl;
        sendto(sockfd, sendBuffer, sendBuffer.size(), 0, (struct sockaddr *)&remoteAddr, sizeof (remoteAddr));
    }
    //sending jpg buffer to the remote endPoint without fragmenting the img
    //sendto(sockfd, ba.data(), ba.size(), 0, (struct sockaddr *)&remoteAddr, sizeof (remoteAddr));
}
/*void* remoteIMGProducer(void *ptr)
{
    //TODO: receive from remote endPoint
    while (1)   usleep(FPS * 1000);
}*/
