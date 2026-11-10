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

The DSI driver currently validates this configuration only. It does **not** program RP1 registers, initialize the D-PHY, or configure TC358762.\n\n## ATTINY backlight controller\n\nThe panel's backlight and power rails are controlled by an ATTINY over I2C, not by a simple Raspberry Pi GPIO. Ubuntu exposed the controller at I2C address `0x45` on bus `11`; the Linux driver identifies controller revisions `0xde` and `0xc3`, uses registers `0x80` through `0x86`, and drives brightness through PWM register `0x86`.\n\n`drivers/display/attiny_backlight.cpp` now implements the verified ATTINY register protocol, controller-ID check, panel power-up sequence, and full-brightness request. It expects an RP1 I2C transport to be supplied through callbacks. **That low-level RP1 I2C transport is not implemented or connected yet**, so this change alone does not turn on the physical backlight. The kernel must not call the driver until a real I2C transport is available. The sequence follows the upstream Linux ATTINY regulator/backlight driver.\n\nThe implementation deliberately does not guess RP1 I2C MMIO addresses or controller timing.

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
- [x] Implement the ATTINY panel-controller protocol and verified power-up sequence (I2C transport not yet connected)\n- [ ] Implement and verify the RP1 I2C transport for the ATTINY controller\n- [ ] Turn on and verify the physical backlight on Raspberry Pi 5
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
