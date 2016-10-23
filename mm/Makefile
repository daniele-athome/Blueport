# Makefile per il modulo mm del kernel di Blueport

OBJS = fismem.o mem.o paging.o virmem.o mem_desc.o mem_area.o kmem.o page_fault.o

.PHONY: all
all: mm.o

mm.o: $(OBJS)
	$(LD) -r -o $@ $(OBJS)

.S.o:
	$(CC) $(CFLAGS) -I../include/ -o $@ $<

.c.o:
	$(CC) $(CFLAGS) -I../include/ -o $@ $<

.PHONY: clean
clean:
	rm -f $(OBJS)
	rm -f mm.o
