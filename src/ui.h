#pragma once
#include <tice.h>
#include <graphx.h>
#include "draw.h"

// Parameters for the block selection UI
#define UI_BORDER 16
#define ICON_SPACING 40
#define ICON_COLS 6
#define ICON_WIDTH 16

#define ICON_BORDER ((LCD_WIDTH - (ICON_SPACING * (ICON_COLS - 1))) / 2)

// Parameters for the world selection UI
#define SAVE_CNT 5

// Some colors pulled from the palette to be used in the UI
#define UI_BORDER_COLOR 1
#define UI_SELECT_COLOR 2
#define UI_BACKGROUND_COLOR 3

// Add the colors needed to draw the UI elements to the palette
void init_ui_palette() { 
    volatile uint16_t* p = (uint16_t*)0xE30200;
    p[0] = gfx_RGBTo1555(  0,  0,  0);
    p[1] = gfx_RGBTo1555(255,255,255);
    p[2] = gfx_RGBTo1555(255,102, 64);

    p[3] = gfx_RGBTo1555(128,128,128);
    p[4] = gfx_RGBTo1555(192,192,192);
    p[5] = gfx_RGBTo1555(160,160,160);
}

// Draws the outline of a progress bar and the label
void progress_bar(const char *text) {
    gfx_SetColor(0);
    gfx_Rectangle(16, (LCD_HEIGHT - 16) / 2, LCD_WIDTH - 32, 16);

    gfx_SetColor(1);
    gfx_FillRectangle(16, (LCD_HEIGHT - 32) / 2, LCD_WIDTH - 32, 8);

    gfx_SetTextFGColor(0);
    gfx_PrintStringXY(text, 16, (LCD_HEIGHT - 32) / 2);
}

// Fills in the progress bar with the fraction of progress / total
void fill_progress_bar(uint16_t progress, uint16_t total) {
    gfx_SetColor(2);
    gfx_FillRectangle(18, (LCD_HEIGHT - 12) / 2, progress * (LCD_WIDTH - 36) / (total - 1), 12);
}

// Draws the block selection GUI during play-mode.
void draw_block_select() {
    gfx_SetColor(UI_BACKGROUND_COLOR);
    gfx_FillRectangle(UI_BORDER, UI_BORDER, LCD_WIDTH - 2 * UI_BORDER, LCD_HEIGHT - 2 * UI_BORDER);

    gfx_SetColor(UI_BORDER_COLOR);
    gfx_Rectangle(UI_BORDER, UI_BORDER, LCD_WIDTH - 2 * UI_BORDER, LCD_HEIGHT - 2 * UI_BORDER);

    int24_t block_x = ICON_BORDER;
    int24_t block_y = ICON_BORDER - 20;

    // Draw all the textured blocks
    for(int i = 0; i < TEX_CNT; i++) {
        draw_block(block_x, block_y, (uint8_t*)textures[i]);

        block_x += ICON_SPACING;

        if(block_x >= LCD_WIDTH - 48) {
            block_x = ICON_BORDER;
            block_y += ICON_SPACING;
        }
    }

    // Draw the water block last
    draw_left_triangle( block_x,      block_y,      water_masks[2][0]);
    draw_right_triangle(block_x,      block_y,      water_masks[2][0]);

    draw_left_triangle( block_x,      block_y + 16, water_masks[2][0]);
    draw_right_triangle(block_x - 16, block_y +  8, water_masks[2][0]);

    draw_left_triangle( block_x + 16, block_y +  8, water_masks[2][0]);
    draw_right_triangle(block_x,      block_y + 16, water_masks[2][0]);

    gfx_SwapDraw();
}

// The full block selection GUI. Dims the screen, draws the graphics, handles input
// and then returns the selected block. The previously selected block is passed as
// an argument.
Block_t block_select(Block_t block) {

    dim_screen();
    draw_block_select();

    gfx_SetDrawScreen();

    // Since water should show last on the list, we have to remap its block ID here
    if(block == WATER)
        block = TEX_CNT + STONE;

    uint8_t row = (block - STONE) / ICON_COLS;
    uint8_t col = (block - STONE) % ICON_COLS;

    uint8_t row_old = row ^ 1;
    uint8_t col_old = 0;

    sk_key_t key;
    
    do
    {
        if(row != row_old || col != col_old) {
            gfx_SetColor(UI_BACKGROUND_COLOR);
            gfx_Rectangle(ICON_BORDER - ICON_WIDTH - 2 + (col_old * ICON_SPACING), 
                          ICON_BORDER - ICON_WIDTH - 6 + (row_old * ICON_SPACING), 
                          36, 36);
            gfx_Rectangle(ICON_BORDER - ICON_WIDTH - 3 + (col_old * ICON_SPACING), 
                          ICON_BORDER - ICON_WIDTH - 7 + (row_old * ICON_SPACING), 
                          38, 38);

            gfx_SetColor(UI_SELECT_COLOR);
            gfx_Rectangle(ICON_BORDER - ICON_WIDTH - 2 + (col * ICON_SPACING), 
                          ICON_BORDER - ICON_WIDTH - 6 + (row * ICON_SPACING), 
                          36, 36);
            gfx_Rectangle(ICON_BORDER - ICON_WIDTH - 3 + (col * ICON_SPACING), 
                          ICON_BORDER - ICON_WIDTH - 7 + (row * ICON_SPACING), 
                          38, 38);
        }

        row_old = row;
        col_old = col;

        key = os_GetCSC();

        switch (key)
        {
            case sk_Left:
                if(col > 0) 
                    col--;
                break;
            case sk_Right:
                if(col < ICON_COLS - 1)
                    col++;
                break;
            case sk_Down:
                row++;
                break;
            case sk_Up:
                if(row > 0)
                    row--;
                break;
            default:
                break;
        }

        if(row * ICON_COLS + col > TEX_CNT) {
            row = row_old;
            col = col_old;
        }

    } while (key != sk_Enter);
    
    gfx_SetDrawBuffer();
    VRAM = (uint8_t*)((uint24_t)VRAM ^ BUFFER_SWP);
    gfx_SwapDraw();

    block = (row * ICON_COLS) + col + STONE;

    // Remap the block back to the actual ID
    if(block == TEX_CNT + STONE) 
        block = WATER;

    return block;
}

// A generic menu implementation used in some of the save management GUI
uint8_t menu(const char **options, uint8_t option_cnt) {
    gfx_SetColor(4);
    gfx_FillRectangle(UI_BORDER, UI_BORDER, LCD_WIDTH - 2 * UI_BORDER, LCD_HEIGHT - 2 * UI_BORDER);

    gfx_SetColor(0);
    gfx_Rectangle(UI_BORDER, UI_BORDER, LCD_WIDTH - 2 * UI_BORDER, LCD_HEIGHT - 2 * UI_BORDER);

        
    gfx_SetTextFGColor(0);
    gfx_PrintStringXY(options[0], UI_BORDER + 16, UI_BORDER + 16);

    
    for(uint8_t i = 0; i < option_cnt; i++) {
        gfx_SetTextFGColor(0);
        gfx_PrintStringXY(options[i + 1], 
                          UI_BORDER + 16, 
                          LCD_HEIGHT - UI_BORDER - 16  - 16 * (option_cnt - 1 - i));
    }

    uint8_t selection = 0;
    uint8_t selection_old = selection ^ 1;

    sk_key_t key;

    do
    {

        if(selection != selection_old) {
            gfx_SetColor(4);
            gfx_Rectangle(UI_BORDER + 8, 
                          LCD_HEIGHT - UI_BORDER - 18 - 16 * (option_cnt - 1 - selection_old), 
                          LCD_WIDTH - UI_BORDER - UI_BORDER - 8 - 8, 12);

            gfx_SetColor(3);
            gfx_Rectangle(UI_BORDER + 8, 
                          LCD_HEIGHT - UI_BORDER - 18 - 16 * (option_cnt - 1 - selection),
                          LCD_WIDTH - UI_BORDER - UI_BORDER - 8 - 8, 12);
        }

        selection_old = selection;

        key = os_GetCSC();

        switch (key)
        {
            case sk_Down:
                if(selection < option_cnt - 1)
                    selection++;
                break;
            case sk_Up:
                if(selection > 0)
                    selection--;
                break;

            default:
                break;
        }

    // Exit the inner loop every time something gets selected
    } while (key != sk_Enter);

    return selection;
}