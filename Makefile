# Makefile per il kernel di Blueport

# ATTENZIONE: il primo oggetto ad essere linkato DOVREBBE essere boot.o
OBJS = boot.o multiboot.o kernel.o video.o critical.o idt.o panic.o register.o irq.o sys.o driver.o asm.o errno.o
MODS = var/var.o mm/mm.o dev/dev.o disk/disk.o fs/fs.o

.EXPORT_ALL_VARIABLES:

LD = ld
CC = gcc -c
CFLAGS = -Wall -fomit-frame-pointer -march=i386 -ffreestanding -nostdinc -fno-builtin -DKBD_IT -DDEBUG #-DDEBUG2
DISASM = objdump -d

.PHONY: all
all: bpkernel

.PHONY: modules
modules:
	make -C var
	make -C mm
	make -C dev
	make -C disk
	make -C fs

bpkernel: $(OBJS) modules
	$(LD) -nostdlib -o $@ -T kernel.ld $(OBJS) $(MODS) -Map kernel.map
	$(DISASM) $@ >$@.dis

.S.o:
	$(CC) $(CFLAGS) -Iinclude/ -o $@ $<

.c.o:
	$(CC) $(CFLAGS) -Iinclude/ -o $@ $<

.PHONY: clean
clean:
	make -C var clean
	make -C mm clean
	make -C dev clean
	make -C disk clean
	make -C fs clean
	rm -f bpkernel
	rm -f bpkernel.dis
	rm -f kernel.map
	rm -f $(OBJS)

.PHONY: prepare
prepare: all
	mount ../bochs/grub/fd0.img ../bochs/grub/image -o loop
	rm -f ../bochs/grub/image/bpkernel
	cp bpkernel ../bochs/grub/image
	umount ../bochs/grub/image
