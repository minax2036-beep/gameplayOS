# gameplayOS

An independent AArch64 game-console operating system for Raspberry Pi 5.

## First milestone: text on the DSI display

The first goal is to boot a tiny freestanding kernel and show:

```text
gameplayOS
Kernel initialized.
Hello from AArch64!
```

The current bring-up branch requests a 32-bit framebuffer from the Raspberry Pi firmware using the property-mailbox interface, then connects it to the existing pixel and boot-screen renderers.

This is an experimental first step, not proof that the DSI panel is working. Raspberry Pi 5 firmware supports only a subset of the older property tags, and the display pipeline/panel initialization is separate. The framebuffer request and the Freenove DSI panel output still need real-hardware verification.

## Current development stage

- [x] Repository initialized
- [x] Existing framebuffer pixel renderer
- [x] Existing boot-screen text renderer
- [x] Raspberry Pi 5 firmware framebuffer request (experimental; not yet hardware-verified)
- [x] Kernel calls the framebuffer and boot-screen code
- [ ] Verify the firmware mailbox request on Raspberry Pi 5 hardware
- [ ] Verify that the Freenove DSI panel displays the framebuffer
- [ ] Implement panel-specific DSI initialization if firmware hand-off is insufficient

## Target

- Board: Raspberry Pi 5
- Architecture: AArch64
- Kernel: C++ (freestanding)
- Initial display: 7-inch DSI display
- Initial debugging: display-first; UART is not required for this milestone

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
