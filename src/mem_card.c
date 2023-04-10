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

#include "log.h"

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
        multi_log("failed to initialise mem card server: %d\n", r);
        return -1;
    }

    return 0;
}

int mc_no_cards(void)
{
    int t, f, m;
    multi_log(
    "port 0 slot 0: %d\nport 0 slot 1: %d\nport 1 slot 0: %d\nport 1 slot 1:%d\n",
        mcGetInfo(0, 0, &t, &f, &m),
        mcGetInfo(0, 1, &t, &f, &m),
        mcGetInfo(1, 0, &t, &f, &m),
        mcGetInfo(1, 1, &t, &f, &m));
    return mcGetInfo(0, 0, &t, &f, &m) <= -2
        && mcGetInfo(0, 1, &t, &f, &m) <= -2;
}