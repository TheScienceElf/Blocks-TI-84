#pragma once
#include <stdint.h>
#include "draw.h"

typedef struct player {
    int24_t x, y, z;
    Block_t current_block;

    world_t *world;

    void move(int8_t dx, int8_t dy, int8_t dz) {
        undraw();

        x += dx;
        y += dy;
        z += dz;

        // Clamp player position to within the world grid
        x = (x < 0) ? 0 : (x >=   WORLD_SIZE) ?   WORLD_SIZE - 1 : x;
        y = (y < 0) ? 0 : (y >= WORLD_HEIGHT) ? WORLD_HEIGHT - 1 : y;
        z = (z < 0) ? 0 : (z >=   WORLD_SIZE) ?   WORLD_SIZE - 1 : z;

        draw();
    }

    void draw() {
        int24_t screen_x = scroll_x + 160 + (16 * x) - (16 * z);
        int24_t screen_y = scroll_y + 209 -  (8 * x) -  (8 * z) - (16 * y);

        uint8_t depth = project_view_depth(x, y, z);
        
        int tri_grid_idx = world->project(x, y, z, TOP_FACE);


        // Draw Back Texture
        if(world->tri_grid_depth[tri_grid_idx] > depth)
            draw_left_triangle(screen_x,  screen_y, 
                               player_tex[RIGHT_FACE * 2],     
                               SHADOW);
        
        tri_grid_idx++;

        if(world->tri_grid_depth[tri_grid_idx] > depth)
            draw_right_triangle(screen_x, screen_y, 
                                player_tex[LEFT_FACE * 2 + 1], 
                                SHADOW);

        tri_grid_idx = world->project(x, y, z, MID_FACE);

        if(world->tri_grid_depth[tri_grid_idx] > depth)
            draw_right_triangle(screen_x - 16, screen_y + 8, 
                                player_tex[RIGHT_FACE * 2 + 1], 
                                SHADOW);
        
        tri_grid_idx++;
        
        if(world->tri_grid_depth[tri_grid_idx] > depth)
            draw_left_triangle(screen_x + 16,  screen_y + 8, 
                               player_tex[LEFT_FACE * 2],     
                               SHADOW);

        tri_grid_idx = world->project(x, y, z, BOT_FACE);

        if(world->tri_grid_depth[tri_grid_idx] > depth)
            draw_left_triangle(screen_x,  screen_y + 16, 
                               player_tex[TOP_FACE * 2],     
                               SHADOW);
        
        tri_grid_idx++;
        
        if(world->tri_grid_depth[tri_grid_idx] > depth)
            draw_right_triangle(screen_x, screen_y + 16, 
                                player_tex[TOP_FACE * 2 + 1], 
                                SHADOW);

        




        // Draw Front Texture
        tri_grid_idx = world->project(x, y, z, TOP_FACE);
        
        draw_left_triangle(screen_x,  screen_y, 
                           player_tex[TOP_FACE * 2],
                           world->tri_grid_depth[tri_grid_idx] >= depth ? 0 : SHADOW);
        
        tri_grid_idx++;

        draw_right_triangle(screen_x, screen_y, 
                            player_tex[TOP_FACE * 2 + 1], 
                            world->tri_grid_depth[tri_grid_idx] >= depth ? 0 : SHADOW);

        tri_grid_idx = world->project(x, y, z, MID_FACE);

        draw_right_triangle(screen_x - 16, screen_y + 8, 
                            player_tex[LEFT_FACE * 2 + 1], 
                            world->tri_grid_depth[tri_grid_idx] >= depth ? 0 : SHADOW);
        
        tri_grid_idx++;
        
        draw_left_triangle(screen_x + 16,  screen_y + 8, 
                           player_tex[RIGHT_FACE * 2],   
                            world->tri_grid_depth[tri_grid_idx] >= depth ? 0 : SHADOW);

        tri_grid_idx = world->project(x, y, z, BOT_FACE);

        draw_left_triangle(screen_x,  screen_y + 16, 
                           player_tex[LEFT_FACE * 2],   
                           world->tri_grid_depth[tri_grid_idx] >= depth ? 0 : SHADOW);
        
        tri_grid_idx++;
        
        draw_right_triangle(screen_x, screen_y + 16, 
                            player_tex[RIGHT_FACE * 2 + 1], 
                            world->tri_grid_depth[tri_grid_idx] >= depth ? 0 : SHADOW);
    }

    void undraw() {
        empty_draw_region();
        expand_draw_region(x, y, z);
        draw_tri_grid(*world);
    }

    void scroll_to_center(int24_t &goal_x, int24_t &goal_y) {
        int24_t screen_x = scroll_x + 160 + (16 * x) - (16 * z);
        int24_t screen_y = scroll_y + 209 -  (8 * x) -  (8 * z) - (16 * y);

        goal_x = (LCD_WIDTH  / 2) + scroll_x - screen_x;
        goal_y = (LCD_HEIGHT / 2) + scroll_y - screen_y - 16;
    }

    void scroll_to_contain(int24_t &goal_x, int24_t &goal_y) {
        int24_t screen_x = scroll_x + 160 + (16 * x) - (16 * z);
        int24_t screen_y = scroll_y + 209 -  (8 * x) -  (8 * z) - (16 * y);

        int24_t target_x = screen_x;
        int24_t target_y = screen_y;

        if(target_x < BLOCK_HALF_WIDTH + SCROLL_SPEED) target_x = BLOCK_HALF_WIDTH + SCROLL_SPEED;
        if(target_x > LCD_WIDTH - BLOCK_HALF_WIDTH - SCROLL_SPEED) target_x = LCD_WIDTH - BLOCK_HALF_WIDTH - SCROLL_SPEED;
        if(target_y < SCROLL_SPEED) target_y = SCROLL_SPEED;
        if(target_y > LCD_HEIGHT - BLOCK_HEIGHT - SCROLL_SPEED) target_y = LCD_HEIGHT - BLOCK_HEIGHT - SCROLL_SPEED;

        goal_x = target_x + scroll_x - screen_x;
        goal_y = target_y + scroll_y - screen_y;
    }

} player_t;