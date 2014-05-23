#ifndef RAWSOCKET_H
#define RAWSOCKET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#include <net/if_arp.h>
#include <netpacket/packet.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#define DST_MAC_ADDR 0x01,0x00,0x5e,0x11,0x11,0x11      //发送语音给对端网卡所用的一个固定的组播地址
//#define DST_MAC_ADDR 0xc8,0x0a,0xa9,0xee,0xea,0xb2
#define NETWORK_CARD_NAME "eth0"
#define ETHER_TYPE_FOR_RTP ETH_P_MPLS_UC        //所用的ether_type字段值，目前先用MPLS，TODO：后面换成自定义的一个值

void rawSendSockInit(int& skfd, char **buf, struct sockaddr_ll& toaddr);
int rawSockSend(const int skfd, char *buf, struct sockaddr_ll& toaddr, const void *buffer, int length);

#endif // RAWSOCKET_H
