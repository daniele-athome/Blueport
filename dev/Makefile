# Makefile per il modulo dev del kernel di Blueport

OBJS = keyboard.o

.PHONY: all
all: dev.o

dev.o: $(OBJS)
	$(LD) -r -o $@ $(OBJS)

.S.o:
	$(CC) $(CFLAGS) -I../include/ -o $@ $<

.c.o:
	$(CC) $(CFLAGS) -I../include/ -o $@ $<

.PHONY:
clean:
	rm -f $(OBJS)
	rm -f dev.o
