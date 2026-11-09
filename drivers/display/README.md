# Display bring-up

The first display target is the user's Freenove 7-inch IPS DSI touchscreen (Amazon listing B0B44VZTRG; FNK0078 family).

## Confirmed panel properties

- Display size: 7 inch
- Resolution: 800 x 480
- Panel type: IPS
- Interface: MIPI DSI
- Touch: 5-point capacitive
- Board target: Raspberry Pi 5

The exact DSI lane count, panel controller, DSI pixel format, command sequence, reset/power control, and timing values are not yet confirmed. Do not guess these values or enable the panel driver until they are verified from the matching FNK0078 manual/schematic.

## Added code

- `framebuffer.hpp/.cpp`: validated framebuffer descriptor and RGBX, BGRX, and RGB565 pixel writes.
- `boot_screen.hpp/.cpp`: colour-bar test pattern, black boot-log strip, compact 5x7 bitmap font, and severity-coloured log entries.
- `dsi.hpp/.cpp`: DSI initialization interface. It currently returns `NotImplemented` rather than touching unknown hardware registers.
- `panel_profile.hpp`: confirmed 800x480 resolution and five touch points; intentionally leaves DSI lane count unknown.

## Planned stages

1. Obtain and inspect the exact FNK0078 tutorial/manual and identify the panel controller, DSI lane count, and power/reset wiring.
2. Determine whether the boot firmware provides a usable framebuffer to the bare-metal kernel.
3. Read and validate framebuffer address, dimensions, pitch, and pixel format.
4. Draw the colour-bar test pattern and black boot-log strip using `draw_boot_test_screen()`.
5. Update the log area with `draw_boot_log()` as initialization messages arrive.
6. If no suitable framebuffer is handed over, implement the RP1 DSI host and the panel-specific initialization sequence.

## Raspberry Pi 5 notes

The DSI controller is part of RP1 on Raspberry Pi 5. Do not assume that Raspberry Pi 4 register addresses or its mailbox/framebuffer setup work unchanged.

A Device Tree overlay such as a `vc4-kms-dsi-...` overlay configures a Linux DRM/KMS driver. It does not, by itself, provide a bare-metal DSI driver.

Do not write to guessed MMIO addresses. Panel reset, power, lane count, PHY timing, and DSI command sequences must be based on the exact panel hardware.


## Boot test screen API

`draw_boot_test_screen(framebuffer)` paints the colour bars and clears the bottom 120 pixels to black. `draw_boot_log(framebuffer, entries, count)` redraws only that black area and shows up to the latest five messages. Log messages use the built-in compact ASCII font. The caller must supply a real, validated framebuffer; the current kernel does not yet discover or receive one from Raspberry Pi firmware.
