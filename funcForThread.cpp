#include "funcForThread.h"
#include <iostream>
#include <vector>
#include <QBuffer>
#include <QByteArray>
#include <string>
#include <cstdio>
#include "rawSocket.h"

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

    //params for raw socket
    int rawSockfd;
    char *rawSockBuff = NULL;
    struct sockaddr_ll toaddr;
    rawSendSockInit(rawSockfd, &rawSockBuff, toaddr);

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
        //send fragment through udp
        //sendto(sockfd, sendBuffer, sendBuffer.size(), 0, (struct sockaddr *)&remoteAddr, sizeof (remoteAddr));

        //send fragment through raw socket
        rawSockSend(rawSockfd, rawSockBuff, toaddr, (const void *)sendBuffer, sendBuffer.size());
    }
    close(rawSockfd);
    free(rawSockBuff);
    //sending jpg buffer to the remote endPoint without fragmenting the img
    //sendto(sockfd, ba.data(), ba.size(), 0, (struct sockaddr *)&remoteAddr, sizeof (remoteAddr));
}

void* remoteIMGProducer(void *ptr)
{
    IMGWidget *remoteWidget = (IMGWidget*)ptr;
    QByteArray receivingFrameBuffer;
    int preFrameTimeStamp = -1;

    int skfd,n;
    char videoFrame[1024]={0};
    struct ethhdr *eth;
    struct sockaddr_ll fromaddr;
    struct ifreq ifr;

    unsigned char src_mac[ETH_ALEN]={0};

    //只接收发给本机的MPLS报文
    if(0>(skfd=socket(PF_PACKET,SOCK_RAW,htons(ETHER_TYPE_FOR_RTP)))){
        perror("Create Error");
        exit(1);
    }

    bzero(&fromaddr,sizeof(fromaddr));
    bzero(&ifr,sizeof(ifr));
    strcpy(ifr.ifr_name,"eth0");

    //获取接口索引
    if(-1 == ioctl(skfd,SIOCGIFINDEX,&ifr)){
        perror("get dev index error:");
        exit(1);
    }
    fromaddr.sll_ifindex = ifr.ifr_ifindex;
    //printf("interface Index:%d\n",ifr.ifr_ifindex);

    //获取接口的MAC地址
    if(-1 == ioctl(skfd,SIOCGIFHWADDR,&ifr)){
        perror("get dev MAC addr error:");
        exit(1);
    }

    memcpy(src_mac,ifr.ifr_hwaddr.sa_data,ETH_ALEN);
    //printf("MAC :%02X-%02X-%02X-%02X-%02X-%02X\n",src_mac[0],src_mac[1],src_mac[2],src_mac[3],src_mac[4],src_mac[5]);

    fromaddr.sll_family = PF_PACKET;
    fromaddr.sll_protocol=htons(ETH_P_MPLS_UC);
    fromaddr.sll_hatype=ARPHRD_ETHER;
    fromaddr.sll_pkttype=PACKET_HOST;
    fromaddr.sll_halen=ETH_ALEN;
    memcpy(fromaddr.sll_addr,src_mac,ETH_ALEN);

    bind(skfd,(struct sockaddr*)&fromaddr,sizeof(struct sockaddr));

    //读入本机电话号码
    unsigned int localPhoneNumber;
    char tmp_str[100];
    FILE *fp = fopen("phoneNumber.txt", "r");
    fscanf(fp, "%s", tmp_str);
    localPhoneNumber = atoi(&tmp_str[4]);
    fclose(fp);
    //localPhoneNumber = 200;
    //printf("pay close attention!!!!!!!!!!!!!!!!!!local phoneNumber: %u\n", localPhoneNumber);


    while(1){
        memset(videoFrame, 0, 1024);
        n = recvfrom(skfd, videoFrame, 1024, 0, NULL, NULL);
        eth=(struct ethhdr*)videoFrame;

        if(ntohs(eth->h_proto)==ETH_P_MPLS_UC){
            //获取以太网帧中label号
            unsigned int currentLabel;
            memcpy(&currentLabel, videoFrame+14, sizeof(unsigned int));
            currentLabel = ntohl(currentLabel);
            currentLabel = currentLabel >> 12;
            //不是发往这台主机的数据帧不应该做处理
            if (currentLabel != localPhoneNumber || n == 190) // n == 190 means that the frame is an audio frame
                continue;

            //restore the frame into QtImage
            restoreVideoFrame(remoteWidget, videoFrame+18, n-18, preFrameTimeStamp, receivingFrameBuffer);
        }
    }
    close(skfd);
}

void restoreVideoFrame(IMGWidget *remoteWidget, char *videoFrame, int len, int &preFrameTimeStamp, QByteArray &receivingFrameBuffer)
{
    if (len < sizeof (int)*2)
        return;

    int curFrameTimeStamp;
    memcpy((char *)&curFrameTimeStamp, videoFrame, sizeof (int));

    if (curFrameTimeStamp == preFrameTimeStamp) //append receiving data to frameBuffer
        receivingFrameBuffer.append(&(videoFrame[4]), (len-sizeof (int)));
    else    //flush buffer into an img, and push the img into queue
    {
        preFrameTimeStamp = curFrameTimeStamp;
        QImage *curImg = new QImage();
        bool ret = curImg->loadFromData((uchar *)receivingFrameBuffer.data(), receivingFrameBuffer.size(), "JGP");
        receivingFrameBuffer.clear();//clear the buffer
        receivingFrameBuffer.append(&(videoFrame[4]), (len-sizeof (int)));

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
