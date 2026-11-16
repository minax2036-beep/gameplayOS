#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#define MAX_INPUTS 16
#define CATEGORY_COUNT 6

struct framebuffer {
    int fd;
    uint8_t *memory;
    size_t length;
    struct fb_fix_screeninfo fix;
    struct fb_var_screeninfo var;
    unsigned bytes_per_pixel;
};

static const uint8_t font[36][5] = {
    {0x7e,0x11,0x11,0x11,0x7e}, {0x7f,0x49,0x49,0x49,0x36},
    {0x3e,0x41,0x41,0x41,0x22}, {0x7f,0x41,0x41,0x22,0x1c},
    {0x7f,0x49,0x49,0x49,0x41}, {0x7f,0x09,0x09,0x09,0x01},
    {0x3e,0x41,0x49,0x49,0x7a}, {0x7f,0x08,0x08,0x08,0x7f},
    {0x00,0x41,0x7f,0x41,0x00}, {0x20,0x40,0x41,0x3f,0x01},
    {0x7f,0x08,0x14,0x22,0x41}, {0x7f,0x40,0x40,0x40,0x40},
    {0x7f,0x02,0x0c,0x02,0x7f}, {0x7f,0x04,0x08,0x10,0x7f},
    {0x3e,0x41,0x41,0x41,0x3e}, {0x7f,0x09,0x09,0x09,0x06},
    {0x3e,0x41,0x51,0x21,0x5e}, {0x7f,0x09,0x19,0x29,0x46},
    {0x46,0x49,0x49,0x49,0x31}, {0x01,0x01,0x7f,0x01,0x01},
    {0x3f,0x40,0x40,0x40,0x3f}, {0x1f,0x20,0x40,0x20,0x1f},
    {0x3f,0x40,0x38,0x40,0x3f}, {0x63,0x14,0x08,0x14,0x63},
    {0x07,0x08,0x70,0x08,0x07}, {0x61,0x51,0x49,0x45,0x43},
    {0x3e,0x51,0x49,0x45,0x3e}, {0x00,0x42,0x7f,0x40,0x00},
    {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4b,0x31},
    {0x18,0x14,0x12,0x7f,0x10}, {0x27,0x45,0x45,0x45,0x39},
    {0x3c,0x4a,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1e}
};

static const char *categories[CATEGORY_COUNT] = {
    "GAME", "NETWORK", "MUSIC", "VIDEO", "SETTINGS", "SYSTEM"
};

static const char *items[CATEGORY_COUNT][4] = {
    {"GAMES", "MEMORY CARD", "GAME DATA", "GAME OPTIONS"},
    {"NETWORK STATUS", "DOWNLOADS", "NETWORK OPTIONS", "CONNECTION INFO"},
    {"MUSIC PLAYER", "MUSIC FILES", "PLAYLISTS", "AUDIO OPTIONS"},
    {"VIDEO PLAYER", "VIDEO FILES", "VIDEO OPTIONS", "DISPLAY INFO"},
    {"SYSTEM SETTINGS", "DISPLAY SETTINGS", "SOUND SETTINGS", "INPUT SETTINGS"},
    {"SYSTEM INFORMATION", "STORAGE INFORMATION", "RESTART", "ABOUT GAMEPLAYOS"}
};

static uint32_t component(uint8_t value, struct fb_bitfield field)
{
    uint32_t maximum;
    if (field.length == 0) return 0;
    maximum = (1u << field.length) - 1u;
    return (((uint32_t)value * maximum + 127u) / 255u) << field.offset;
}

static uint32_t pixel_value(struct framebuffer *fb,
                            uint8_t r, uint8_t g, uint8_t b)
{
    return component(r, fb->var.red) |
           component(g, fb->var.green) |
           component(b, fb->var.blue) |
           component(255, fb->var.transp);
}

static void put_pixel(struct framebuffer *fb, int x, int y,
                      uint8_t r, uint8_t g, uint8_t b)
{
    size_t offset;
    uint32_t value;
    uint8_t *p;

    if (x < 0 || y < 0 || x >= (int)fb->var.xres ||
        y >= (int)fb->var.yres) return;

    offset = (size_t)(y + fb->var.yoffset) * fb->fix.line_length +
             (size_t)(x + fb->var.xoffset) * fb->bytes_per_pixel;
    if (offset + fb->bytes_per_pixel > fb->length) return;

    value = pixel_value(fb, r, g, b);
    p = fb->memory + offset;
    p[0] = (uint8_t)value;
    if (fb->bytes_per_pixel > 1) p[1] = (uint8_t)(value >> 8);
    if (fb->bytes_per_pixel > 2) p[2] = (uint8_t)(value >> 16);
    if (fb->bytes_per_pixel > 3) p[3] = (uint8_t)(value >> 24);
}

static void fill_rect(struct framebuffer *fb, int x, int y, int w, int h,
                      uint8_t r, uint8_t g, uint8_t b)
{
    int xx, yy;
    int x2 = x + w, y2 = y + h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x2 > (int)fb->var.xres) x2 = (int)fb->var.xres;
    if (y2 > (int)fb->var.yres) y2 = (int)fb->var.yres;
    for (yy = y; yy < y2; ++yy)
        for (xx = x; xx < x2; ++xx)
            put_pixel(fb, xx, yy, r, g, b);
}

static void draw_text(struct framebuffer *fb, int x, int y,
                      const char *text, int scale,
                      uint8_t r, uint8_t g, uint8_t b)
{
    for (; *text; ++text, x += 6 * scale) {
        int index = -1, row, col;
        char c = *text;
        if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        if (c >= 'A' && c <= 'Z') index = c - 'A';
        else if (c >= '0' && c <= '9') index = 26 + c - '0';
        if (index < 0) continue;
        for (col = 0; col < 5; ++col)
            for (row = 0; row < 7; ++row)
                if (font[index][col] & (1u << row))
                    fill_rect(fb, x + col * scale, y + row * scale,
                              scale, scale, r, g, b);
    }
}

static void draw_circle(struct framebuffer *fb, int cx, int cy, int radius,
                        uint8_t r, uint8_t g, uint8_t b)
{
    int x, y;
    for (y = -radius; y <= radius; ++y)
        for (x = -radius; x <= radius; ++x)
            if (x * x + y * y <= radius * radius)
                put_pixel(fb, cx + x, cy + y, r, g, b);
}

static void draw_wave(struct framebuffer *fb, int base, int phase,
                      uint8_t r, uint8_t g, uint8_t b)
{
    static const int wave[32] = {
        0, 3, 6, 9, 11, 13, 14, 15, 15, 14, 13, 11, 9, 6, 3, 0,
        -3, -6, -9, -11, -13, -14, -15, -15, -14, -13, -11, -9, -6, -3, 0, 0
    };
    int x, y, previous = base + wave[(phase / 7) % 32];
    for (x = 1; x < (int)fb->var.xres; ++x) {
        y = base + wave[((x + phase) / 7) % 32];
        if (y > previous) {
            int yy;
            for (yy = previous; yy <= y; ++yy)
                put_pixel(fb, x, yy, r, g, b);
        } else {
            int yy;
            for (yy = y; yy <= previous; ++yy)
                put_pixel(fb, x, yy, r, g, b);
        }
        previous = y;
    }
}

static void draw_background(struct framebuffer *fb)
{
    int y, x;
    int height = (int)fb->var.yres;
    int width = (int)fb->var.xres;

    for (y = 0; y < height; ++y) {
        uint8_t r = (uint8_t)(5 + (y * 5) / (height ? height : 1));
        uint8_t g = (uint8_t)(12 + (y * 15) / (height ? height : 1));
        uint8_t b = (uint8_t)(48 + (y * 45) / (height ? height : 1));
        fill_rect(fb, 0, y, width, 1, r, g, b);
    }

    for (x = 0; x < 4; ++x)
        draw_wave(fb, height - 90 - x * 27, x * 19,
                  (uint8_t)(12 + x * 5), (uint8_t)(55 + x * 8),
                  (uint8_t)(115 + x * 12));
}

static void draw_category(struct framebuffer *fb, int index, int x, int y,
                          int selected)
{
    int radius = selected ? 15 : 10;
    draw_circle(fb, x, y, radius,
                selected ? 65 : 75,
                selected ? 205 : 105,
                selected ? 245 : 150);
    draw_circle(fb, x, y, selected ? 7 : 4, 225, 245, 255);
    draw_text(fb, x - 22, y + 25, categories[index], 1,
              selected ? 235 : 145,
              selected ? 250 : 175,
              selected ? 255 : 205);
}

static void draw_screen(struct framebuffer *fb, int category, int selected)
{
    int width = (int)fb->var.xres;
    int height = (int)fb->var.yres;
    int i, top, left, row_height;

    draw_background(fb);

    draw_text(fb, 28, 22, "GAMEPLAYOS", 2, 235, 247, 255);
    draw_text(fb, 30, 42, "XMB INTERFACE", 1, 95, 190, 235);
    fill_rect(fb, 28, 61, width - 56, 1, 75, 135, 190);

    top = 112;
    left = width / 12;
    for (i = 0; i < CATEGORY_COUNT; ++i) {
        int x = left + i * ((width - 2 * left) / (CATEGORY_COUNT - 1));
        draw_category(fb, i, x, top, i == category);
    }

    fill_rect(fb, 28, 160, width - 56, 1, 65, 115, 160);
    draw_text(fb, 52, 181, categories[category], 2, 175, 225, 255);

    row_height = height < 400 ? 43 : 52;
    top = 222;
    for (i = 0; i < 4; ++i) {
        int y = top + i * row_height;
        if (i == selected) {
            fill_rect(fb, 36, y - 7, width - 72, row_height - 2,
                      20, 85, 145);
            fill_rect(fb, 36, y - 7, 4, row_height - 2,
                      80, 220, 255);
            draw_text(fb, 57, y + 5, ">", 1, 255, 255, 255);
            draw_text(fb, 79, y + 5, items[category][i], 1,
                      255, 255, 255);
        } else {
            draw_text(fb, 79, y + 5, items[category][i], 1,
                      175, 205, 230);
        }
    }

    fill_rect(fb, 28, height - 34, width - 56, 1, 65, 115, 160);
    draw_text(fb, 30, height - 24, "ARROWS SELECT", 1,
              125, 170, 205);
    draw_text(fb, width - 112, height - 24, "GAMEPLAYOS", 1,
              125, 170, 205);

    (void)msync(fb->memory, fb->length, MS_ASYNC);
}

static int open_framebuffer(struct framebuffer *fb)
{
    memset(fb, 0, sizeof(*fb));
    fb->fd = open("/dev/fb0", O_RDWR);
    if (fb->fd < 0) return -1;
    if (ioctl(fb->fd, FBIOGET_FSCREENINFO, &fb->fix) < 0 ||
        ioctl(fb->fd, FBIOGET_VSCREENINFO, &fb->var) < 0) return -1;

    fb->bytes_per_pixel = (fb->var.bits_per_pixel + 7u) / 8u;
    if ((fb->var.bits_per_pixel != 16 &&
         fb->var.bits_per_pixel != 24 &&
         fb->var.bits_per_pixel != 32) ||
        fb->bytes_per_pixel == 0 || fb->fix.smem_len == 0)
        return -1;

    fb->length = fb->fix.smem_len;
    fb->memory = mmap(NULL, fb->length, PROT_READ | PROT_WRITE,
                      MAP_SHARED, fb->fd, 0);
    if (fb->memory == MAP_FAILED) {
        fb->memory = NULL;
        return -1;
    }
    return 0;
}

static int open_input_devices(int fds[MAX_INPUTS])
{
    int i, count = 0;
    char path[32];

    for (i = 0; i < MAX_INPUTS; ++i) {
        (void)snprintf(path, sizeof(path), "/dev/input/event%d", i);
        fds[i] = open(path, O_RDONLY | O_NONBLOCK);
        if (fds[i] >= 0) ++count;
    }
    return count;
}

int main(void)
{
    struct framebuffer fb;
    struct pollfd pollfds[MAX_INPUTS];
    int input_fds[MAX_INPUTS];
    int category = 0, selected = 0;
    int i, count;

    if (open_framebuffer(&fb) < 0) {
        int fd = open("/dev/console", O_WRONLY);
        if (fd >= 0) {
            static const char msg[] =
                "\n[gameplayOS] /dev/fb0 is unavailable.\n";
            (void)write(fd, msg, sizeof(msg) - 1);
            close(fd);
        }
        for (;;) pause();
    }

    for (i = 0; i < MAX_INPUTS; ++i) input_fds[i] = -1;
    (void)open_input_devices(input_fds);

    count = 0;
    for (i = 0; i < MAX_INPUTS; ++i) {
        if (input_fds[i] >= 0) {
            pollfds[count].fd = input_fds[i];
            pollfds[count].events = POLLIN;
            pollfds[count].revents = 0;
            ++count;
        }
    }

    draw_screen(&fb, category, selected);

    for (;;) {
        int result = poll(pollfds, (nfds_t)count, 500);
        int changed = 0;

        if (result <= 0) continue;

        for (i = 0; i < count; ++i) {
            if (pollfds[i].revents & POLLIN) {
                struct input_event event;
                while (read(pollfds[i].fd, &event, sizeof(event)) ==
                       (ssize_t)sizeof(event)) {
                    if (event.type != EV_KEY || event.value != 1) continue;

                    switch (event.code) {
                    case KEY_LEFT:
                        category = (category + CATEGORY_COUNT - 1) %
                                   CATEGORY_COUNT;
                        selected = 0;
                        changed = 1;
                        break;
                    case KEY_RIGHT:
                        category = (category + 1) % CATEGORY_COUNT;
                        selected = 0;
                        changed = 1;
                        break;
                    case KEY_UP:
                        selected = (selected + 3) % 4;
                        changed = 1;
                        break;
                    case KEY_DOWN:
                        selected = (selected + 1) % 4;
                        changed = 1;
                        break;
                    default:
                        break;
                    }
                }
            }
        }

        if (changed) draw_screen(&fb, category, selected);
    }
}
