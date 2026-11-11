#include <cstddef>

extern "C" void* memset(void* destination, int value, std::size_t count)
{
    auto* output = static_cast<volatile unsigned char*>(destination);
    const unsigned char byte = static_cast<unsigned char>(value);

    for (std::size_t i = 0; i < count; ++i) {
        output[i] = byte;
    }

    return destination;
}
