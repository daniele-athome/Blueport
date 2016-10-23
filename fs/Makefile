# Makefile per il modulo fs del kernel di Blueport

OBJS = dev.o initfs.o path.o vfs.o

.PHONY: all
all: fs.o

fs.o: $(OBJS)
	$(LD) -r -o $@ $(OBJS)

.S.o:
	$(CC) $(CFLAGS) -I../include/ -o $@ $<

.c.o:
	$(CC) $(CFLAGS) -I../include/ -o $@ $<

.PHONY: clean
clean:
	rm -f $(OBJS)
	rm -f fs.o
