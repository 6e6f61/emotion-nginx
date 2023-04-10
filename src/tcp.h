#ifndef _TCP_H_
#define _TCP_H_
#define print_ip4_addr(l, x) do { \
    struct ip4_addr *t = (struct ip4_addr *)&x; \
    scr_printf(l "%d.%d.%d.%d\n", ip4_addr1(t), ip4_addr2(t), ip4_addr3(t), ip4_addr4(t)); \
    } while (0)

void tcp_init_sif();
int tcp_enable_dhcp(char *);
int tcp_up_link(char *);
void tcp_deinit(void);
int tcp_init_server(void);
#endif