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
#include <string.h>
#include <ctype.h>
#include <ps2ip.h>

#include "log.h"
#include "mem_card.h"

static int  parse_request(int, char *);
static void url_decode(char *, const char *);

#define MAX_REQUEST_SIZE (1024 * 2)

const char NO_MEMORY_CARDS[] =
"<!DOCTYPE html><html><head><title>Welcome to emotion-nginx!</title><style>html{color-scheme:light"
"dark;}body{width:35em;margin:0 auto;font-family:Tahoma,Verdana,Arial,sans-serif;}</style></head>"
"<body><h1>Welcome to emotion-nginx!</h1><p>If you are seeing this page, the emotion-nginx web "
"server is successfully installed and working. Further configuration is required.</p><p>For online "
"documentation and support please refer to /dev/null. Commercial support is available at "
"/dev/random.</p><p><em>Thank you for using emotion-nginx.</em></p></body></html>\n";

const char FOUR_OH_FOUR[] =
"<!DOCTYPE html><html><head><title>404 Not Found</title></head><body><h1 style=\"text-align:"
"center;\">404 Not Found</h1><hr><p>emotion-nginx</p></body></html>\n";

const char FIVE_OH_ONE[] =
"<!DOCTYPE html><html><head><title>501 Unsupported Method</title></head><body><h1 style="
"\"text-align:center;\">501 Unsupported Method</h1><hr><p>emotion-nginx</p></body></html>\n";

int http_respond(int socket)
{
    char path[512] = { 0 };
    if (parse_request(socket, path) == -1)
    {
        return (writesocket(socket, "HTTP/1.0 501 Unsupported Method\r\n\r\n", 34) == -1 ||
               writesocket(socket, FIVE_OH_ONE, sizeof (FIVE_OH_ONE)) == -1) ? -1 : 0;
    }

    if (writesocket(socket, "HTTP/1.0 200 OK\r\n\r\n", 39) == -1)
        return -1;

    if (mc_no_cards())
    {
        multi_log("no memory cards inserted; served template\n");
        if (writesocket(socket, NO_MEMORY_CARDS, sizeof (NO_MEMORY_CARDS) - 1) == -1)
            return -1;
        return 0;
    } else
    {
        multi_log("mem card found\n");
    }

    return 0;
}

/* -1 = unsupported method; < -1 = other error */
static int parse_request(int socket, char *out_path)
{
    char method[5]   = { 0 };
    char path[512]   = { 0 };
    char http_ver[3] = { 0 };
    char req[MAX_REQUEST_SIZE] = { 0 };
    long bytes_recv = recv(socket, req, MAX_REQUEST_SIZE, 0);
    if (bytes_recv <= 0) return -1;

    if (sscanf(req, "%7s /%s HTTP/%s", method, path, http_ver) == EOF)
        return -2;
    
    if (strncmp(method, "GET", 3) != 0 && strncmp(method, "HEAD", 5) != 0)
    {
        return -1;
    }
    
    if (strlen(http_ver) == 0) strcpy(path, "index.html");
    url_decode(out_path, path);

    return 0;
}

/* https://stackoverflow.com/questions/2673207/c-c-url-decode-library */
static void url_decode(char *dst, const char *src)
{
    /* decode */
    char a, b;
    while (*src)
    {
        if (*src == '+')
        {
            *dst++ = ' ';
            src++;
            continue;
        }

        if (*src == '%')
        {
            a = src[1], b = src[2];
            if (isxdigit(a) && isxdigit(b))
            {
                if (a >= 'a') a -= 'a'-'A';
                if (a >= 'A') a -= ('A' - 10);
                else a -= '0';

                if (b >= 'a') b -= 'a'-'A';
                if (b >= 'A') b -= ('A' - 10);
                else b -= '0';

                *dst++ = 16 * a + b;
                src += 3;
            }
            continue;
        }

        *dst++ = *src++;
    }
    *dst++ = '\0';
}