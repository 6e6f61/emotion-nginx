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
#include <libmc.h>
#include <loadfile.h>
#include <debug.h>
#include <fcntl.h>

int mc_init_sif(void)
{
    if (SifLoadModule("rom0:XSIO2MAN", 0, NULL) < 0)
        return -1;

    if (SifLoadModule("rom0:XMCMAN", 0, NULL) < 0)
        return -1;

    if (SifLoadModule("rom0:XMCSERV", 0, NULL) < 0)
        return -1;

    return 0;
}

int mc_init_server(void)
{
    int r;

    if ((r = mcInit(MC_TYPE_XMC)) < 0)
    {
        scr_printf("failed to initialise mem card server: %d\n", r);
        return -1;
    }

    return 0;
}

int mc_no_cards(void)
{
    int r_slot1, r_slot2;
    int t;

    mcGetInfo(0, 0, &t, &t, &t);
    mcSync(0, NULL, &r_slot1);

    mcGetInfo(1, 0, &t, &t, &t);
    mcSync(0, NULL, &r_slot2);

    scr_printf("detected mem card state:\nslot 1: %d; slot 2: %d\n", r_slot1, r_slot2);
    return r_slot1 < -2 && r_slot2 < -2;
}

int mc_is_dir(const char *path)
{
    int r;

    mcGetDir(0, 0, path, 0, 0, NULL);
    mcSync(0, NULL, &r);
    if (r > 0) return 1;

    mcGetDir(1, 0, path, 0, 0, NULL);
    mcSync(0, NULL, &r);
    return r > 0;    
}

int mc_retrieve_file(char *dst, const char *path)
{
    int r, fd;

    mcOpen(0, 0, path, O_RDONLY);
    mcSync(0, NULL, &fd);
    if (fd < 0)
    {   
        mcOpen(1, 0, path, O_RDONLY);
        mcSync(0, NULL, &fd);
    }

    if (fd < 0) return fd;

    mcRead(fd, dst, 65535);
    mcSync(0, NULL, &r);

    return r;
}