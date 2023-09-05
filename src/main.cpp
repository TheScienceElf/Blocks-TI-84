#include <tice.h>
#include <graphx.h>
#include <stdlib.h>
#include "draw.h"
#include "world.h"
#include "worldgen.h"
#include "world_io.h"
#include "player.h"
#include "ui.h"
#include <debug.h>

void init_play(uint8_t world_id, world_t *world, player_t &player) {
    
    world->clear_world();
    world->init_tri_grid();

    // Try to load the world file, and otherwise generate a new one
    if(!load(world_id, *world, player)) {
        const char* options[4] = {"What type of world?", "Natural", "Flat", "Demo"};

        switch(menu(options, 3)) {
            case 0:
                generate_natural(*world, player);
                break;
            case 1:
                generate_flat(*world, player);
                break;
            case 2:
                generate_demo(*world, player);
                break;
        }

        player.scroll_to_center(scroll_x, scroll_y);
    }

    gfx_FillScreen(1);
    progress_bar("Building world...");
    
    progress_bar("Initializing shadows...");
    // Initialize the shadow triangle grid with data from the world
    for(int y = 0; y < WORLD_HEIGHT; y++) {
        fill_progress_bar(y, WORLD_HEIGHT + WORLD_HEIGHT);
        for(int z = 0; z < WORLD_SIZE; z++) {
            for(int x = 0; x < WORLD_SIZE; x++) {
                if(world->blocks[y][x][z] > WATER) {
                    world->set_block_shadow(x, y, z);
                }
            }
        }
    }

    progress_bar("Initializing blocks...");
    // Initialize the triangle grid with data from the world
    for(int y = 0; y < WORLD_HEIGHT; y++) {
        fill_progress_bar(y + WORLD_HEIGHT, WORLD_HEIGHT + WORLD_HEIGHT);
        for(int z = WORLD_SIZE - 1; z >= 0; z--) {
            for(int x = WORLD_SIZE - 1; x >= 0; x--) {
                if(world->blocks[y][x][z] == WATER) {
                    world->set_water(x, y, z);
                }
                else if(world->blocks[y][x][z] != AIR) {
                    world->set_block(x, y, z, world->blocks[y][x][z]);
                }
            }
        }
    }
    
    init_palette();
    memset(VRAM, SKY, LCD_CNT);
    
    draw_x0 = 0;
    draw_y0 = 0;
    draw_x1 = LCD_WIDTH;
    draw_y1 = LCD_HEIGHT;

    gfx_SetDrawBuffer();
    draw_tri_grid(*world);

    player.draw();
}

void play(uint8_t world_id) {
    // We store the world data at the contiguous 69K of Safe RAM
    // as listed here:
    // https://wikiti.brandonw.net/index.php?title=Category:84PCE:RAM:By_Address
    world_t* world = (world_t*)0xD05350;
    player_t player;
    player.current_block = STONE;

    init_play(world_id, world, player);

    int24_t scroll_goal_x = scroll_x;
    int24_t scroll_goal_y = scroll_y;

    player.world = world;

    sk_key_t key;
    
    do
    {
        key = os_GetCSC();

        switch (key)
        {
            // Viewport scrolling
            case sk_Left:
                scroll_goal_x += SCROLL_SPEED;
                break;
            case sk_Right:
                scroll_goal_x -= SCROLL_SPEED;
                break;
            case sk_Down:
                scroll_goal_y -= SCROLL_SPEED;
                break;
            case sk_Up:
                scroll_goal_y += SCROLL_SPEED;
                break;

            // Player movement
            case sk_7:
                player.move(0, 0, 1);
                player.scroll_to_contain(scroll_goal_x, scroll_goal_y);
                break;
            case sk_8:
                player.move(1, 0, 1);
                player.scroll_to_contain(scroll_goal_x, scroll_goal_y);
                break;
            case sk_9:
                player.move(1, 0, 0);
                player.scroll_to_contain(scroll_goal_x, scroll_goal_y);
                break;
            case sk_4:
                player.move(-1, 0, 1);
                player.scroll_to_contain(scroll_goal_x, scroll_goal_y);
                break;
            case sk_6:
                player.move(1, 0, -1);
                player.scroll_to_contain(scroll_goal_x, scroll_goal_y);
                break;
            case sk_1:
                player.move(-1, 0, 0);
                player.scroll_to_contain(scroll_goal_x, scroll_goal_y);
                break;
            case sk_2:
                player.move(-1, 0, -1);
                player.scroll_to_contain(scroll_goal_x, scroll_goal_y);
                break;
            case sk_3:
                player.move(0, 0, -1);
                player.scroll_to_contain(scroll_goal_x, scroll_goal_y);
                break;
            case sk_Mul:
                player.move(0, 1, 0);
                player.scroll_to_contain(scroll_goal_x, scroll_goal_y);
                break;
            case sk_Sub:
                player.move(0, -1, 0);
                player.scroll_to_contain(scroll_goal_x, scroll_goal_y);
                break;

            // Block placement or removal
            case sk_5:
                // Initialize our update region to be empty
                empty_draw_region();

                // Place or remove block will expand the update region to contain all updated
                // blocks (where a shadow is cast or uncast)
                if(player.current_block != WATER) {
                    if(world->blocks[player.y][player.x][player.z] == AIR) {
                        world->place_block(player.x, player.y, player.z, player.current_block);
                    }
                    // Just replace water with solid blocks when placing (double tap 5 to replace water with air)
                    else if(world->blocks[player.y][player.x][player.z] == WATER) {
                        world->remove_block(player.x, player.y, player.z);
                        world->place_block(player.x, player.y, player.z, player.current_block);
                    }
                    else {
                        world->remove_block(player.x, player.y, player.z);
                    }
                }
                else
                {
                    if(world->blocks[player.y][player.x][player.z] == AIR) {
                        world->set_water(player.x, player.y, player.z);
                        expand_draw_region(player.x, player.y, player.z);
                    }
                    else {
                        world->remove_block(player.x, player.y, player.z);
                    }
                }
                
                // Redraw the section of the screen where updates occurred and the cursor on top of that
                draw_tri_grid(*world);
                player.draw();
                break;

            // Change the currently selected block
            case sk_Enter:
                player.current_block = block_select(player.current_block);
                break;

            // Ignore all other keystrokes
            default:
                break;
        }

        // Compute the motion needed to reach our scroll target
        int24_t scroll_step_x = scroll_goal_x - scroll_x;
        int24_t scroll_step_y = scroll_goal_y - scroll_y;

        // Clamp it to the max scroll speed
        if(scroll_step_x > SCROLL_SPEED)
            scroll_step_x = SCROLL_SPEED;
        if(scroll_step_x < -SCROLL_SPEED)
            scroll_step_x = -SCROLL_SPEED;

        if(scroll_step_y > SCROLL_SPEED)
            scroll_step_y = SCROLL_SPEED;
        if(scroll_step_y < -SCROLL_SPEED)
            scroll_step_y = -SCROLL_SPEED;

        if(scroll_step_x != 0 || scroll_step_y != 0){
            scroll_view(*world, scroll_step_x, scroll_step_y);
            player.draw();
        }

    } while (key != sk_2nd);


    init_ui_palette();
    gfx_SetDrawScreen();

    gfx_FillScreen(1);
    progress_bar("Saving...");

    save(world_id, *world, player);
}

void world_select() {
    uint8_t selection = 0;

    while(true) {
        gfx_SetDrawScreen();

        gfx_SetColor(4);
        gfx_FillRectangle(UI_BORDER, UI_BORDER, LCD_WIDTH - 2 * UI_BORDER, LCD_HEIGHT - 2 * UI_BORDER);

        gfx_SetColor(0);
        gfx_Rectangle(UI_BORDER, UI_BORDER, LCD_WIDTH - 2 * UI_BORDER, LCD_HEIGHT - 2 * UI_BORDER);


        char name[8] = "World A";
        char filename[8] = "WORLDA";

        for(uint8_t i = 0; i < SAVE_CNT; i++) {
            name[6] = 'A' + i;
            filename[5] = 'A' + i;

            ti_var_t var = ti_Open(filename, "r");
            if (var == 0) {
                gfx_SetTextFGColor(3);
                gfx_PrintStringXY(name, UI_BORDER + 16, UI_BORDER + 16 + i * 32);
                gfx_SetTextFGColor(3);
                gfx_PrintStringXY("( Empty )", UI_BORDER + 24, UI_BORDER + 24 + i * 32);
            }
            else
            {
                gfx_SetTextFGColor(0);
                gfx_PrintStringXY(name, UI_BORDER + 16, UI_BORDER + 16 + i * 32);
                gfx_SetTextFGColor(3);
                gfx_PrintStringXY("48x16x48", UI_BORDER + 24, UI_BORDER + 24 + i * 32);
                ti_Close(var);
            }

        }

        gfx_SetTextFGColor(0);
        gfx_PrintStringXY("Quit", UI_BORDER + 16, LCD_HEIGHT - UI_BORDER - 16 - 8);


        uint8_t selection_old = selection ^ 1;

        sk_key_t key;

        do
        {

            if(selection != selection_old) {
                gfx_SetColor(4);
                gfx_Rectangle(UI_BORDER + 8, 
                              UI_BORDER + 12 + selection_old * 32, 
                              LCD_WIDTH - UI_BORDER - UI_BORDER - 8 - 8, 24);

                gfx_SetColor(3);
                gfx_Rectangle(UI_BORDER + 8, 
                              UI_BORDER + 12 + selection * 32, 
                              LCD_WIDTH - UI_BORDER - UI_BORDER - 8 - 8, 24);
            }

            selection_old = selection;

            key = os_GetCSC();

            switch (key)
            {
                case sk_Down:
                    if(selection < SAVE_CNT)
                        selection++;
                    break;
                case sk_Up:
                    if(selection > 0)
                        selection--;
                    break;
                case sk_Enter:
                    // Break from the program when the last option (quit) is selected
                    if(selection == SAVE_CNT) return;
                    play(selection);
                    break;

                case sk_Del:
                {
                    const char* options[3] = {"Are you sure?", "No", "Yes"};

                    if(menu(options, 2))
                        erase(selection);
                }
                    break;

                default:
                    break;
            }

        // Exit the inner loop every time something gets selected
        } while (key != sk_Enter && key != sk_Del);
    }
}

void home_menu() {
    uint8_t selection = 4;

    while(true) {
        gfx_SetDrawScreen();

        gfx_SetColor(4);
        gfx_FillRectangle(UI_BORDER, UI_BORDER, LCD_WIDTH - 2 * UI_BORDER, LCD_HEIGHT - 2 * UI_BORDER);

        init_palette();

        draw_block((int24_t) ( ( LCD_WIDTH - 2 * UI_BORDER ) / 2 ) + BLOCK_WIDTH / 2, (int24_t) ( LCD_HEIGHT - 2 * UI_BORDER ) / 3, (uint8_t*)textures[1]);

        init_ui_palette();

        gfx_SetTextFGColor(0);
        gfx_PrintStringXY("Isometric Block Game", ( ( LCD_WIDTH - 2 * UI_BORDER ) / 2 ) - 55, ( ( LCD_HEIGHT - 2 * UI_BORDER ) / 3 ) + BLOCK_HEIGHT + 8);

        gfx_SetTextFGColor(0);
        gfx_PrintStringXY("Play!", ( LCD_WIDTH - 2 * UI_BORDER ) / 2, LCD_HEIGHT - UI_BORDER - 16 - 8 - 32);

        gfx_SetTextFGColor(0);
        gfx_PrintStringXY("Quit", ( LCD_WIDTH - 2 * UI_BORDER ) / 2, LCD_HEIGHT - UI_BORDER - 16 - 8);


        uint8_t selection_old = selection ^ 1;

        sk_key_t key;

        do
        {

            if(selection != selection_old) {
                gfx_SetColor(4);
                gfx_Rectangle(UI_BORDER + 8, 
                              UI_BORDER + 12 + selection_old * 32, 
                              LCD_WIDTH - UI_BORDER - UI_BORDER - 8 - 8, 24);

                gfx_SetColor(3);
                gfx_Rectangle(UI_BORDER + 8, 
                              UI_BORDER + 12 + selection * 32, 
                              LCD_WIDTH - UI_BORDER - UI_BORDER - 8 - 8, 24);
            }

            selection_old = selection;

            key = os_GetCSC();

            switch (key)
            {
                case sk_Down:
                    if(selection < SAVE_CNT)
                        selection++;
                    break;
                case sk_Up:
                    if(selection > 4)
                        selection--;
                    break;
                case sk_Enter:
                    // Break from the program when the last option (quit) is selected
                    if(selection == SAVE_CNT) return;
                    world_select();
                    break;

                default:
                    break;
            }

        // Exit the inner loop every time something gets selected
        } while (key != sk_Enter && key != sk_Del);
    }
}

void init () {
    // Set right face textures to always to be in shadow
    for(int i = 0; i < TEX_CNT; i++) {
        for(int j = 0; j < TEX_SIZE; j++) {
            textures[i][RIGHT_FACE * 2 + 0][j] |= SHADOW;
            textures[i][RIGHT_FACE * 2 + 1][j] |= SHADOW;
        }
    }

    /* Initialize graphics drawing */
    gfx_Begin();
    gfx_SetDrawScreen();

    init_ui_palette();

    //world_select();
    home_menu();
}

int main(void)
{
    static_assert(sizeof(world_t) < 69090, "World_t size is too big for the specified SafeRAM area!");
    
    init();

    gfx_End();

    return 0;
}