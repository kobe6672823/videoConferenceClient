#include "rawSocket.h"

void rawSendSockInit(int& skfd, char **buf, struct sockaddr_ll& toaddr)
{
    unsigned int label_header;
    struct ether_header *eth;
    struct ifreq ifr;

    //为buf分配空间
    *buf = (char *)malloc(1024);
    unsigned char src_mac[ETH_ALEN]={0};
    unsigned char dst_mac[ETH_ALEN]={DST_MAC_ADDR}; //对端IP地址，后面需要改成一个固定的组播地址

    if(0>(skfd=socket(PF_PACKET,SOCK_RAW,htons(ETH_P_ALL)))){
        perror("Create Error");
        exit(1);
    }

    bzero(&toaddr,sizeof(toaddr));
    bzero(&ifr,sizeof(ifr));
    strcpy(ifr.ifr_name,NETWORK_CARD_NAME);

    //获取接口索引
    if(-1 == ioctl(skfd,SIOCGIFINDEX,&ifr)){
       perror("get dev index error:");
       exit(1);
    }
    toaddr.sll_ifindex = ifr.ifr_ifindex;
    toaddr.sll_family = PF_PACKET;

    //获取接口的MAC地址
    if(-1 == ioctl(skfd,SIOCGIFHWADDR,&ifr)){
       perror("get dev MAC addr error:");
       exit(1);
    }

    memcpy(src_mac,ifr.ifr_hwaddr.sa_data,ETH_ALEN);

    //开始填充，构造以太头部
    eth=(struct ether_header*)(*buf);
    memcpy(eth->ether_dhost,dst_mac,ETH_ALEN);
    memcpy(eth->ether_shost,src_mac,ETH_ALEN);
    eth->ether_type = htons(ETHER_TYPE_FOR_RTP);

    //构造MPLS包头
    char tmp_str[100];
    FILE *fp = fopen("phoneNumber.txt", "r");
    fscanf(fp, "%s", tmp_str);
    label_header = atoi(&tmp_str[4]);
    fclose(fp);

    label_header = label_header << 12;//标签号
    label_header |= 0x00000111;//TTL
    label_header = htonl(label_header);
    memcpy((*buf)+sizeof(ether_header), &label_header, sizeof(unsigned int));

    /*//将video数据复制到buf中，MPLS包头后
    memcpy(buf+sizeof (ether_header)+sizeof (label_header), buffer, length);

    int n = sendto(skfd,buf,sizeof (ether_header)+sizeof(label_header)+length,0,(struct sockaddr*)&toaddr,sizeof(toaddr));

    close(skfd);
    //printf("raw send: %d\n", n);

    free(buf);*/
}

int rawSockSend(const int skfd, char *buf, struct sockaddr_ll& toaddr, const void *vidoeBuffer, int length)
{
    //将video数据复制到buf中，MPLS包头后
    memcpy(buf+sizeof (ether_header)+sizeof (unsigned int), vidoeBuffer, length);

    int n = sendto(skfd,buf,sizeof (ether_header)+sizeof(unsigned int)+length,0,(struct sockaddr*)&toaddr,sizeof(toaddr));
    return n;
}
