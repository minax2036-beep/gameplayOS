#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

static void ensure_directory(const char *path)
{
    if (mkdir(path, 0755) < 0 && errno != EEXIST) {
        dprintf(STDERR_FILENO, "gameplayOS init: mkdir %s failed: %d\n",
                path, errno);
    }
}

static void mount_virtual_fs(const char *source,
                             const char *target,
                             const char *type)
{
    ensure_directory(target);
    if (mount(source, target, type, 0, NULL) < 0 && errno != EBUSY) {
        dprintf(STDERR_FILENO, "gameplayOS init: mount %s failed: %d\n",
                target, errno);
    }
}

static void silence_console_input(int fd)
{
    struct termios settings;

    if (tcgetattr(fd, &settings) < 0) return;

    settings.c_lflag &= (tcflag_t)~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    settings.c_iflag &= (tcflag_t)~(IXON | ICRNL | INLCR);
    settings.c_cc[VMIN] = 1;
    settings.c_cc[VTIME] = 0;
    (void)tcsetattr(fd, TCSANOW, &settings);
}

int main(void)
{
    int console_fd;

    umask(022);
    (void)chdir("/");

    mount_virtual_fs("proc", "/proc", "proc");
    mount_virtual_fs("sysfs", "/sys", "sysfs");
    mount_virtual_fs("devtmpfs", "/dev", "devtmpfs");
    mount_virtual_fs("tmpfs", "/run", "tmpfs");

    console_fd = open("/dev/console", O_RDWR);
    if (console_fd >= 0) {
        silence_console_input(console_fd);
        (void)dup2(console_fd, STDIN_FILENO);
        (void)dup2(console_fd, STDOUT_FILENO);
        (void)dup2(console_fd, STDERR_FILENO);
        if (console_fd > STDERR_FILENO) close(console_fd);
    }

    puts("");
    puts("================================");
    puts("          gameplayOS");
    puts("       Linux system base");
    puts("================================");
    puts("Starting the native XMB interface...");

    {
        char *const argv[] = { (char *)"gameplayos-ui", NULL };
        char *const envp[] = {
            (char *)"PATH=/usr/bin",
            (char *)"HOME=/",
            (char *)"LANG=C",
            NULL
        };
        execve("/usr/bin/gameplayos-ui", argv, envp);
    }

    dprintf(STDERR_FILENO,
            "gameplayOS init: UI launch failed: %d\n", errno);
    puts("Recovery state: the UI process could not be started.");

    for (;;) pause();
}
