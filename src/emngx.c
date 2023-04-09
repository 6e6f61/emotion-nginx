/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.

 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/* this license is to stop filthy tech giants switching to PS2 server farms without giving back to
 * the man. thank you for understanding.
 */
#include <stdio.h>
#include <debug.h>
#include <unistd.h>
#include <kernel.h>
#include <string.h>

#include <ps2ip.h>
#include <netman.h>

#include <sifrpc.h>
#include <loadfile.h>
#include <sbv_patches.h>
#include <iopcontrol.h>
#include <iopheap.h>

int  init_tpcip(void);
int  init_server(void);
int  respond(int);
void deinit_tcpip(void);

#define INTERFACE "sm0"

#define multi_log(fmt, ...) do { \
        printf("emotion-nginx: " fmt, ##__VA_ARGS__); \
        scr_printf(fmt, ##__VA_ARGS__); \
    } while (0)
#define print_ip4_addr(l, x) do { \
    struct ip4_addr *t = (struct ip4_addr *)&x; \
    multi_log(l "%d.%d.%d.%d\n", ip4_addr1(t), ip4_addr2(t), ip4_addr3(t), ip4_addr4(t)); \
    } while (0)

extern unsigned char DEV9_irx[];
extern unsigned int size_DEV9_irx;
extern unsigned char SMAP_irx[];
extern unsigned int size_SMAP_irx;
extern unsigned char NETMAN_irx[];
extern unsigned int size_NETMAN_irx;

int main(void)
{
    struct sockaddr_in client_addr;
    int ca_size = sizeof (client_addr);
    int socket, r_ret, listenfd;

    init_scr();
    sleep(2);
    multi_log("emotion-nginx\n");

    multi_log("initialising tcp/ip stack\n");
    init_tpcip();

    multi_log("binding to an interface\n");
    if ((listenfd = init_server()) < 0)
    {
        multi_log("!! couldn't bind to an interface\n");
        goto end;
    }

    multi_log("ready to serve requests on :80\n");
    for (;;)
    {
        if ((socket = accept(listenfd, (struct sockaddr *)&client_addr, &ca_size)) < 0)
            multi_log("!! error accepting request: %d\n", socket);
        char *buffer = inet_ntoa(client_addr.sin_addr);
        multi_log("accepting request from %s (%d)\n", buffer, socket);

        if ((r_ret = respond(socket)) != 0) multi_log("!! couldn't respond: %d\n", r_ret);
        else multi_log("served ok\n");

        close(socket);
    }

end:
    /* oh noes */
    multi_log("deinit-ing tcp/ip stack\n");
    deinit_tcpip();
    multi_log("end of main\n");
    for (;;) sleep(2); 

    return 0;
}

int respond(int socket)
{
    if (writesocket(socket, "HTTP/1.0 200 OK\r\nContent-Length: 78\r\n\r\n", 39) == -1)
    {
        multi_log("!! couldn't write to socket\n");
        return -1;
    }
    if (writesocket(socket,
        "<!DOCTYPE html><html><body>from the ps2 to you: emotion-nginx.</body></html>\r\n", 78) == -1)
    {
        multi_log("!! couldn't write to socket\n");
        return -1;
    }

    return 0;
}

int init_server(void)
{
    struct addrinfo hints, *res, *p;
    int listenfd;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, "80", &hints, &res) != 0)
    {
        multi_log("!! couldn't enumerate available interfaces\n");
        return -1;
    }

    for (p = res; p != NULL; p = p->ai_next)
    {
        listenfd = socket(p->ai_family, p->ai_socktype, 0);
        if (listenfd == -1)
            continue;

        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
            break;
    }

    if (!p)
    {
        multi_log("!! enumerated interfaces but found nothing to bind to\n");
        return -1;
    }

    freeaddrinfo(res);

    return (listen(listenfd, 0x075EC00F) == 0) ? listenfd : -1;
}

int init_tpcip(void)
{
    ip4_addr_t ip_addr, netmask, gateway;
    t_ip_info ip_config;
    const ip_addr_t *dns_addr;
    int e, state;

    SifInitRpc(0);
    while (!SifIopReset("", 0)) { };
    while (!SifIopSync()) { };

    SifInitRpc(0);
    SifLoadFileInit();
    SifInitIopHeap();
    sbv_patch_enable_lmb();

    SifExecModuleBuffer(DEV9_irx, size_DEV9_irx, 0, NULL, NULL);
    SifExecModuleBuffer(NETMAN_irx, size_NETMAN_irx, 0, NULL, NULL);
    SifExecModuleBuffer(SMAP_irx, size_SMAP_irx, 0, NULL, NULL);
    multi_log("sif ready\n");

    if ((e = NetManInit()))
    {
        multi_log("!! failed to initialise netman: %d\n", e);
        return -1;
    } else multi_log("initialised netman ok\n");

    /* unimportant because we're using dhcp */
    ip4_addr_set_zero(&ip_addr);
    ip4_addr_set_zero(&netmask);
    ip4_addr_set_zero(&gateway);

    ps2ipInit(&ip_addr, &netmask, &gateway);
    multi_log("initialised tcp stack\n");

    if ((e = ps2ip_getconfig(INTERFACE, &ip_config)) == 0)
    {
        multi_log("!! couldn't extract %s configuration: %d\n", INTERFACE, e);
        return -1;
    }

    ip_config.dhcp_enabled = 1;
    if ((e = ps2ip_setconfig(&ip_config)) == 0)
    {
        multi_log("!! couldn't apply network configuration: %d\n", e);
        return -1;
    }

    multi_log("enabled dhcp\n");

    while ((state = NetManIoctl(NETMAN_NETIF_IOCTL_GET_LINK_STATUS, NULL, 0, NULL, 0))
        != NETMAN_NETIF_ETH_LINK_STATE_UP)
    {
        multi_log("waiting for link (%d)\n", state);
        sleep(1);
    }

    while (state != DHCP_STATE_BOUND)
    {
        ps2ip_getconfig(INTERFACE, &ip_config);
        state = ip_config.dhcp_status;
        multi_log("waiting for dhcp lease (%d)\n", state);
        sleep(1);
    }

    dns_addr = dns_getserver(0);
    print_ip4_addr("DNS: ", dns_addr);
    print_ip4_addr("netmask: ", ip_config.netmask);
    print_ip4_addr("gateway: ", ip_config.gw);
    print_ip4_addr("IP address: ", ip_config.ipaddr);
    
    return 0;
}

void deinit_tcpip(void)
{
  ps2ipDeinit();
  NetManDeinit();
}