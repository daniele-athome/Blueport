# Makefile per il modulo disk del kernel di Blueport

OBJS = ramdisk.o

.PHONY: all
all: disk.o

disk.o: $(OBJS)
	$(LD) -r -o $@ $(OBJS)

.S.o:
	$(CC) $(CFLAGS) -I../include/ -o $@ $<

.c.o:
	$(CC) $(CFLAGS) -I../include/ -o $@ $<

.PHONY: clean
clean:
	rm -f $(OBJS)
	rm -f disk.o
