include config.mk

PHONY := all rebuild progs init shell utils games initramfs iso qemu clean

all: iso

rebuild: clean all

progs: init shell utils games

init:
	@echo "Making init"
	@$(MAKE) -C init

shell:
	@echo "Making shell"
	@$(MAKE) -C shell

utils:
	@echo "Making utils"
	@$(MAKE) -C utils

games:
	@echo "Making games"
	@$(MAKE) -C games

initramfs: progs
	@echo "Packing initramfs"
	@cd build && find . -print0 | cpio --null -ov --format=newc | gzip -9 > ../DonutOS

iso: initramfs
	@echo "Making ISO"
	@mkdir -p iso/boot/grub
	@cp kernel/kernel iso/
	@cp grub.cfg iso/boot/grub/
	@cp DonutOS iso/
	@grub-mkrescue -o "DonutOS-$(DonutOS_Version).iso" iso --compress=xz

qemu: iso
	qemu-system-x86_64 -cdrom "DonutOS-$(DonutOS_Version).iso"

clean:
	@echo "Cleaning..."
	rm -rf build
	rm -rf iso
	rm -rf DonutOS
	rm -rf "DonutOS-$(DonutOS_Version).iso"

.PHONY: $(PHONY)
