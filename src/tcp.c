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
#include <ps2ip.h>
#include <netman.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <sbv_patches.h>
#include <iopcontrol.h>
#include <unistd.h>
#include <iopheap.h>
#include <string.h>
#include <debug.h>

#include "tcp.h"

extern unsigned char DEV9_irx[];
extern unsigned int size_DEV9_irx;
extern unsigned char SMAP_irx[];
extern unsigned int size_SMAP_irx;
extern unsigned char NETMAN_irx[];
extern unsigned int size_NETMAN_irx;

void tcp_init_sif()
{
    SifLoadFileInit();
    SifInitIopHeap();
    sbv_patch_enable_lmb();

    SifExecModuleBuffer(DEV9_irx, size_DEV9_irx, 0, NULL, NULL);
    SifExecModuleBuffer(NETMAN_irx, size_NETMAN_irx, 0, NULL, NULL);
    SifExecModuleBuffer(SMAP_irx, size_SMAP_irx, 0, NULL, NULL);
}

int tcp_enable_dhcp(char *interface)
{
    t_ip_info ip_config;
    int r;

    if ((r = ps2ip_getconfig(interface, &ip_config)) == 0)
    {
        scr_printf("!! couldn't extract %s configuration: %d\n", interface, r);
        return -1;
    }

    ip_config.dhcp_enabled = 1;
    if ((r = ps2ip_setconfig(&ip_config)) == 0)
    {
        scr_printf("!! couldn't apply network configuration: %d\n", r);
        return -1;
    }

    while (r != DHCP_STATE_BOUND)
    {
        ps2ip_getconfig(interface, &ip_config);
        r = ip_config.dhcp_status;
        scr_printf("waiting for dhcp lease (%d)\n", r);
        sleep(1);
    }

    return 0;
}

int tcp_up_link(char *interface)
{
    ip4_addr_t ip_addr, netmask, gateway;
    t_ip_info ip_config;
    const ip_addr_t *dns_addr;
    int e, state;

    if ((e = NetManInit()))
    {
        scr_printf("!! failed to initialise netman: %d\n", e);
        return -1;
    } else scr_printf("initialised netman ok\n");

    /* unimportant because we're using dhcp */
    ip4_addr_set_zero(&ip_addr);
    ip4_addr_set_zero(&netmask);
    ip4_addr_set_zero(&gateway);

    ps2ipInit(&ip_addr, &netmask, &gateway);
    scr_printf("initialised tcp stack\n");

    while ((state = NetManIoctl(NETMAN_NETIF_IOCTL_GET_LINK_STATUS, NULL, 0, NULL, 0))
        != NETMAN_NETIF_ETH_LINK_STATE_UP)
    {
        scr_printf("waiting for link (%d)\n", state);
        sleep(1);
    }

    tcp_enable_dhcp(interface);
    scr_printf("enabled dhcp\n");

    dns_addr = dns_getserver(0);
    ps2ip_getconfig(interface, &ip_config);;
    print_ip4_addr("DNS: ", dns_addr);
    print_ip4_addr("netmask: ", ip_config.netmask);
    print_ip4_addr("gateway: ", ip_config.gw);
    print_ip4_addr("IP address: ", ip_config.ipaddr);
    
    return 0;
}

void tcp_deinit(void)
{
  ps2ipDeinit();
  NetManDeinit();
}

int tcp_init_server(void)
{
    struct addrinfo hints, *res, *p;
    int listenfd;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, "80", &hints, &res) != 0)
    {
        scr_printf("!! couldn't enumerate available interfaces\n");
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
        scr_printf("!! enumerated interfaces but found nothing to bind to\n");
        return -1;
    }

    freeaddrinfo(res);

    return (listen(listenfd, 0x075EC00F) == 0) ? listenfd : -1;
}