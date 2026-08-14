#include "ssd1309_anim.h"

#include <math.h>
#include <string.h>

#include "board_config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ssd1309.h"

static const char *TAG = "ssd1309_anim";

/* 稀疏背景：少而清晰 */
#define STAR_COUNT     28
#define DUST_COUNT     6
#define ASTEROID_SLOTS 2

/* 右侧入画起点、左侧完全离场余量；阶段帧数按速度自动算满全程 */
#define ANIM_SPAWN_X    (OLED_H_RES + 36)
#define ANIM_CLEAR_L    20
#define PHASE_LEN_SCROLL(half_w, speed_px) \
    ((ANIM_SPAWN_X + (half_w) + ANIM_CLEAR_L + (speed_px) - 1) / (speed_px))

/* 大循环 = 各阶段帧数之和（每段都等物体出屏后再切） */
#define SCENE_CYCLE ( \
    38 + PHASE_LEN_SCROLL(20, 2) + PHASE_LEN_SCROLL(10, 9) + PHASE_LEN_SCROLL(13, 2) + \
    PHASE_LEN_SCROLL(8, 2) + PHASE_LEN_SCROLL(14, 2) + PHASE_LEN_SCROLL(22, 4) + \
    PHASE_LEN_SCROLL(8, 3) + PHASE_LEN_SCROLL(22, 3) + PHASE_LEN_SCROLL(9, 3) + \
    PHASE_LEN_SCROLL(14, 2) + PHASE_LEN_SCROLL(12, 2))

typedef enum {
    PHASE_CRUISE = 0,
    PHASE_PLANET_RING,
    PHASE_SHOOTING_STAR,
    PHASE_PLANET_GAS,
    PHASE_ASTEROIDS,
    PHASE_PLANET_LARGE,
    PHASE_COMET,
    PHASE_PLANET_MOON,
    PHASE_STATION,
    PHASE_PLANET_CRATER,
    PHASE_PLANET_TWIN,
    PHASE_PLANET_CRESCENT,
    PHASE_COUNT
} scene_phase_t;

typedef struct {
    int16_t x;
    int16_t y;
    uint8_t speed;
    uint8_t bright;
} star_t;

typedef struct {
    int16_t y;
    uint8_t phase;
} dust_t;

typedef struct {
    int16_t y;
    uint8_t speed;
    uint8_t size;
    uint16_t phase;
} asteroid_t;

static star_t s_stars[STAR_COUNT];
static dust_t s_dust[DUST_COUNT];
static asteroid_t s_asteroids[ASTEROID_SLOTS];
static bool s_scene_ready;

static void anim_fb_set_pixel(uint8_t *fb, int x, int y, bool on)
{
    if (x < 0 || x >= OLED_H_RES || y < 0 || y >= OLED_V_RES) {
        return;
    }
    uint8_t *byte = &fb[x + (y / 8) * OLED_H_RES];
    if (on) {
        *byte |= (uint8_t)(1u << (y % 8));
    } else {
        *byte &= (uint8_t)~(1u << (y % 8));
    }
}

static void anim_fb_clear(uint8_t *fb)
{
    memset(fb, 0, SSD1309_FB_SIZE);
}

static const uint16_t s_phase_len[PHASE_COUNT] = {
    38,                           /* 巡航 */
    PHASE_LEN_SCROLL(20, 2),      /* 土星 */
    PHASE_LEN_SCROLL(10, 9),      /* 流星 */
    PHASE_LEN_SCROLL(13, 2),      /* 木星 */
    PHASE_LEN_SCROLL(8, 2),       /* 小行星带 */
    PHASE_LEN_SCROLL(14, 2),      /* 地球 */
    PHASE_LEN_SCROLL(22, 4),      /* 彗星 */
    PHASE_LEN_SCROLL(8, 3),       /* 月球 */
    PHASE_LEN_SCROLL(22, 3),      /* 空间站 */
    PHASE_LEN_SCROLL(9, 3),       /* 火星 */
    PHASE_LEN_SCROLL(14, 2),      /* 天王星 */
    PHASE_LEN_SCROLL(12, 2),      /* 金星 */
};

static scene_phase_t anim_current_phase(int frame)
{

    int t = frame % SCENE_CYCLE;
    int acc = 0;
    for (int p = 0; p < PHASE_COUNT; p++) {
        acc += s_phase_len[p];
        if (t < acc) {
            return (scene_phase_t)p;
        }
    }
    return PHASE_CRUISE;
}

static int anim_phase_local(int frame)
{
    int t = frame % SCENE_CYCLE;
    int acc = 0;
    for (int p = 0; p < PHASE_COUNT; p++) {
        if (t < acc + s_phase_len[p]) {
            return t - acc;
        }
        acc += s_phase_len[p];
    }
    return 0;
}

static int anim_scroll_cx(int local_t, int speed_px)
{
    return ANIM_SPAWN_X - local_t * speed_px;
}

static void anim_scene_init(void)
{
    if (s_scene_ready) {
        return;
    }

    for (int i = 0; i < STAR_COUNT; i++) {
        s_stars[i].x = (int16_t)((i * 47 + 11) % OLED_H_RES);
        s_stars[i].y = (int16_t)((i * 37 + 5) % OLED_V_RES);
        s_stars[i].speed = (uint8_t)(1 + (i % 3));
        s_stars[i].bright = (uint8_t)(i % 7 == 0);
    }

    for (int i = 0; i < DUST_COUNT; i++) {
        s_dust[i].y = (int16_t)((i * 29 + 9) % OLED_V_RES);
        s_dust[i].phase = (uint8_t)(i * 17);
    }

    for (int i = 0; i < ASTEROID_SLOTS; i++) {
        s_asteroids[i].y = (int16_t)(10 + i * 22);
        s_asteroids[i].speed = (uint8_t)(1 + i);
        s_asteroids[i].size = (uint8_t)(i % 2);
        s_asteroids[i].phase = (uint16_t)(i * 120 + 40);
    }

    s_scene_ready = true;
}

static void anim_draw_dust(uint8_t *fb, int frame, bool enable)
{
    if (!enable) {
        return;
    }
    for (int i = 0; i < DUST_COUNT; i++) {
        int x = OLED_H_RES - (frame / 4 + s_dust[i].phase);
        while (x < -2) {
            x += OLED_H_RES + 24;
        }
        anim_fb_set_pixel(fb, x, s_dust[i].y, true);
    }
}

static void anim_draw_stars(uint8_t *fb, int frame)
{
    for (int i = 0; i < STAR_COUNT; i++) {
        star_t *st = &s_stars[i];
        st->x -= st->speed;
        if (st->x < 0) {
            st->x = OLED_H_RES - 1;
            st->y = (int16_t)((frame + i * 23) % OLED_V_RES);
        }

        anim_fb_set_pixel(fb, st->x, st->y, true);
        if (st->bright && (frame + i) % 20 < 10) {
            anim_fb_set_pixel(fb, st->x + 1, st->y, true);
        }
    }
}

static bool anim_pt_in_circle(int dx, int dy, int r)
{
    return dx * dx + dy * dy <= r * r;
}

/* 土星：倾斜环，后半环 → 球体 → 前半环，卡西尼缝 */
static void anim_draw_saturn(uint8_t *fb, int cx, int cy, int r, int local_t)
{
    int ring_y = cy + r / 4;
    int ring_rx = r + 10;

    if (cx < -ring_rx - 2 || cx > OLED_H_RES + ring_rx + 2) {
        return;
    }

    for (int dx = -ring_rx; dx <= ring_rx; dx++) {
        int dy = (dx * dx) / (ring_rx - 2);
        int y = ring_y - 2 + dy / 3;
        if (y >= cy) {
            continue;
        }
        if (dx > -r / 2 && dx < r / 2 + 2) {
            continue;
        }
        if ((local_t / 4 + dx) % 14 == 0) {
            continue;
        }
        anim_fb_set_pixel(fb, cx + dx, y, true);
        anim_fb_set_pixel(fb, cx + dx, y + 1, true);
    }

    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            if (!anim_pt_in_circle(dx, dy, r)) {
                continue;
            }
            if (dy % 4 == 0) {
                continue;
            }
            anim_fb_set_pixel(fb, cx + dx, cy + dy, true);
        }
    }
    anim_fb_set_pixel(fb, cx - 2, cy - 3, true);
    anim_fb_set_pixel(fb, cx + 3, cy + 2, true);

    for (int dx = -ring_rx; dx <= ring_rx; dx++) {
        int dy = (dx * dx) / (ring_rx - 2);
        int y = ring_y + 1 + dy / 3;
        if (y < cy - 1) {
            continue;
        }
        if (dx > -r / 2 && dx < r / 2 + 2) {
            continue;
        }
        if ((local_t / 4 + dx) % 14 == 0) {
            continue;
        }
        anim_fb_set_pixel(fb, cx + dx, y, true);
        anim_fb_set_pixel(fb, cx + dx, y + 1, true);
    }
}

/* 木星：云带 + 大红斑 */
static void anim_draw_jupiter(uint8_t *fb, int cx, int cy, int r)
{
    if (cx < -r - 2 || cx > OLED_H_RES + r + 2) {
        return;
    }
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            if (!anim_pt_in_circle(dx, dy, r)) {
                continue;
            }
            int band = (dy + r) % 5;
            if (band == 1 || band == 3) {
                continue;
            }
            if (dx > r / 3 && dx < r / 3 + 3 && dy > -2 && dy < 3) {
                continue;
            }
            anim_fb_set_pixel(fb, cx + dx, cy + dy, true);
        }
    }
    for (int dx = 0; dx < 4; dx++) {
        for (int dy = -1; dy <= 2; dy++) {
            anim_fb_set_pixel(fb, cx + r / 3 + dx, cy + dy, true);
        }
    }
    anim_fb_set_pixel(fb, cx - r / 3, cy - r / 4, true);
}

/* 地球：大陆块 + 云带 */
static void anim_draw_earth(uint8_t *fb, int cx, int cy, int r)
{
    if (cx < -r - 2 || cx > OLED_H_RES + r + 2) {
        return;
    }
    static const int8_t land[][2] = {
        {-3, -4}, {-2, -5}, {-1, -4}, {0, -3}, {1, -2}, {2, -1}, {3, 0},
        {4, 1}, {3, 2}, {2, 3}, {1, 4}, {0, 3}, {-1, 2}, {-2, 0}, {-3, -2},
        {2, -3}, {3, -2},
    };
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            if (!anim_pt_in_circle(dx, dy, r)) {
                continue;
            }
            bool is_land = false;
            for (size_t i = 0; i < sizeof(land) / sizeof(land[0]); i++) {
                if (dx == land[i][0] || dx == land[i][0] + 1) {
                    if (dy == land[i][1] || dy == land[i][1] + 1) {
                        is_land = true;
                        break;
                    }
                }
            }
            if (is_land) {
                continue;
            }
            if (dy == -r + 2 && dx > -r / 2) {
                continue;
            }
            anim_fb_set_pixel(fb, cx + dx, cy + dy, true);
        }
    }
}

/* 月球：环形山 + 月海（暗区） */
static void anim_draw_moon(uint8_t *fb, int cx, int cy, int r)
{
    if (cx < -r - 2 || cx > OLED_H_RES + r + 2) {
        return;
    }
    static const int8_t mare[][2] = {{0, 0}, {1, 0}, {0, 1}, {-1, 1}, {2, 1}, {1, 2}};
    static const int8_t cr[][2] = {
        {-3, -2}, {3, -3}, {2, 2}, {-2, 3}, {4, 0}, {-4, 1}, {0, -4},
    };
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            if (!anim_pt_in_circle(dx, dy, r)) {
                continue;
            }
            bool skip = false;
            for (size_t i = 0; i < sizeof(mare) / sizeof(mare[0]); i++) {
                if (dx == mare[i][0] && dy == mare[i][1]) {
                    skip = true;
                    break;
                }
            }
            for (size_t i = 0; i < sizeof(cr) / sizeof(cr[0]); i++) {
                if (dx == cr[i][0] && dy == cr[i][1]) {
                    skip = true;
                    break;
                }
                if (dx == cr[i][0] + 1 && dy == cr[i][1]) {
                    skip = true;
                    break;
                }
            }
            if (!skip) {
                anim_fb_set_pixel(fb, cx + dx, cy + dy, true);
            }
        }
    }
    anim_fb_set_pixel(fb, cx - 3, cy - 1, true);
    anim_fb_set_pixel(fb, cx - 2, cy - 1, true);
}

/* 火星：锈色球 + 多处陨石坑 */
static void anim_draw_mars(uint8_t *fb, int cx, int cy, int r)
{
    if (cx < -r - 2 || cx > OLED_H_RES + r + 2) {
        return;
    }
    static const int8_t cr[][2] = {
        {-3, -2}, {-1, -3}, {2, -2}, {4, 0}, {1, 3}, {-2, 2}, {0, 0}, {3, 2},
    };
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            if (!anim_pt_in_circle(dx, dy, r)) {
                continue;
            }
            bool hole = false;
            for (size_t i = 0; i < sizeof(cr) / sizeof(cr[0]); i++) {
                int ddx = dx - cr[i][0];
                int ddy = dy - cr[i][1];
                if (ddx * ddx + ddy * ddy <= 2) {
                    hole = true;
                    break;
                }
            }
            if ((dx == 2 || dx == 3) && dy == -1) {
                hole = true;
            }
            if (!hole) {
                anim_fb_set_pixel(fb, cx + dx, cy + dy, true);
            }
        }
    }
}

/* 天王星：侧向倾斜环 + 淡色球 */
static void anim_draw_uranus(uint8_t *fb, int cx, int cy, int r)
{
    int ring_rx = r + 7;

    if (cx < -ring_rx - 2 || cx > OLED_H_RES + ring_rx + 2) {
        return;
    }
    for (int dx = -ring_rx; dx <= ring_rx; dx++) {
        int y = cy + (dx * dx) / (ring_rx + 4) - 3;
        if ((dx + 3) % 9 == 0) {
            continue;
        }
        anim_fb_set_pixel(fb, cx + dx, y, true);
        anim_fb_set_pixel(fb, cx + dx, y + 1, true);
    }
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            if (anim_pt_in_circle(dx, dy, r)) {
                anim_fb_set_pixel(fb, cx + dx, cy + dy, true);
            }
        }
    }
}

/* 金星：厚重云层条纹 */
static void anim_draw_venus(uint8_t *fb, int cx, int cy, int r)
{
    if (cx < -r - 2 || cx > OLED_H_RES + r + 2) {
        return;
    }
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            if (!anim_pt_in_circle(dx, dy, r)) {
                continue;
            }
            if ((dy + dx / 2) % 3 == 0) {
                continue;
            }
            anim_fb_set_pixel(fb, cx + dx, cy + dy, true);
        }
    }
    for (int dx = -r + 2; dx <= r - 1; dx++) {
        if (dx % 4 == 0) {
            anim_fb_set_pixel(fb, cx + dx, cy - 2, true);
            anim_fb_set_pixel(fb, cx + dx, cy + 2, true);
        }
    }
}

static void anim_draw_far_planet(uint8_t *fb, scene_phase_t phase, int local_t, int frame)
{
    int scroll = 2;
    int cx;
    int cy;

    (void)frame;

    switch (phase) {
    case PHASE_PLANET_RING:
        cx = anim_scroll_cx(local_t, scroll);
        anim_draw_saturn(fb, cx, 14, 11, local_t);
        break;
    case PHASE_PLANET_GAS:
        cx = anim_scroll_cx(local_t, scroll);
        anim_draw_jupiter(fb, cx, 20, 10);
        break;
    case PHASE_PLANET_LARGE:
        cx = anim_scroll_cx(local_t, 2);
        anim_draw_earth(fb, cx, 24, 12);
        break;
    case PHASE_PLANET_MOON:
        cx = anim_scroll_cx(local_t, 3);
        anim_draw_moon(fb, cx, 48, 7);
        break;
    case PHASE_PLANET_CRATER:
        cx = anim_scroll_cx(local_t, 3);
        anim_draw_mars(fb, cx, 28, 8);
        break;
    case PHASE_PLANET_TWIN:
        cx = anim_scroll_cx(local_t, 2);
        anim_draw_uranus(fb, cx, 22, 9);
        break;
    case PHASE_PLANET_CRESCENT:
        cx = anim_scroll_cx(local_t, 2);
        anim_draw_venus(fb, cx, 18, 9);
        break;
    default:
        break;
    }
}

static void anim_draw_asteroid(uint8_t *fb, int ax, int ay, uint8_t size)
{
    if (size == 0) {
        static const int8_t s0[][2] = {{0, 0}, {1, 0}, {2, 1}, {2, 2}, {1, 2}, {0, 1}};
        for (size_t i = 0; i < sizeof(s0) / sizeof(s0[0]); i++) {
            anim_fb_set_pixel(fb, ax + s0[i][0], ay + s0[i][1], true);
        }
        return;
    }
    static const int8_t s1[][2] = {
        {1, 0}, {2, 0}, {3, 0}, {4, 1}, {4, 2}, {3, 3}, {2, 4},
        {1, 4}, {0, 3}, {0, 2}, {0, 1}, {2, 2},
    };
    for (size_t i = 0; i < sizeof(s1) / sizeof(s1[0]); i++) {
        anim_fb_set_pixel(fb, ax + s1[i][0], ay + s1[i][1], true);
    }
}

static void anim_draw_shooting_star(uint8_t *fb, int local_t)
{
    int sx = ANIM_SPAWN_X - 36 - local_t * 9;
    int sy = 6 + local_t * 2;
    /* 仅当整段尾迹完全离开屏幕左侧/下侧才不画 */
    if (sx + 16 < 0 && sy >= OLED_V_RES) {
        return;
    }
    for (int i = 0; i < 8; i++) {
        anim_fb_set_pixel(fb, sx - i * 2, sy - i, true);
    }
}

static void anim_draw_space_station(uint8_t *fb, int local_t)
{
    int sx = anim_scroll_cx(local_t, 3);
    int sy = 20;

    for (int x = 0; x < 20; x++) {
        anim_fb_set_pixel(fb, sx + x, sy + 3, true);
    }
    for (int y = 1; y <= 5; y++) {
        anim_fb_set_pixel(fb, sx + 3, sy + y, true);
        anim_fb_set_pixel(fb, sx + 16, sy + y, true);
    }
    anim_fb_set_pixel(fb, sx + 9, sy, true);
    anim_fb_set_pixel(fb, sx + 9, sy + 6, true);
    if (local_t % 10 < 5) {
        anim_fb_set_pixel(fb, sx + 1, sy + 3, true);
        anim_fb_set_pixel(fb, sx + 18, sy + 3, true);
    }
}

static void anim_draw_comet(uint8_t *fb, int local_t)
{
    int cx = ANIM_SPAWN_X - 20 - local_t * 4;
    int cy = 10 + local_t;
    if (cx + 22 < 0) {
        return;
    }
    anim_fb_set_pixel(fb, cx, cy, true);
    anim_fb_set_pixel(fb, cx + 1, cy, true);
    for (int i = 1; i <= 10; i++) {
        if ((i % 2) != 0) {
            anim_fb_set_pixel(fb, cx + i * 2, cy + i / 2, true);
        }
    }
}

/* 全部场景物体均为背景：只在飞船之前绘制 */
static void anim_draw_phase_event(uint8_t *fb, scene_phase_t phase, int local_t, int frame)
{
    (void)frame;

    switch (phase) {
    case PHASE_PLANET_RING:
    case PHASE_PLANET_GAS:
    case PHASE_PLANET_LARGE:
    case PHASE_PLANET_MOON:
    case PHASE_PLANET_CRATER:
    case PHASE_PLANET_TWIN:
    case PHASE_PLANET_CRESCENT:
        anim_draw_far_planet(fb, phase, local_t, frame);
        break;
    case PHASE_SHOOTING_STAR:
        anim_draw_shooting_star(fb, local_t);
        break;
    case PHASE_ASTEROIDS:
        for (int i = 0; i < ASTEROID_SLOTS; i++) {
            int x = anim_scroll_cx(local_t, 2) - (int)(i * 58);
            if (x > -10) {
                anim_draw_asteroid(fb, x, s_asteroids[i].y, s_asteroids[i].size);
            }
        }
        break;
    case PHASE_COMET:
        anim_draw_comet(fb, local_t);
        break;
    case PHASE_STATION:
        anim_draw_space_station(fb, local_t);
        break;
    default:
        break;
    }
}

static void anim_draw_spaceship(uint8_t *fb, int ox, int oy, int frame)
{
    for (int x = 2; x <= 16; x++) {
        anim_fb_set_pixel(fb, ox + x, oy + 5, true);
    }
    for (int x = 5; x <= 14; x++) {
        anim_fb_set_pixel(fb, ox + x, oy + 4, true);
        anim_fb_set_pixel(fb, ox + x, oy + 6, true);
    }
    for (int x = 8; x <= 12; x++) {
        anim_fb_set_pixel(fb, ox + x, oy + 3, true);
        anim_fb_set_pixel(fb, ox + x, oy + 7, true);
    }

    anim_fb_set_pixel(fb, ox + 17, oy + 5, true);
    anim_fb_set_pixel(fb, ox + 18, oy + 4, true);
    anim_fb_set_pixel(fb, ox + 18, oy + 5, true);
    anim_fb_set_pixel(fb, ox + 18, oy + 6, true);
    anim_fb_set_pixel(fb, ox + 19, oy + 5, true);
    anim_fb_set_pixel(fb, ox + 20, oy + 5, true);

    if ((frame / 6) % 2) {
        anim_fb_set_pixel(fb, ox + 11, oy + 4, false);
        anim_fb_set_pixel(fb, ox + 12, oy + 4, false);
        anim_fb_set_pixel(fb, ox + 11, oy + 5, false);
        anim_fb_set_pixel(fb, ox + 12, oy + 5, false);
    }

    anim_fb_set_pixel(fb, ox + 7, oy + 2, true);
    anim_fb_set_pixel(fb, ox + 8, oy + 1, true);
    anim_fb_set_pixel(fb, ox + 9, oy + 1, true);
    anim_fb_set_pixel(fb, ox + 10, oy + 2, true);
    anim_fb_set_pixel(fb, ox + 7, oy + 8, true);
    anim_fb_set_pixel(fb, ox + 8, oy + 9, true);
    anim_fb_set_pixel(fb, ox + 9, oy + 9, true);
    anim_fb_set_pixel(fb, ox + 10, oy + 8, true);

    if (frame % 12 < 6) {
        anim_fb_set_pixel(fb, ox + 2, oy + 4, true);
        anim_fb_set_pixel(fb, ox + 2, oy + 6, true);
    }
}

static void anim_draw_engine_flame(uint8_t *fb, int ship_x, int ship_y, int frame)
{
    int flick = frame & 3;
    int base_x = ship_x - 3;
    int base_y = ship_y + 4;

    for (int i = 0; i < 4 + flick; i++) {
        anim_fb_set_pixel(fb, base_x - i, base_y, true);
        anim_fb_set_pixel(fb, base_x - i, base_y + 1, true);
    }
    for (int i = 1; i <= 5; i++) {
        if ((i % 2) == 0) {
            int tx = base_x - 4 - i * 2;
            int ty = base_y + ((i % 3) - 1);
            anim_fb_set_pixel(fb, tx, ty, true);
        }
    }
}

static void anim_space_scene(uint8_t *fb, int frame)
{
    anim_fb_clear(fb);
    anim_scene_init();

    scene_phase_t phase = anim_current_phase(frame);
    int local_t = anim_phase_local(frame);

    int ship_x = 32;
    int ship_y = OLED_V_RES / 2 - 5 + (int)(2.0f * sinf(frame * 0.05f));

    /* 从远到近：星场 → 尾焰 → 所有掠过物 → 飞船（最前） */
    anim_draw_stars(fb, frame);
    anim_draw_dust(fb, frame, phase == PHASE_CRUISE || phase == PHASE_ASTEROIDS);
    anim_draw_engine_flame(fb, ship_x, ship_y, frame);
    anim_draw_phase_event(fb, phase, local_t, frame);
    anim_draw_spaceship(fb, ship_x, ship_y, frame);
}

static void ssd1309_anim_task(void *arg)
{
    (void)arg;
    static uint8_t fb[SSD1309_FB_SIZE];
    int frame = 0;

    ESP_LOGI(TAG, "spaceship scene started (phased)");

    while (true) {
        anim_space_scene(fb, frame);

        esp_err_t err = ssd1309_flush(fb);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "flush failed: %s", esp_err_to_name(err));
        }

        frame++;
        vTaskDelay(pdMS_TO_TICKS(35));
    }
}

esp_err_t ssd1309_anim_start(void)
{
    BaseType_t ok = xTaskCreate(ssd1309_anim_task, "ssd1309_anim", 5120, NULL, 5, NULL);
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "create task failed");
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}
