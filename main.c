
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define NUM_SOCKETS     2

static char *sockets_addr[] =
{
    "10.0.22.255",
    "10.0.22.51",
    "10.0.22.200"
};


static int create_socket(const char *ipaddr)
{
    int sd;
    int one = 1;
    struct sockaddr_in servaddr;

    if ((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("error: socket\n");
        return -1;
    }

    if (setsockopt(sd, SOL_SOCKET, SO_BROADCAST, (const char *)&one, sizeof(one)) < 0)
    {
        printf("error: set broadcast\n");
        return -1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = ipaddr != NULL ? inet_addr(ipaddr) : INADDR_ANY;
    servaddr.sin_port = htons(5060);

    if (bind(sd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr_in)) < 0)
    {
        printf("error: bind %s\n", ipaddr);
        return -1;
    }

    printf("socket '%s'  sd = %d created\n", ipaddr, sd);

    return sd;
}


static int test_receiver(void)
{
    int res, ix;
    struct sockaddr_in cliaddr;
    int sd[NUM_SOCKETS];
    socklen_t socklen = sizeof(struct sockaddr_in);
    fd_set rfds;
    char buf[1024];

    for (ix = 0; ix < NUM_SOCKETS; ix++)
    {
        if ((sd[ix] = create_socket(sockets_addr[ix])) < 0)
        {
            printf("error: create socket %s\n", sockets_addr[ix]);
            return -1;
        }
    }

    while(1)
    {
        FD_ZERO(&rfds);

        for (ix = 0; ix < NUM_SOCKETS; ix++)
            FD_SET(sd[ix], &rfds);

        res = select(sd[NUM_SOCKETS-1] + 1, &rfds, NULL, NULL, NULL);
        if (res > 0)
        {
            for (ix = 0; ix < NUM_SOCKETS; ix++)
            {
                if (FD_ISSET(sd[ix], &rfds))
                {
                    if ((res = recvfrom(sd[ix], buf, sizeof(buf), 0, (struct sockaddr *)&cliaddr, &socklen)) < 0)
                    {
                        printf("error: recvfrom sd = %d\n", sd[ix]);
                        return 1;
                    }
                    else if (res > 0)
                    {
                        printf("sd = %d   Rcv %d bytes from %s  --> ", sd[ix], res, inet_ntoa(cliaddr.sin_addr));
                        buf[res] = 0;
                        printf("%s", buf);
                    }
                }
            }
        }
    }

    return 0;
}


static int test_sender(void)
{
#define TEST_BUFSZ   1000
#define TEST_IPADDR   "10.0.22.100"
    uint8_t buf[TEST_BUFSZ];
    int fd;
    struct sockaddr_in servaddr;

    memset(buf, 0x00, sizeof(buf));

    fd = create_socket(NULL);
    if (fd < 0)
    {
        printf("error: create socket\n");
        return -1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(TEST_IPADDR);
    servaddr.sin_port = htons(5060);

    while(1)
    {
        if (sendto(fd, buf, sizeof(buf), 0, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
        {
            printf("error: send UDP packet\n");
            return -1;
        }

        usleep(10);
    }


    return 0;
}



int main(int argc, char *argv[])
{
    test_sender();
    return 0;
}
