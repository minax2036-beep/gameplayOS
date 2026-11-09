# gameplayOS

An independent AArch64 game-console operating system for Raspberry Pi 5.

## First milestone: text on the DSI display

The first goal is to boot a tiny freestanding kernel and show:

```text
gameplayOS
Kernel initialized.
Hello from AArch64!
```

We will bring up the display in small, testable steps. Raspberry Pi 5's boot firmware and display pipeline differ from older Pi models, so the DSI framebuffer path will be implemented only after its hand-off and display mode are verified.

## Current development stage

- [x] Repository initialized
- [ ] AArch64 boot entry and linker layout
- [ ] Minimal freestanding kernel
- [ ] Raspberry Pi 5 display/framebuffer hand-off
- [ ] Text renderer
- [ ] DSI text output

## Target

- Board: Raspberry Pi 5
- Architecture: AArch64
- Kernel: C++ (freestanding)
- Initial display: 7-inch DSI display
- Initial debugging: UART where available

## Build artifacts

GitHub Actions creates these files:

- `gameplayOS-rpi5.bin`: raw Raspberry Pi 5 disk image, intended for tools that accept local `.bin` images such as Chromebook Recovery Utility.
- `gameplayOS-rpi5.img`: identical raw disk image with the conventional `.img` extension.
- `kernel8.img`: kernel binary only; this is not the complete disk image.
- `gameplayOS.elf`: linked ELF file for debugging.

The `.bin` and `.img` disk images contain the same bytes. The extension does not change the image format.

## Boot files

The Raspberry Pi 5 boot partition requires a `config.txt`. The firmware normally looks for `kernel_2712.img`, then falls back to `kernel8.img`. This project initially uses `kernel8.img` for a simpler 4 KiB-page bring-up target.

**This is experimental bare-metal software. Do not overwrite the boot files or storage used by your existing Raspberry Pi OS/NAS installation.** Test from a separate SD card or USB device.
