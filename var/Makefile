# Makefile per il modulo var del kernel di Blueport

OBJS = ctype.o math.o modf.o stdlib.o vsprintf.o progress.o

.PHONY: all
all: var.o

var.o: $(OBJS)
	$(LD) -r -o $@ $(OBJS)

.S.o:
	$(CC) $(CFLAGS) -I../include/ -o $@ $<

.c.o:
	$(CC) $(CFLAGS) -I../include/ -o $@ $<

.PHONY: clean
clean:
	rm -f $(OBJS)
	rm -f var.o
