.section .text.boot
.global _start
.type _start, %function

_start:
    // Only the primary CPU runs the initial kernel bring-up.
    mrs     x0, mpidr_el1
    and     x0, x0, #0x3
    cbnz    x0, .Lpark

    // Use a private, 16-byte-aligned bootstrap stack.
    ldr     x0, =__stack_top
    mov     sp, x0

    // Clear the kernel BSS section.
    ldr     x0, =__bss_start
    ldr     x1, =__bss_end
.Lclear_bss:
    cmp     x0, x1
    b.hs    .Lbss_done
    str     xzr, [x0], #8
    b       .Lclear_bss

.Lbss_done:
    dsb     sy
    isb
    bl      kernel_main

.Lpark:
    wfe
    b       .Lpark

.size _start, . - _start

.section .bss.stack, "aw", %nobits
.align 4
.global __stack_bottom
__stack_bottom:
    .skip 16384
.global __stack_top
__stack_top:
