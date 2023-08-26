#pragma once
#include <fileioc.h>
#include "world.h"
#include "player.h"
#include "ui.h"


// Saves a world and player position details to a set of files.
// (world_id should be 1-5 though that limit is only imposed by the UI)
void save(uint8_t world_id, world_t &world, player_t &player) {
    char filename[7] = "WORLDA";
    filename[5] = 'A' + world_id;

    // Save player position
    {
        ti_var_t var = ti_Open(filename, "w+");
        if (var == 0) return;

        ti_PutC((uint8_t)player.x, var);
        ti_PutC((uint8_t)player.y, var);
        ti_PutC((uint8_t)player.z, var);
        ti_PutC((uint8_t)player.current_block, var);

        ti_PutC((uint8_t)((scroll_x >> 16) & 0xFF), var);
        ti_PutC((uint8_t)((scroll_x >>  8) & 0xFF), var);
        ti_PutC((uint8_t)((scroll_x >>  0) & 0xFF), var);

        ti_PutC((uint8_t)((scroll_y >> 16) & 0xFF), var);
        ti_PutC((uint8_t)((scroll_y >>  8) & 0xFF), var);
        ti_PutC((uint8_t)((scroll_y >>  0) & 0xFF), var);

        ti_SetArchiveStatus(true, var);

        ti_Close(var);
    }



    char out_name[9] = "WORLDA00";
    out_name[5] = 'A' + world_id;

    // To save RAM, worlds are stored and loaded one horizontal slice
    // at a time
    for(uint8_t i = 0; i < WORLD_HEIGHT; i++) {
        fill_progress_bar(i, WORLD_HEIGHT);

        // Set the digit counter to the world level
        out_name[6] = '0' + (i / 10);
        out_name[7] = '0' + (i % 10);

        /* Open a new variable; deleting it if it already exists */
        ti_var_t var = ti_Open(out_name, "w+");
        if (var == 0) return;

        ti_Write((char*)&world.blocks[i], WORLD_SIZE * WORLD_SIZE, 1, var);

        ti_SetArchiveStatus(true, var);

        ti_Close(var);
    }
}

// Attempts to load a world in from a given ID. Returns true or
// false depending on if this was successful
bool load(uint8_t world_id, world_t &world, player_t &player) {
    char filename[7] = "WORLDA";
    filename[5] = 'A' + world_id;

    // Load player position
    {
        ti_var_t var = ti_Open(filename, "r");
        if (var == 0) return false;

        player.x = (int24_t)ti_GetC(var);
        player.y = (int24_t)ti_GetC(var);
        player.z = (int24_t)ti_GetC(var);

        player.current_block = ti_GetC(var);

        scroll_x = 0;
        scroll_x += ti_GetC(var);
        scroll_x <<= 8; 
        scroll_x += ti_GetC(var);
        scroll_x <<= 8; 
        scroll_x += ti_GetC(var);

        scroll_y = 0;
        scroll_y += ti_GetC(var);
        scroll_y <<= 8; 
        scroll_y += ti_GetC(var);
        scroll_y <<= 8; 
        scroll_y += ti_GetC(var);

        ti_SetArchiveStatus(true, var);

        ti_Close(var);
    }

    char out_name[9] = "WORLDA00";
    out_name[5] = 'A' + world_id;

    // To save RAM, worlds are stored and loaded one horizontal slice
    // at a time
    for(uint8_t i = 0; i < WORLD_HEIGHT; i++) {
        // Set the digit counter to the world level
        out_name[6] = '0' + (i / 10);
        out_name[7] = '0' + (i % 10);

        /* Open a new variable; deleting it if it already exists */
        ti_var_t var = ti_Open(out_name, "r");
        if (var == 0) return false;

        ti_Read((char*)&world.blocks[i], WORLD_SIZE * WORLD_SIZE, 1, var);

        ti_Close(var);
    }

    return true;
}

// Deletes all related world files for a given ID
void erase(uint8_t world_id) {
    char filename[7] = "WORLDA";
    filename[5] = 'A' + world_id;

    if(ti_Delete(filename) == 0) return;

    char out_name[9] = "WORLDA00";
    out_name[5] = 'A' + world_id;

    for(uint8_t i = 0; i < WORLD_HEIGHT; i++) {
        // Set the digit counter to the world level
        out_name[6] = '0' + (i / 10);
        out_name[7] = '0' + (i % 10);
        ti_Delete(out_name);
    }
}