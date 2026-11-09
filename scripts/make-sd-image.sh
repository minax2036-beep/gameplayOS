#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
KERNEL="${ROOT_DIR}/build/kernel8.img"
CONFIG="${ROOT_DIR}/config.txt"
FIRMWARE_DIR="${RPI_FIRMWARE_BOOT_DIR:-}"

IMAGE="${1:-${ROOT_DIR}/build/gameplayOS-rpi5.img}"
IMAGE_SIZE_MIB="${IMAGE_SIZE_MIB:-512}"
PARTITION_OFFSET_BYTES=1048576

if [[ ! -f "$KERNEL" ]]; then
    echo "error: kernel not found: $KERNEL (run make first)" >&2
    exit 1
fi
if [[ ! -f "$CONFIG" ]]; then
    echo "error: config.txt not found" >&2
    exit 1
fi
if [[ -z "$FIRMWARE_DIR" || ! -d "$FIRMWARE_DIR" ]]; then
    echo "error: set RPI_FIRMWARE_BOOT_DIR to the Raspberry Pi firmware boot directory" >&2
    exit 1
fi

for file in start4.elf fixup4.dat bcm2712-rpi-5-b.dtb; do
    if [[ ! -f "${FIRMWARE_DIR}/${file}" ]]; then
        echo "error: required Raspberry Pi 5 boot file missing: ${FIRMWARE_DIR}/${file}" >&2
        exit 1
    fi
done
if [[ ! -d "${FIRMWARE_DIR}/overlays" ]]; then
    echo "error: firmware overlays directory missing" >&2
    exit 1
fi

for tool in truncate sfdisk mformat mcopy; do
    command -v "$tool" >/dev/null 2>&1 || {
        echo "error: required tool not found: $tool" >&2
        exit 1
    }
done

mkdir -p "$(dirname "$IMAGE")"
rm -f "$IMAGE"
truncate -s "${IMAGE_SIZE_MIB}M" "$IMAGE"

# Create an MBR partition table with one FAT32 boot partition.
printf 'label: dos\nunit: sectors\n\nstart=2048, type=c, bootable\n' | sfdisk "$IMAGE"

# mtools writes directly into the partition area; no loop device is required.
PARTITION="${IMAGE}@@${PARTITION_OFFSET_BYTES}"
mformat -i "$PARTITION" -F -v GAMEPLAYOS ::

mcopy -i "$PARTITION" "$KERNEL" ::/kernel8.img
mcopy -i "$PARTITION" "$CONFIG" ::/config.txt
mcopy -i "$PARTITION" "${FIRMWARE_DIR}/start4.elf" ::/start4.elf
mcopy -i "$PARTITION" "${FIRMWARE_DIR}/fixup4.dat" ::/fixup4.dat
mcopy -i "$PARTITION" "${FIRMWARE_DIR}/bcm2712-rpi-5-b.dtb" ::/bcm2712-rpi-5-b.dtb
mcopy -s -i "$PARTITION" "${FIRMWARE_DIR}/overlays" ::/overlays

echo "Created Raspberry Pi 5 SD-card image: $IMAGE"
ls -lh "$IMAGE"
