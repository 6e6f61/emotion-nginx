EE_BIN=emngx.elf
EE_OBJS=src/emngx.o src/http.o src/tcp.o src/mem_card.o \
	generated/DEV9_irx.o generated/NETMAN_irx.o generated/SMAP_irx.o
EE_LIBS=-ldebug -lc \
	-lnetman -lps2ip -lpatches -lmc
EE_OPTFLAGS=-Wall

all: $(EE_BIN)

clean:
	-rm $(EE_BIN) $(EE_OBJS)

generated/DEV9_irx.c: $(PS2SDK)/iop/irx/ps2dev9.irx
	bin2c $< generated/DEV9_irx.c DEV9_irx

generated/NETMAN_irx.c: $(PS2SDK)/iop/irx/netman.irx
	bin2c $< generated/NETMAN_irx.c NETMAN_irx

generated/SMAP_irx.c: $(PS2SDK)/iop/irx/smap.irx
	bin2c $< generated/SMAP_irx.c SMAP_irx

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
