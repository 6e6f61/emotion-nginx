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
#include <kernel.h>
#include <string.h>
#include <unistd.h>
#include <ps2ip.h>
#include <sifrpc.h>
#include <iopcontrol.h>

#include "tcp.h"
#include "mem_card.h"
#include "http.h"

#define INTERFACE "sm0"

int prepare_sif(void)
{
    int r;

    SifInitRpc(0);
    while (!SifIopReset("", 0)) { };
    while (!SifIopSync()) { };

    if ((r = mc_init_sif()) != 0)
    {
        scr_printf("!! couldn't prepare sif for memory cards: %d\n", r);
        return -1;
    }

    tcp_init_sif();

    return 0;
}

int main(void)
{
    struct sockaddr_in client_addr;
    int ca_size = sizeof (client_addr);
    int socket, r, listenfd;

    init_scr();
    sleep(2);
    scr_printf("emotion-nginx\n");

    if ((r = prepare_sif()) != 0) goto end;
    scr_printf("prepared sif\n");

    if (mc_init_server() != 0) goto end;
    scr_printf("initialised memory card server\n");

    if ((r = tcp_up_link(INTERFACE)) != 0) goto end;
    scr_printf("link is up\n");

    if ((listenfd = tcp_init_server()) < 0) goto end;
    scr_printf("interface bound\n");

    scr_printf("ready to serve requests on :80\n");
    for (;;)
    {
        if ((socket = accept(listenfd, (struct sockaddr *)&client_addr, &ca_size)) < 0)
        {
            scr_printf("!! error accepting request: %d\n", socket);
            close(socket);
            continue;
        }
        char *buffer = inet_ntoa(client_addr.sin_addr);
        scr_printf("accepting request from %s (%d)\n", buffer, socket);

        if ((r = http_respond(socket)) != 0) scr_printf("!! couldn't respond: %d\n", r);
        else scr_printf("served ok\n");

        close(socket);
    }

end:
    /* oh noes */
    scr_printf("!! end of main\n");
    tcp_deinit();
    for (;;) sleep(2); 

    return 0;
}