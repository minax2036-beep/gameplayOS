#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

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

static void draw_tile(struct framebuffer *fb, int x, int y, int w, int h,
                      const char *label, int selected)
{
    fill_rect(fb, x, y, w, h, 25, 38, 59);
    fill_rect(fb, x, y, w, 3, selected ? 50 : 39,
              selected ? 205 : 105, selected ? 220 : 150);
    if (selected) {
        fill_rect(fb, x, y, 3, h, 50, 205, 220);
        fill_rect(fb, x + w - 3, y, 3, h, 50, 205, 220);
        fill_rect(fb, x, y + h - 3, w, 3, 50, 205, 220);
    }
    draw_text(fb, x + 22, y + (h - 14) / 2, label, 2,
              selected ? 255 : 215, selected ? 255 : 225,
              selected ? 255 : 238);
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

int main(void)
{
    struct framebuffer fb;
    char status[96];
    int width, height, margin, gap, tile_w, tile_h, start_y;

    if (open_framebuffer(&fb) < 0) {
        int fd = open("/dev/console", O_WRONLY);
        if (fd >= 0) {
            static const char msg[] =
                "\n[gameplayOS] /dev/fb0 is unavailable. "
                "Enable DRM fbdev emulation to show the launcher.\n";
            (void)write(fd, msg, sizeof(msg) - 1);
            close(fd);
        }
        for (;;) pause();
    }

    width = (int)fb.var.xres;
    height = (int)fb.var.yres;
    fill_rect(&fb, 0, 0, width, height, 11, 17, 29);

    fill_rect(&fb, 0, 0, width, 7, 35, 190, 215);
    draw_text(&fb, 28, 28, "GAMEPLAYOS", 3, 245, 250, 255);
    draw_text(&fb, 30, 58, "LAUNCHER", 1, 115, 190, 210);

    margin = width / 20;
    if (margin < 16) margin = 16;
    gap = width / 35;
    if (gap < 12) gap = 12;
    tile_w = (width - margin * 2 - gap) / 2;
    tile_h = (height - 150) / 2;
    if (tile_h > 105) tile_h = 105;
    if (tile_h < 48) tile_h = 48;
    start_y = height / 3;
    if (start_y + tile_h * 2 + gap > height - 35)
        start_y = height - tile_h * 2 - gap - 35;

    draw_tile(&fb, margin, start_y, tile_w, tile_h, "GAMES", 1);
    draw_tile(&fb, margin + tile_w + gap, start_y,
              tile_w, tile_h, "FILES", 0);
    draw_tile(&fb, margin, start_y + tile_h + gap,
              tile_w, tile_h, "SETTINGS", 0);
    draw_tile(&fb, margin + tile_w + gap, start_y + tile_h + gap,
              tile_w, tile_h, "SYSTEM INFO", 0);

    fill_rect(&fb, 0, height - 28, width, 1, 45, 65, 85);
    draw_text(&fb, margin, height - 20, "HDMI EDITION", 1,
              115, 145, 165);
    (void)snprintf(status, sizeof(status), "DISPLAY %ux%u",
                   fb.var.xres, fb.var.yres);
    draw_text(&fb, width - margin - (int)strlen(status) * 6,
              height - 20, status, 1, 115, 145, 165);

    (void)msync(fb.memory, fb.length, MS_SYNC);
    for (;;) pause();
}
