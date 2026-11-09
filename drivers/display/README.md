# Display bring-up

The first display target is the 7-inch DSI panel connected to the Raspberry Pi 5.

## Planned stages

1. Confirm the exact panel model and firmware display configuration.
2. Determine whether the boot firmware provides a usable framebuffer to the bare-metal kernel.
3. Read and validate framebuffer address, dimensions, pitch, and pixel format.
4. Draw a solid test colour.
5. Add a tiny bitmap-font renderer and display the gameplayOS boot text.
6. If no suitable framebuffer is handed over, investigate the Pi 5 display pipeline and DSI panel initialization separately.

Do not assume that the Raspberry Pi 4 mailbox/framebuffer setup works unchanged on Raspberry Pi 5.
