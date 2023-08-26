#pragma once
#include <tice.h>
#include <graphx.h>
#include <string.h>
#include "textures.h"
#include "world.h"

#define BLOCK_WIDTH  32
#define BLOCK_HALF_WIDTH  (BLOCK_WIDTH / 2)
#define BLOCK_HEIGHT 31

#define SCROLL_SPEED 8

#define LCD_CNT (LCD_WIDTH * LCD_HEIGHT)

#define BUFFER_1 0xD40000
#define BUFFER_2 0xD52C00
#define BUFFER_SWP (BUFFER_1 ^ BUFFER_2)

extern uint8_t* VRAM;

extern int24_t scroll_x;
extern int24_t scroll_y;

// Bounds to draw within
extern uint16_t draw_x0;
extern uint16_t draw_y0;
extern uint16_t draw_x1;
extern uint16_t draw_y1;

// Draws a left facing triangle and checks every pixel to ensure nothing gets drawn out of bounds
void draw_left_triangle_clipped(int24_t x0, int24_t y0, uint8_t *tex, uint8_t *shadow_mask, uint8_t *water_mask);

// Draws a right facing triangle and checks every pixel to ensure nothing gets drawn out of bounds
void draw_right_triangle_clipped(int24_t x0, int24_t y0, uint8_t *tex, uint8_t *shadow_mask, uint8_t *water_mask);

// Draws a left facing triangle
void draw_left_triangle(int24_t x0, int24_t y0, uint8_t *tex, uint8_t *shadow_mask, uint8_t *water_mask);

// Draws a right facing triangle
void draw_right_triangle(int24_t x0, int24_t y0, uint8_t *tex, uint8_t *shadow_mask, uint8_t *water_mask);


// Draws a left facing filled triangle and checks every pixel to ensure nothing gets drawn out of bounds
void draw_left_triangle_clipped(int24_t x0, int24_t y0, uint8_t *water_mask);

// Draws a right facing filled triangle and checks every pixel to ensure nothing gets drawn out of bounds
void draw_right_triangle_clipped(int24_t x0, int24_t y0, uint8_t *water_mask);

// Draws a left facing filled triangle
void draw_left_triangle(int24_t x0, int24_t y0, uint8_t *water_mask);

// Draws a right facing filled triangle
void draw_right_triangle(int24_t x0, int24_t y0, uint8_t *water_mask);


// Draws a left facing triangle
void draw_left_triangle(int24_t x0, int24_t y0, uint8_t *tex, uint8_t flags);

// Draws a right facing triangle
void draw_right_triangle(int24_t x0, int24_t y0, uint8_t *tex, uint8_t flags);


void draw_block(int24_t x, int24_t y, uint8_t *tex);

void draw_block(uint8_t x, uint8_t y, uint8_t z, uint8_t *tex);

void draw_tri_grid(world_t &world);

void scroll_view(world_t &world, int24_t x, int24_t y);

void dim_screen();

void draw_num(int24_t x, int24_t y, uint8_t n);

void empty_draw_region(void);

void expand_draw_region(uint8_t x, uint8_t y, uint8_t z);