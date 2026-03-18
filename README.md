DonutOS 4.2

Dependencies:
- `gcc`
- `cpio`
- `gzip`
- `grub-mkrescue`
- `qemu-system-x86_64` (optional)

Build:
- `make -j$(nproc)`

Rebuild:
- `make -j$(nproc) rebuild`

Run in QEMU:
- `make qemu`

Clean:
- `make clean`
