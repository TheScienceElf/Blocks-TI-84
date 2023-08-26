#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "block.h"
#include "textures.h"

#define WORLD_SIZE 48
#define WORLD_HEIGHT 16

// Flags for the face configurations of each triangle
#define LEFT_FACE 0
#define RIGHT_FACE 1
#define TOP_FACE 2

#define FACE_MASK 3

#define BOT_FACE 0
#define MID_FACE 1

// -------- Triangle Grid --------
// Graphical information about the world is stored in a 2d hexagonal triangle
// grid, which allows lookup for the closest block to the camera in each 
// isometric position

#define TRI_CNT (((WORLD_SIZE * WORLD_SIZE) + (2 * WORLD_SIZE * WORLD_HEIGHT)) * 2)
#define ROW_CNT (WORLD_SIZE + WORLD_SIZE + WORLD_HEIGHT + WORLD_HEIGHT - 1)

inline uint8_t project_view_depth(uint8_t x, uint8_t y, uint8_t z) {
    return x + z + (WORLD_HEIGHT - 1 - y);
}

inline uint8_t project_light_depth(uint8_t x, uint8_t y, uint8_t z) {
    return x + (WORLD_SIZE - 1 - z) + (WORLD_HEIGHT - 1 - y);
}

inline void to_shadow_space(uint8_t x, uint8_t y, uint8_t z, uint8_t &sx, uint8_t &sy, uint8_t &sz) {
    sx = WORLD_SIZE - 1 - z;
    sy = y;
    sz = x;
}

inline void from_shadow_space(uint8_t x, uint8_t y, uint8_t z, uint8_t &sx, uint8_t &sy, uint8_t &sz) {
    sx = z;
    sy = y;
    sz = WORLD_SIZE - 1 - x;
}

typedef struct world {
    // 3D array of the world, indexed as [Y, X, Z]
    Block_t blocks[WORLD_HEIGHT][WORLD_SIZE][WORLD_SIZE];

    // The associated texture for each triangle
    uint8_t tri_grid_tex[TRI_CNT];
    // The flags determining drawing information for each triangle
    uint8_t tri_grid_flags[TRI_CNT];
    // The projected depth of each block from the view of the camera
    uint8_t tri_grid_depth[TRI_CNT];

    // Indices into the above array for the start of each row segment
    uint24_t tri_grid_rows[ROW_CNT];

    // Adjustment offset because rows change length at different rates across the hexagon
    int24_t tri_grid_row_offset[ROW_CNT];

    // Pixel offset from the center for each row's starting point
    int24_t tri_grid_row_px_offset[ROW_CNT];

    uint24_t tri_grid_row_width[ROW_CNT];


    /* Populates the LUTs for indexing into the trigrid */
    void init_tri_grid();

    /* Sweeps through blocks in the world starting from (x, y, z) and apply steps of
    * (dx, dy, dz) to offset the search. Return true if we find a solid block, false
    * if we reach the world border first
    */
    bool sweep_ray(int x, int y, int z, int dx, int dy, int dz);

    /* Sweep through the world from a certain point to see if if the top of
    * the block is in shadow. Returns the proper flags for that face
    */
    uint8_t compute_top_shadow(uint8_t x, uint8_t y, uint8_t z);

    /* Sweep through the world from a certain point to see if if the left side
    * of the block is in shadow. Returns the proper flags for that face
    */
    uint8_t compute_left_shadow(uint8_t x, uint8_t y, uint8_t z);

    // On an infinite grid, we can increment each coordinate using these rules
    // Increment x: row++, idx++
    // Increment y: row += 2, idx++
    // Increment z: row++
    //
    // s represents the pair of triangles we are projecting to
    // 0 - bottom, 1 - mid, 2 - top
    int project(uint8_t x, uint8_t y, uint8_t z, uint8_t s);

    void unproject(int row, int idx, int depth, uint8_t &x, uint8_t &y, uint8_t &z);

    void set_block(int x, int y, int z, Block_t block);

    void refresh_shadows(int x, int y, int z);

    void set_water(int x, int y, int z);

    void set_block_shadow(int x, int y, int z);

    void place_block(int x, int y, int z, Block_t block);

    bool scan_tri(int row, int idx, int depth, uint8_t &x, uint8_t &y, uint8_t &z, Block_t skip);

    bool scan_shadow(int row, int idx, int depth, uint8_t &x, uint8_t &y, uint8_t &z);

    void remove_block(int x, int y, int z);

    // Inclusively fills the space within the provided bounds with the specified block
    void fill_space(int x0, int y0, int z0, int x1, int y1, int z1, Block_t block);

    // Adds a tree rooted at the provided position
    void add_tree(int tree_x, int tree_y, int tree_z);

    void clear_world();
} world_t;