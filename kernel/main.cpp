#include <cstdint>

extern "C" void kernel_main()
{
    // First milestone: prove that the AArch64 kernel reaches C++.
    // The DSI framebuffer and text renderer will be added next.
    volatile std::uint32_t heartbeat = 0;

    for (;;) {
        ++heartbeat;
        asm volatile("wfe");
    }
}
