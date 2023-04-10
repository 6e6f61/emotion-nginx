#ifndef _MEM_CARD_H_
#define _MEM_CARD_H_
int mc_init_sif(void);
int mc_init_server(void);
int mc_no_cards(void);
int mc_is_dir(const char *);
int mc_retrieve_file(char *, const char *);
#endif