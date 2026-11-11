#include <fcntl.h>
#include <unistd.h>

int main(void)
{
    int console_fd = open("/dev/console", O_WRONLY);

    if (console_fd >= 0) {
        static const char message[] =
            "\n[gameplayOS] Native UI process is running.\n"
            "[gameplayOS] Display compositor: next milestone.\n";
        (void)write(console_fd, message, sizeof(message) - 1);
        close(console_fd);
    }

    for (;;) {
        pause();
    }
}
