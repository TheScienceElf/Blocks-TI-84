#include <tice.h>
#include <graphx.h>
#include <string.h>
#include "textures.h"
#include "draw.h"

// The base pointer for VRAM
uint8_t* buffer1 = (uint8_t*)BUFFER_1;
uint8_t* buffer2 = (uint8_t*)BUFFER_2;

uint8_t* VRAM = buffer1;

int24_t scroll_x = 0;
int24_t scroll_y = 0;

// Bounds to draw within
uint16_t draw_x0 = 0;
uint16_t draw_y0 = 0;
uint16_t draw_x1 = LCD_WIDTH;
uint16_t draw_y1 = LCD_HEIGHT;


// Copies pixels from a texture line into a VRAM line, applying a constant mask across all pixels
// In this case, the texture can be transparent, with blank pixels represented as a zero
void copy_tex_line(uint8_t *dest, uint8_t *tex, uint8_t flags, int length) {
    for(int i = 0; i < length; i++) {
        if(tex[i])
            dest[i] = tex[i] | flags;
    }
}

// Copies pixels from a texture line into a VRAM line, along with applying shadow and water mask terms
void copy_tex_line(uint8_t *dest, uint8_t *tex, uint8_t *shadow_mask, uint8_t *water_mask, int length) {
    for(int i = 0; i < length; i++) {
        dest[i] = tex[i] | shadow_mask[i] | water_mask[i];
    }
}

// Copies SKY colored pixels with the water mask applied into a VRAM line
void copy_tex_line(uint8_t *dest, uint8_t *water_mask, int length) {
    for(int i = 0; i < length; i++) {
        dest[i] = SKY | water_mask[i];
    }
}


// Draws a left facing triangle and checks every pixel to ensure nothing gets drawn out of bounds
void draw_left_triangle_clipped(int24_t x0, int24_t y0, uint8_t *tex, uint8_t *shadow_mask, uint8_t *water_mask) {
    for(int row = 1; row <= 8; row++) {
        for(int dx = x0 - 2 * row; dx < x0; dx++) {
            uint8_t color = *(tex++) | *(shadow_mask++) | *(water_mask++);

            int24_t dy = (row - 1 + y0);

            if(dy >= 0 && dy < LCD_HEIGHT && dx >= 0 && dx < LCD_WIDTH)
                VRAM[LCD_WIDTH * dy + dx] = color;
        }
    }

    for(int row = 1; row < 8; row++) {
        for(int dx = x0 - 16 + 2 * row; dx < x0; dx++) {
            uint8_t color = *(tex++) | *(shadow_mask++) | *(water_mask++);

            int24_t dy = (8 + row - 1 + y0);

            if(dy >= 0 && dy < LCD_HEIGHT && dx >= 0 && dx < LCD_WIDTH)
                VRAM[LCD_WIDTH * dy + dx] = color;
        }
    }
}

// Draws a right facing triangle and checks every pixel to ensure nothing gets drawn out of bounds
void draw_right_triangle_clipped(int24_t x0, int24_t y0, uint8_t *tex, uint8_t *shadow_mask, uint8_t *water_mask) {
    for(int row = 1; row <= 8; row++) {
        for(int dx = x0; dx < x0 + 2 * row; dx++) {
            uint8_t color = *(tex++) | *(shadow_mask++) | *(water_mask++);

            int24_t dy = (row - 1 + y0);

            if(dy >= 0 && dy < LCD_HEIGHT && dx >= 0 && dx < LCD_WIDTH)
                VRAM[LCD_WIDTH * dy + dx] = color;
        }
    }

    for(int row = 1; row < 8; row++) {
        for(int dx = x0; dx < x0 + 16 - 2 * row; dx++) {
            uint8_t color = *(tex++) | *(shadow_mask++) | *(water_mask++);

            int24_t dy = (8 + row - 1 + y0);

            if(dy >= 0 && dy < LCD_HEIGHT && dx >= 0 && dx < LCD_WIDTH)
                VRAM[LCD_WIDTH * dy + dx] = color;
        }
    }
}

// Draws a left facing triangle
void draw_left_triangle(int24_t x0, int24_t y0, uint8_t *tex, uint8_t *shadow_mask, uint8_t *water_mask) {
    // If we're going to clip, defer to the slow version
    if(x0 < 16 || x0 > LCD_WIDTH || y0 < 0 || y0 >= LCD_HEIGHT - 16) {
        draw_left_triangle_clipped(x0, y0, tex, shadow_mask, water_mask);
        return;
    }

    // Our base VRAM pointer for each line
    uint8_t* base = &VRAM[LCD_WIDTH * y0 + x0];
    // The width of each line as we draw
    uint8_t width = 0;

    // Top half
    for(int row = 1; row <= 8; row++) {
        width += 2;
        //memcpy(base - width, tex, width);
        copy_tex_line(base - width, tex, shadow_mask, water_mask, width);
        tex += width;
        shadow_mask += width;
        water_mask += width;
        base += LCD_WIDTH;
    }

    // Bottom half
    for(int row = 1; row < 8; row++) {
        width -= 2;
        //memcpy(base - width, tex, width);
        copy_tex_line(base - width, tex, shadow_mask, water_mask, width);
        tex += width;
        shadow_mask += width;
        water_mask += width;
        base += LCD_WIDTH;
    }
}

// Draws a right facing triangle
void draw_right_triangle(int24_t x0, int24_t y0, uint8_t *tex, uint8_t *shadow_mask, uint8_t *water_mask) {
    // If we're going to clip, defer to the slow version
    if(x0 < 0 || x0 >= LCD_WIDTH - 16 || y0 < 0 || y0 >= LCD_HEIGHT - 16) {
        draw_right_triangle_clipped(x0, y0, tex, shadow_mask, water_mask);
        return;
    }

    // Our base VRAM pointer for each line
    uint8_t* base = &VRAM[LCD_WIDTH * y0 + x0];
    // The width of each line as we draw
    uint8_t width = 0;

    // Top half
    for(int row = 1; row <= 8; row++) {
        width += 2;
        copy_tex_line(base, tex, shadow_mask, water_mask, width);
        tex += width;
        shadow_mask += width;
        water_mask += width;
        base += LCD_WIDTH;
    }

    // Bottom half
    for(int row = 1; row < 8; row++) {
        width -= 2;
        copy_tex_line(base, tex, shadow_mask, water_mask, width);
        tex += width;
        shadow_mask += width;
        water_mask += width;
        base += LCD_WIDTH;
    }
}


// Draws a left facing triangle and checks every pixel to ensure nothing gets drawn out of bounds
void draw_left_triangle_clipped(int24_t x0, int24_t y0, uint8_t *water_mask) {
    for(int row = 1; row <= 8; row++) {
        for(int dx = x0 - 2 * row; dx < x0; dx++) {
            uint8_t color = SKY | *(water_mask++);

            int24_t dy = (row - 1 + y0);

            if(dy >= 0 && dy < LCD_HEIGHT && dx >= 0 && dx < LCD_WIDTH)
                VRAM[LCD_WIDTH * dy + dx] = color;
        }
    }

    for(int row = 1; row < 8; row++) {
        for(int dx = x0 - 16 + 2 * row; dx < x0; dx++) {
            uint8_t color = SKY | *(water_mask++);

            int24_t dy = (8 + row - 1 + y0);

            if(dy >= 0 && dy < LCD_HEIGHT && dx >= 0 && dx < LCD_WIDTH)
                VRAM[LCD_WIDTH * dy + dx] = color;
        }
    }
}

// Draws a right facing triangle and checks every pixel to ensure nothing gets drawn out of bounds
void draw_right_triangle_clipped(int24_t x0, int24_t y0, uint8_t *water_mask) {
    for(int row = 1; row <= 8; row++) {
        for(int dx = x0; dx < x0 + 2 * row; dx++) {
            uint8_t color = SKY | *(water_mask++);

            int24_t dy = (row - 1 + y0);

            if(dy >= 0 && dy < LCD_HEIGHT && dx >= 0 && dx < LCD_WIDTH)
                VRAM[LCD_WIDTH * dy + dx] = color;
        }
    }

    for(int row = 1; row < 8; row++) {
        for(int dx = x0; dx < x0 + 16 - 2 * row; dx++) {
            uint8_t color = SKY | *(water_mask++);

            int24_t dy = (8 + row - 1 + y0);

            if(dy >= 0 && dy < LCD_HEIGHT && dx >= 0 && dx < LCD_WIDTH)
                VRAM[LCD_WIDTH * dy + dx] = color;
        }
    }
}

// Draws a left facing triangle
void draw_left_triangle(int24_t x0, int24_t y0, uint8_t *water_mask) {
    // If we're going to clip, defer to the slow version
    if(x0 < 16 || x0 > LCD_WIDTH || y0 < 0 || y0 >= LCD_HEIGHT - 16) {
        draw_left_triangle_clipped(x0, y0, water_mask);
        return;
    }

    // Our base VRAM pointer for each line
    uint8_t* base = &VRAM[LCD_WIDTH * y0 + x0];
    // The width of each line as we draw
    uint8_t width = 0;

    // Top half
    for(int row = 1; row <= 8; row++) {
        width += 2;
        copy_tex_line(base - width, water_mask, width);
        water_mask += width;
        base += LCD_WIDTH;
    }

    // Bottom half
    for(int row = 1; row < 8; row++) {
        width -= 2;
        copy_tex_line(base - width, water_mask, width);
        water_mask += width;
        base += LCD_WIDTH;
    }
}

// Draws a right facing triangle
void draw_right_triangle(int24_t x0, int24_t y0, uint8_t *water_mask) {
    // If we're going to clip, defer to the slow version
    if(x0 < 0 || x0 >= LCD_WIDTH - 16 || y0 < 0 || y0 >= LCD_HEIGHT - 16) {
        draw_right_triangle_clipped(x0, y0, water_mask);
        return;
    }

    // Our base VRAM pointer for each line
    uint8_t* base = &VRAM[LCD_WIDTH * y0 + x0];
    // The width of each line as we draw
    uint8_t width = 0;

    // Top half
    for(int row = 1; row <= 8; row++) {
        width += 2;
        copy_tex_line(base, water_mask, width);
        water_mask += width;
        base += LCD_WIDTH;
    }

    // Bottom half
    for(int row = 1; row < 8; row++) {
        width -= 2;
        copy_tex_line(base, water_mask, width);
        water_mask += width;
        base += LCD_WIDTH;
    }
}


// Draws a left facing triangle and checks every pixel to ensure nothing gets drawn out of bounds
void draw_left_triangle_clipped(int24_t x0, int24_t y0, uint8_t *tex, uint8_t flags) {
    for(int row = 1; row <= 8; row++) {
        for(int dx = x0 - 2 * row; dx < x0; dx++) {
            uint8_t color = *(tex++);

            if(color == 0) continue;

            color |= flags;

            int24_t dy = (row - 1 + y0);

            if(dy >= 0 && dy < LCD_HEIGHT && dx >= 0 && dx < LCD_WIDTH)
                VRAM[LCD_WIDTH * dy + dx] = color;
        }
    }

    for(int row = 1; row < 8; row++) {
        for(int dx = x0 - 16 + 2 * row; dx < x0; dx++) {
            uint8_t color = *(tex++);

            if(color == 0) continue;

            color |= flags;

            int24_t dy = (8 + row - 1 + y0);

            if(dy >= 0 && dy < LCD_HEIGHT && dx >= 0 && dx < LCD_WIDTH)
                VRAM[LCD_WIDTH * dy + dx] = color;
        }
    }
}

// Draws a right facing triangle and checks every pixel to ensure nothing gets drawn out of bounds
void draw_right_triangle_clipped(int24_t x0, int24_t y0, uint8_t *tex, uint8_t flags) {
    for(int row = 1; row <= 8; row++) {
        for(int dx = x0; dx < x0 + 2 * row; dx++) {
            uint8_t color = *(tex++);

            if(color == 0) continue;

            color |= flags;

            int24_t dy = (row - 1 + y0);

            if(dy >= 0 && dy < LCD_HEIGHT && dx >= 0 && dx < LCD_WIDTH)
                VRAM[LCD_WIDTH * dy + dx] = color;
        }
    }

    for(int row = 1; row < 8; row++) {
        for(int dx = x0; dx < x0 + 16 - 2 * row; dx++) {
            uint8_t color = *(tex++);

            if(color == 0) continue;

            color |= flags;

            int24_t dy = (8 + row - 1 + y0);

            if(dy >= 0 && dy < LCD_HEIGHT && dx >= 0 && dx < LCD_WIDTH)
                VRAM[LCD_WIDTH * dy + dx] = color;
        }
    }
}

// Draws a left facing triangle
void draw_left_triangle(int24_t x0, int24_t y0, uint8_t *tex, uint8_t flags) {
    // If we're going to clip, defer to the slow version
    if(x0 < 16 || x0 > LCD_WIDTH || y0 < 0 || y0 >= LCD_HEIGHT - 16) {
        draw_left_triangle_clipped(x0, y0, tex, flags);
        return;
    }

    // Our base VRAM pointer for each line
    uint8_t* base = &VRAM[LCD_WIDTH * y0 + x0];
    // The width of each line as we draw
    uint8_t width = 0;

    // Top half
    for(int row = 1; row <= 8; row++) {
        width += 2;
        //memcpy(base - width, tex, width);
        copy_tex_line(base - width, tex, flags, width);
        tex += width;
        base += LCD_WIDTH;
    }

    // Bottom half
    for(int row = 1; row < 8; row++) {
        width -= 2;
        //memcpy(base - width, tex, width);
        copy_tex_line(base - width, tex, flags, width);
        tex += width;
        base += LCD_WIDTH;
    }
}

// Draws a right facing triangle
void draw_right_triangle(int24_t x0, int24_t y0, uint8_t *tex, uint8_t flags) {
    // If we're going to clip, defer to the slow version
    if(x0 < 0 || x0 >= LCD_WIDTH - 16 || y0 < 0 || y0 >= LCD_HEIGHT - 16) {
        draw_right_triangle_clipped(x0, y0, tex, flags);
        return;
    }

    // Our base VRAM pointer for each line
    uint8_t* base = &VRAM[LCD_WIDTH * y0 + x0];
    // The width of each line as we draw
    uint8_t width = 0;

    // Top half
    for(int row = 1; row <= 8; row++) {
        width += 2;
        copy_tex_line(base, tex, flags, width);
        tex += width;
        base += LCD_WIDTH;
    }

    // Bottom half
    for(int row = 1; row < 8; row++) {
        width -= 2;
        copy_tex_line(base, tex, flags, width);
        tex += width;
        base += LCD_WIDTH;
    }
}




void draw_block(int24_t x, int24_t y, uint8_t *tex) {

    draw_left_triangle( x,      y,      &tex[TEX_SIZE * (TOP_FACE * 2    )],     shadow_masks[SHADOW_NONE][TOP_FACE * 2],     water_masks[WATER_NONE][TOP_FACE * 2]);
    draw_right_triangle(x,      y,      &tex[TEX_SIZE * (TOP_FACE * 2 + 1)], shadow_masks[SHADOW_NONE][TOP_FACE * 2 + 1], water_masks[WATER_NONE][TOP_FACE * 2 + 1]);

    draw_left_triangle( x,      y + 16, &tex[TEX_SIZE * (LEFT_FACE * 2    )],     shadow_masks[SHADOW_NONE][LEFT_FACE * 2],     water_masks[WATER_NONE][LEFT_FACE * 2]);
    draw_right_triangle(x - 16, y +  8, &tex[TEX_SIZE * (LEFT_FACE * 2 + 1)], shadow_masks[SHADOW_NONE][LEFT_FACE * 2 + 1], water_masks[WATER_NONE][LEFT_FACE * 2 + 1]);

    draw_left_triangle( x + 16, y +  8, &tex[TEX_SIZE * (RIGHT_FACE * 2    )],     shadow_masks[SHADOW_NONE][RIGHT_FACE * 2],     water_masks[WATER_NONE][RIGHT_FACE * 2]);
    draw_right_triangle(x,      y + 16, &tex[TEX_SIZE * (RIGHT_FACE * 2 + 1)], shadow_masks[SHADOW_NONE][RIGHT_FACE * 2 + 1], water_masks[WATER_NONE][RIGHT_FACE * 2 + 1]);
}

void draw_block(uint8_t x, uint8_t y, uint8_t z, uint8_t *tex) {
    int24_t screen_x = scroll_x + 160 + (16 * x) - (16 * z);
    int24_t screen_y = scroll_y + 209 -  (8 * x) -  (8 * z) - (16 * y);

    draw_block(screen_x, screen_y, tex);
}

void draw_tri_grid(world_t &world) {
    uint24_t origin_x = LCD_WIDTH / 2;
    uint24_t draw_y = LCD_HEIGHT - 15;

    int start_row = (scroll_y + LCD_HEIGHT - draw_y1 - 1) / 8;
    int end_row = (scroll_y + LCD_HEIGHT - draw_y0 + 7) / 8;

    // Clamp the range to [0, ROW_CNT)
    start_row = (start_row < 0 ? 0 : start_row);
    end_row = (end_row > ROW_CNT ? ROW_CNT : end_row);

    draw_y -= 8 * start_row;

    for(int row = start_row; row < end_row; row++) {
        int width = world.tri_grid_row_width[row];
        int24_t draw_x = origin_x - world.tri_grid_row_px_offset[row];
        int offset = world.tri_grid_row_offset[row];
        
        // Round down the start triangle, and up the end triangle
        int start_tri = (draw_x0 - draw_x - scroll_x + 16) / 16;
        int end_tri = (draw_x1 - draw_x - scroll_x + 31) / 16;

        // Clamp the range to [0, width)
        start_tri = (start_tri < 0 ? 0 : start_tri);
        end_tri = (end_tri > width ? width : end_tri);
        
        draw_x += start_tri * 16;

        if(((offset + start_tri) & 1) == 1) {
            draw_x -= 16;
        }
        
        for(int i = start_tri; i < end_tri; i++) {
            uint8_t texture = world.tri_grid_tex[world.tri_grid_rows[row] + i];
            //uint8_t texture = world.tri_grid_depth[world.tri_grid_rows[row] + i] % 8 + STONE;
            uint8_t flags = world.tri_grid_flags[world.tri_grid_rows[row] + i];

            uint8_t face = flags & FACE_MASK;
            uint8_t shadow = (flags & SHADOW_MASK) >> SHADOW_OFFSET;
            uint8_t water  = (flags & WATER_MASK)  >> WATER_OFFSET;

            if(((i - offset) & 1) == 0) {
                // Draw filled triangles with texture and shadows
                if(texture != 0) {
                    texture -= 2;
                    draw_left_triangle(draw_x + scroll_x, draw_y + scroll_y, 
                                       textures[texture][face * 2 + 0], 
                                       shadow_masks[shadow][face * 2 + 0],
                                       water_masks[water][face * 2 + 0]);
                }
                // Draw empty triangles with sky color and water
                else 
                {
                    draw_left_triangle(draw_x + scroll_x, draw_y + scroll_y, 
                                       water_masks[water][face * 2 + 0]);
                }

            }
            else {
                // Draw filled triangles with texture and shadows
                if(texture != 0) {
                    texture -= 2;
                    draw_right_triangle(draw_x + scroll_x, draw_y + scroll_y, 
                                        textures[texture][face * 2 + 1], 
                                        shadow_masks[shadow][face * 2 + 1],
                                        water_masks[water][face * 2 + 1]);
                }
                // Draw empty triangles with sky color and water
                else 
                {
                    draw_right_triangle(draw_x + scroll_x, draw_y + scroll_y, 
                                        water_masks[water][face * 2 + 1]);
                }

                draw_x += 32;
            }
        }

        draw_y -= 8;
    }
}

void scroll_view(world_t &world, int24_t x, int24_t y) {
    // Swap our draw buffer
    uint8_t* old_VRAM = VRAM;
    VRAM = (uint8_t*)((uint24_t)VRAM ^ BUFFER_SWP);

    scroll_x += x;
    scroll_y += y;

    int24_t abs_x = (x > 0) ? x : -x;
    int24_t abs_y = (y > 0) ? y : -y;

    uint8_t *src_row = old_VRAM;
    uint8_t *dst_row = VRAM;

    // Vertical scrolling
    if(y > 0) {
        dst_row += LCD_WIDTH * abs_y;
    }
    else {
        src_row += LCD_WIDTH * abs_y;
    }
    
    // Horizontal scrolling
    if(x > 0) {
        dst_row += abs_x;
    }
    else {
        src_row += abs_x;
    }

    // Copy over the old frame onto the new one
    for(int row = 0; row < LCD_HEIGHT - abs_y; row++) {
        memcpy((void*)dst_row, (void*)src_row, LCD_WIDTH - abs_x);

        src_row += LCD_WIDTH;
        dst_row += LCD_WIDTH;
    }


    // Vertical scrolling
    draw_x0 = 0;
    draw_x1 = LCD_WIDTH;
    
    if(y > 0) {
        draw_y0 = 0;
        draw_y1 = y + 16;
    }
    else if(y < 0) {        
        draw_y0 = LCD_HEIGHT + y - 16;
        draw_y1 = LCD_HEIGHT;
    }

    if(y != 0) {
        // Clear out the rectangle
        for(int y = draw_y0; y < draw_y1; y++) {
            memset(&VRAM[y * LCD_WIDTH + draw_x0], SKY, draw_x1 - draw_x0);
        }

        // Redraw in a patch
        draw_tri_grid(world);
    }

    
    // Horizontal scrolling
    draw_y0 = 0;
    draw_y1 = LCD_HEIGHT;
    
    if(x > 0) {
        draw_x0 = 0;
        draw_x1 = x + 16;
    }
    else if(x < 0) {
        draw_x0 = LCD_WIDTH + x - 16;
        draw_x1 = LCD_WIDTH;
    }

    if(x != 0) {
        // Clear out the rectangle
        for(int y = draw_y0; y < draw_y1; y++) {
            memset(&VRAM[y * LCD_WIDTH + draw_x0], SKY, draw_x1 - draw_x0);
        }

        // Redraw in a patch
        draw_tri_grid(world);
    }

    gfx_SwapDraw();
}

void dim_screen() {
    // Swap our draw buffer
    uint8_t* old_VRAM = VRAM;
    VRAM = (uint8_t*)((uint24_t)VRAM ^ BUFFER_SWP);

    memcpy(VRAM, old_VRAM, LCD_CNT);

    for(int i = 0; i < LCD_CNT; i++) {
        VRAM[i] |= SHADOW;
    }
}


void draw_num(int24_t x, int24_t y, uint8_t n) {
    char buf[4];

    for(int i = 0; i < 3; i++) {
        buf[2 - i] = (n % 10) + '0';
        n /= 10;
    }

    buf[3] = 0;

    gfx_SetDrawScreen();
    gfx_SetColor(SKY);
    gfx_FillRectangle(x, y, 24, 8);
    gfx_SetColor(0);
    gfx_PrintStringXY(buf, x, y);
    gfx_SetDrawBuffer();
}


void empty_draw_region(void) {
    draw_x0 = LCD_WIDTH;
    draw_x1 = 0;
    draw_y0 = LCD_HEIGHT;
    draw_y1 = 0;
}

int24_t max(int24_t a, int24_t b) {
    return (a > b) ? a : b;
}

int24_t min(int24_t a, int24_t b) {
    return (a < b) ? a : b;
}

uint16_t max(uint16_t a, uint16_t b) {
    return (a > b) ? a : b;
}

uint16_t min(uint16_t a, uint16_t b) {
    return (a < b) ? a : b;
}

void expand_draw_region(uint8_t x, uint8_t y, uint8_t z) {
    int24_t screen_x = scroll_x + 160 + (16 * x) - (16 * z);
    int24_t screen_y = scroll_y + 209 -  (8 * x) -  (8 * z) - (16 * y);

    uint16_t block_draw_x0 = (uint16_t)max(min(screen_x - 16, LCD_WIDTH), 0);
    uint16_t block_draw_x1 = (uint16_t)max(min(screen_x + 16, LCD_WIDTH), 0);
    uint16_t block_draw_y0 = (uint16_t)max(min(screen_y -  0, LCD_HEIGHT), 0);
    uint16_t block_draw_y1 = (uint16_t)max(min(screen_y + 24, LCD_HEIGHT), 0);

    draw_x0 = min(draw_x0, block_draw_x0);
    draw_x1 = max(draw_x1, block_draw_x1);
    draw_y0 = min(draw_y0, block_draw_y0);
    draw_y1 = max(draw_y1, block_draw_y1);
}