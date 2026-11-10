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

A firmware framebuffer allocation is not proof that the DSI panel is initialized. The RP1 DSI host, TC358762 bridge, panel power sequence, and display timing must be handled separately.

## Confirmed Linux display reference

The connected panel was observed working under Ubuntu on the target Raspberry Pi 5. The active Linux DRM state reported:

| Property | Observed value |
|---|---:|
| Connector | DSI-1 |
| Resolution | 800x480 |
| Refresh rate | approximately 60.03 Hz |
| Pixel clock | 30 MHz |
| Horizontal display / front porch / sync / back porch | 800 / 131 / 2 / 45 |
| Vertical display / front porch / sync / back porch | 480 / 7 / 2 / 22 |
| DSI host | RP1 DSI (`drm-rp1-dsi`) |
| DSI bridge | Toshiba TC358762 |
| DSI data lanes | 1 |
| DRM pixel format | XR24 (little-endian XRGB8888; memory byte order B, G, R, X) |
| Framebuffer pitch | 3200 bytes |

The timing values are recorded in `drivers/display/dsi.cpp` as a reference configuration. The DRM mode string was `800x480: 60 30000 800 931 933 978 480 487 489 511`.

The DSI driver currently validates this configuration only. It does **not** program RP1 registers, initialize the D-PHY, configure TC358762, or power on the panel. Those operations must not be guessed: they will be implemented only after checking the matching Linux driver and panel power sequence.

## Current development stage

- [x] Repository initialized
- [x] Existing framebuffer pixel renderer
- [x] Existing boot-screen text renderer
- [x] Raspberry Pi 5 firmware framebuffer request (experimental; not yet hardware-verified)
- [x] Kernel calls the framebuffer and boot-screen code
- [x] Record the observed 800x480 Linux DSI timing as a native driver reference
- [x] Add validation for DSI panel timing configuration
- [ ] Verify the firmware mailbox request on Raspberry Pi 5 hardware
- [ ] Verify that the bare-metal kernel can display pixels on the DSI panel
- [ ] Implement and verify RP1 DSI host / D-PHY initialization
- [ ] Implement and verify TC358762 bridge and panel power sequencing

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
