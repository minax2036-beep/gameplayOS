# Display bring-up

The first display target is the 7-inch DSI panel connected to the Raspberry Pi 5.

## Added code

- `framebuffer.hpp/.cpp`: validated framebuffer descriptor and RGBX, BGRX, and RGB565 pixel writes.
- `dsi.hpp/.cpp`: DSI initialization interface. It currently returns `NotImplemented` rather than touching unknown hardware registers.

## Planned stages

1. Confirm the exact panel model and which Pi 5 DSI connector is used.
2. Determine whether the boot firmware provides a usable framebuffer to the bare-metal kernel.
3. Read and validate framebuffer address, dimensions, pitch, and pixel format.
4. Draw a solid test colour through the framebuffer module.
5. Add a small bitmap font and display the gameplayOS boot text.
6. If no suitable framebuffer is handed over, implement the RP1 DSI host and the panel-specific initialization sequence.

## Raspberry Pi 5 notes

The DSI controller is part of RP1 on Raspberry Pi 5. Do not assume that Raspberry Pi 4 register addresses or its mailbox/framebuffer setup work unchanged.

A Device Tree overlay such as a `vc4-kms-dsi-...` overlay configures a Linux DRM/KMS driver. It does not, by itself, provide a bare-metal DSI driver.

Do not write to guessed MMIO addresses. Panel reset, power, lane count, PHY timing, and DSI command sequences must be based on the exact panel hardware.
