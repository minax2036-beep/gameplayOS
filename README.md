# gameplayOS

A custom Raspberry Pi 5 operating system built on the Linux kernel and Buildroot.

GameplayOS uses Linux as its hardware and process foundation, while presenting its own system UI and native application environment. It is not intended to be a general-purpose Linux distribution.

## Current direction

- Board: Raspberry Pi 5
- Architecture: AArch64 / Cortex-A76
- Kernel: Raspberry Pi downstream Linux kernel
- System builder: Buildroot 2026.08.1
- Init: GameplayOS-owned PID 1
- Target UI: native GameplayOS process
- Display target: Freenove 7-inch DSI panel, 800×480

## Shell-free system policy

The target image is built without BusyBox and without an interactive shell. It does not provide Bash, ash, a login prompt, a terminal application, package managers, or general-purpose command-line utilities.

The kernel still provides its normal system-call interface. GameplayOS controls which native programs are installed and started; removing a shell is not, by itself, a complete security boundary. Future services must expose only specific operations needed by the UI and must not provide an arbitrary command launcher.

Build tools remain on the development host and are not copied into the Raspberry Pi image.

## Current milestone

This branch establishes the first Buildroot-based bootable system structure:

- [x] Buildroot external tree
- [x] Raspberry Pi 5 configuration
- [x] Custom PID 1 with no shell fallback
- [x] Native UI process placeholder
- [x] Minimal Linux display / framebuffer configuration fragment
- [x] GitHub Actions build workflow
- [ ] Verify the image boots on the Raspberry Pi 5
- [ ] Draw the first native UI on the 800×480 DSI display
- [ ] Implement input handling and the boot screen
- [ ] Add Safe Mode and Recovery UI

The current UI process is intentionally a placeholder. It writes a status message to the kernel console and waits; graphical output is the next implementation step.

## Build on the development host

Required host tools include Git, Make, a C/C++ build environment, bc, cpio, rsync, unzip, wget, and ncurses development headers.

Fetch the pinned Buildroot release:

    git clone --depth 1 --branch 2026.08.1 https://gitlab.com/buildroot.org/buildroot.git

Configure and build from the gameplayOS repository root:

    make -C buildroot O=$PWD/output BR2_EXTERNAL=$PWD gameplayos_rpi5_defconfig
    make -C buildroot O=$PWD/output BR2_EXTERNAL=$PWD -j2

Build output is written to output/images/. The Raspberry Pi 5 post-image script creates the SD-card image.

GitHub Actions runs the same configuration and uploads the generated image as an artifact. The CI job also checks that no shell or BusyBox command suite is present in the target filesystem.

## Safety when testing

This is experimental OS software. Use a separate SD card or USB device for testing. Do not overwrite the storage used by an existing Raspberry Pi OS, Ubuntu, or NAS installation.
