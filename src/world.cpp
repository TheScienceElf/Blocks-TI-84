#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "world.h"
#include "draw.h"
#include "block.h"
#include "textures.h"

/* The projected depth of each block from the view of the sun.
*  
*  There was not a large enough contiguous piece of memory to include this in the 
*  world struct.
*
*  I just put this in a random spot in RAM that looked open, that could
*  very well be a mistake
*/
uint8_t *tri_grid_shadow = (uint8_t*)0xD3C000;

/* Populates the LUTs for indexing into the trigrid */
void world::init_tri_grid() {
    // The starting index (in the overall array) of this row
    int idx = 0;
    // The width in triangles of this row
    int width = 0;
    // Bottom diamond
    for(int row = 0; row < WORLD_SIZE; row++) {
        // Width increases with each row
        width += 2;
        // Store the start index of this row, and compute the index for the next row
        tri_grid_rows[row] = idx;
        idx += width;
        // Compute the pixel offset of the leftmost trangle based on the width of the row
        tri_grid_row_px_offset[row] = (width * 8) - 16;
        // Compute the trigrid offset based on how this row width deviates from the width
        // of an infinite trigrid
        tri_grid_row_offset[row] = (width - (2 * row + 2)) / 2;
        
        tri_grid_row_width[row] = width;
    }
    // Middle rows (same as above, but width is constant)
    for(int row = WORLD_SIZE; row < WORLD_SIZE + WORLD_HEIGHT + WORLD_HEIGHT - 1; row++) {
        tri_grid_rows[row] = idx;
        idx += width;
        tri_grid_row_px_offset[row] = (width * 8) - 16;
        tri_grid_row_offset[row] = (width - (2 * row + 2)) / 2;
        tri_grid_row_width[row] = width;
    }
    // Upper diamond (same as above, but width decreases)
    for(int row = WORLD_SIZE + WORLD_HEIGHT + WORLD_HEIGHT - 1; row < ROW_CNT; row++) {
        tri_grid_rows[row] = idx;
        idx += width;
        tri_grid_row_px_offset[row] = (width * 8) - 16;
        tri_grid_row_offset[row] = (width - (2 * row + 2)) / 2;
        tri_grid_row_width[row] = width;
        width -= 2;
    }
    // Clear the trigrid
    memset(tri_grid_tex, AIR, TRI_CNT);
    memset(tri_grid_flags, 0, TRI_CNT);
    memset(tri_grid_depth, 255, TRI_CNT);
    memset(tri_grid_shadow, 255, TRI_CNT);
}

/* Sweeps through blocks in the world starting from (x, y, z) and apply steps of
* (dx, dy, dz) to offset the search. Return true if we find a solid block, false
* if we reach the world border first
*/
bool world::sweep_ray(int x, int y, int z, int dx, int dy, int dz) {
    int bx = x;
    int by = y;
    int bz = z;
    while(0 <= bx && bx < WORLD_SIZE && 0 <= by && by < WORLD_HEIGHT && 0 <= bz && bz < WORLD_SIZE) {
        if(blocks[by][bx][bz] != AIR) return true;
        bx += dx;
        by += dy;
        bz += dz;
    }
    return false;
}

/* Sweep through the world from a certain point to see if if the top of
* the block is in shadow. Returns the proper flags for that face
*/
uint8_t world::compute_top_shadow(uint8_t x, uint8_t y, uint8_t z) {
    uint8_t shadow = SHADOW_NONE;
    uint8_t depth = project_light_depth(x, y, z);
    // Transform the position to the perspective centered 
    // around the light source
    int sx = (WORLD_SIZE - 1 - z);
    int sy = y;
    int sz = x;
    
    // We then turn these rules into a linear transformation
    int row = sx + sy + sy + sz + 2;
    int idx = sx + sx + sy + sy + tri_grid_row_offset[row] + 2;
    int tri_grid_idx = tri_grid_rows[row] + idx;
    
    if(tri_grid_shadow[tri_grid_idx] < depth) 
        shadow |= SHADOW_TOP;
    tri_grid_idx++;
    if(tri_grid_shadow[tri_grid_idx] < depth) 
        shadow |= SHADOW_BOTTOM;
    return shadow;
}

/* Sweep through the world from a certain point to see if if the left side
* of the block is in shadow. Returns the proper flags for that face
*/
uint8_t world::compute_left_shadow(uint8_t x, uint8_t y, uint8_t z) {
    uint8_t shadow = SHADOW_NONE;
    uint8_t depth = project_light_depth(x, y, z);
    // Transform the position to the perspective centered 
    // around the light source
    int sx = (WORLD_SIZE - 1 - z);
    int sy = y;
    int sz = x;
    
    // We then turn these rules into a linear transformation        
    int row = sx + sy + sy + sz + 1;
    int idx = sx + sx + sy + sy + tri_grid_row_offset[row] + 2;
    int tri_grid_idx = tri_grid_rows[row] + idx;
    
    if(tri_grid_shadow[tri_grid_idx] < depth) 
        shadow |= SHADOW_TOP;
    
    // We then turn these rules into a linear transformation
    row = sx + sy + sy + sz + 0;
    idx = sx + sx + sy + sy + tri_grid_row_offset[row] + 1;
    tri_grid_idx = tri_grid_rows[row] + idx;
    
    if(tri_grid_shadow[tri_grid_idx] < depth) 
        shadow |= SHADOW_BOTTOM;
    return shadow;
}

/* On an infinite grid, we can increment each coordinate using these rules
*  Increment x: row++, idx++
*  Increment y: row += 2, idx++
*  Increment z: row++
* 
*  s represents the pair of triangles we are projecting to
*  0 - bottom, 1 - mid, 2 - top
*/
int world::project(uint8_t x, uint8_t y, uint8_t z, uint8_t s) {
    int row = x + y + y + z + s;
    int idx = x + x + y + y + tri_grid_row_offset[row] + s;
    return tri_grid_rows[row] + idx;
}

void world::unproject(int row, int idx, int depth, uint8_t &x, uint8_t &y, uint8_t &z) {
    int A = row;
    int B = idx - tri_grid_row_offset[row];
    int C = depth - (WORLD_HEIGHT - 1);
    // Little offset to fix the rounding error from the inversion
    // I worked this out through trial and error, but I believe it
    // accounts for left and right facing triangles.
    uint8_t z_offset = B % 2 == 0 ? 2 : 4;
    // Compute the inverse projection transformation
    x = (-2 * A + 3 * B + 2 * C) / 6;
    y = ( 2 * A + 0 * B - 2 * C) / 6;
    z = ( 4 * A - 3 * B + 2 * C + z_offset) / 6;
}

// Adds this block to the world's data structures and applies any necessary masks
void world::set_block(int x, int y, int z, Block_t block) {
    blocks[y][x][z] = block;
    
    // A lookup table for the order of faces we draw
    uint8_t faces[6] = {LEFT_FACE, RIGHT_FACE, LEFT_FACE, RIGHT_FACE, TOP_FACE, TOP_FACE};
    uint8_t face_shadows[3];

    face_shadows[TOP_FACE]  = compute_top_shadow(x, y, z);
    face_shadows[LEFT_FACE] = compute_left_shadow(x, y, z);
    face_shadows[RIGHT_FACE] = SHADOW_NONE;
    

    uint8_t block_depth = project_view_depth(x, y, z);

    uint8_t i = 0;
    for(int s = 0; s < 3; s++) {
        int tri_grid_idx = project(x, y, z, s);
        for(int t = 0; t < 2; t++) {
            uint8_t depth = block_depth;
            uint8_t tri_depth = tri_grid_depth[tri_grid_idx];
            uint8_t water = WATER_NONE;
            
            // If the face is occluded, but the cell is flagged with water
            // manually search for the frontmost solid block face to check if
            // this face is really occluded
            if(tri_depth < depth && tri_grid_flags[tri_grid_idx] & WATER_MASK) {
                int row = x + y + y + z + s;
                int idx = x + x + y + y + tri_grid_row_offset[row] + s + t;
                uint8_t ux, uy, uz;
                if(scan_tri(row, idx, tri_depth, ux, uy, uz, WATER)) {
                    tri_depth = project_view_depth(ux, uy, uz);
                }

                // If this block is below water level, just copy the water flags already
                // in this triangle
                water = tri_grid_flags[tri_grid_idx] & WATER_MASK;
                depth = tri_grid_depth[tri_grid_idx];
            }

            if(tri_depth >= block_depth) {
                uint8_t face = faces[i];
                tri_grid_tex[tri_grid_idx] = block;
                tri_grid_flags[tri_grid_idx] = face | face_shadows[face] | water;
                tri_grid_depth[tri_grid_idx] = depth;
            }
            tri_grid_idx++;
            i++;
        }
    }
}


// Recomputes which shadow masks should be used for the block at the given position
void world::refresh_shadows(int x, int y, int z) {
    expand_draw_region(x, y, z);

    uint8_t top_shadow  = compute_top_shadow(x, y, z);
    uint8_t left_shadow = compute_left_shadow(x, y, z);
    uint8_t depth = project_view_depth(x, y, z);

    // Blank out all 6 triangles this block covers
    for(uint8_t s = 0; s < 3; s++) {
        int tri_grid_idx = project(x, y, z, s);

        for(uint8_t t = 0; t < 2; t++) {
            uint8_t tri_depth = tri_grid_depth[tri_grid_idx];

            // If the face is occluded, but the cell is flagged with water
            // manually search for the frontmost solid block face to check if
            // this face is really occluded
            if(tri_depth < depth && tri_grid_flags[tri_grid_idx] & WATER_MASK) {
                int row = x + y + y + z + s;
                int idx = x + x + y + y + tri_grid_row_offset[row] + s + t;
                uint8_t ux, uy, uz;
                if(scan_tri(row, idx, tri_depth, ux, uy, uz, WATER)) {
                    tri_depth = project_view_depth(ux, uy, uz);
                }
            }

            if(tri_depth == depth) {
                uint8_t face = tri_grid_flags[tri_grid_idx] & FACE_MASK;

                if(face == TOP_FACE) {
                    tri_grid_flags[tri_grid_idx] &= ~SHADOW_MASK;
                    tri_grid_flags[tri_grid_idx] |= top_shadow;
                }
                if(face == LEFT_FACE) {
                    tri_grid_flags[tri_grid_idx] &= ~SHADOW_MASK;
                    tri_grid_flags[tri_grid_idx] |= left_shadow;
                }
            }
            tri_grid_idx++;
        }
    }
}

// Adds a water block to the world's data structures and applies the water mask where appropriate
void world::set_water(int x, int y, int z) {
    blocks[y][x][z] = WATER;

    uint8_t water_left[3]  = { WATER_FULL, WATER_FULL, WATER_FULL };
    uint8_t water_right[3] = { WATER_FULL, WATER_FULL, WATER_FULL };

    // Some logic to determine which water mask pattern should be used on the triangles this block covers
    if((y == WORLD_HEIGHT - 1) || (blocks[y + 1][x][z] != WATER)) {    
        if((z == WORLD_SIZE - 1) || (blocks[y][x][z + 1] != WATER))
            water_left[MID_FACE]  = WATER_HALF;
        if((x == WORLD_SIZE - 1) || (blocks[y][x + 1][z] != WATER))
            water_right[MID_FACE] =  WATER_HALF;

        if((water_left[MID_FACE] == WATER_HALF) || (x == WORLD_SIZE - 1) || (blocks[y][x + 1][z + 1] != WATER))
            water_left[TOP_FACE]  = WATER_HALF;
        if((water_right[MID_FACE] == WATER_HALF) || (z == WORLD_SIZE - 1) || (blocks[y][x + 1][z + 1] != WATER))
            water_right[TOP_FACE] =  WATER_HALF;
    }
    uint8_t depth = project_view_depth(x, y, z);
    
    // Loop over all 6 triangles this block covers
    for(uint8_t s = 0; s < 3; s++) {
        int tri_grid_idx = project(x, y, z, s);
        if(tri_grid_depth[tri_grid_idx] > depth) {
            tri_grid_flags[tri_grid_idx] &= ~WATER_MASK;
            tri_grid_flags[tri_grid_idx] |= water_left[s];
            tri_grid_depth[tri_grid_idx] = depth;
        }
        tri_grid_idx++;
        if(tri_grid_depth[tri_grid_idx] > depth) {
            tri_grid_flags[tri_grid_idx] &= ~WATER_MASK;
            tri_grid_flags[tri_grid_idx] |= water_right[s];
            tri_grid_depth[tri_grid_idx] = depth;
        }
    }
}

// Updates the shadow map with a solid block at the given position
void world::set_block_shadow(int x, int y, int z) {
    uint8_t depth = project_light_depth(x, y, z);
    // Loop over all 6 triangles this block covers
    for(uint8_t s = 0; s < 3; s++) {
        int row = x + y + y + (WORLD_SIZE - 1 - z) + s;
        int idx = (WORLD_SIZE - 1 - z)+ (WORLD_SIZE - 1 - z) + y + y + tri_grid_row_offset[row] + s;
        int tri_grid_idx = tri_grid_rows[row] + idx;
        // Update the shadow depth if this block is not already in shadow
        if(tri_grid_shadow[tri_grid_idx] > depth)
            tri_grid_shadow[tri_grid_idx] = depth;
        tri_grid_idx++;
        if(tri_grid_shadow[tri_grid_idx] > depth)
            tri_grid_shadow[tri_grid_idx] = depth;
    }
}

// Inserts a block into the world data structures and updates any blocks
// which may be shadowed by it
void world::place_block(int x, int y, int z, Block_t block) {
    // Compute the block position along the shadow map
    uint8_t shadow_x, shadow_y, shadow_z;
    
    to_shadow_space(x, y, z, shadow_x, shadow_y, shadow_z);
    
    uint8_t x_update[6];
    uint8_t y_update[6];
    uint8_t z_update[6];
    
    uint8_t i = 0;
    for(int t = 0; t < 2; t++) {
    for(int s = 0; s < 3; s++) {
        int row = shadow_x + shadow_y + shadow_y + shadow_z + s;
        int idx = shadow_x + shadow_x + shadow_y + shadow_y + tri_grid_row_offset[row] + s + t;
        int depth = project_view_depth(x, y, z);
        depth = tri_grid_shadow[tri_grid_rows[row] + idx];
        // Reverse the projection in shadow space
        uint8_t xs, ys, zs;
        unproject(row, idx, depth, xs, ys, zs);
        from_shadow_space(xs, ys, zs, x_update[i], y_update[i], z_update[i]);
        i++;
    }
    }
    
    set_block_shadow(x, y, z);
    set_block(x, y, z, block);
    expand_draw_region(x, y, z);
    // Make the blocks now in shadow update their shadow flags
    for(uint8_t i = 0; i < 6; i++)
        refresh_shadows(x_update[i], y_update[i], z_update[i]);
}

// Search along a triangle in screen-space for the first solid block under it
// Use skip = WATER for strictly solid blocks and skip = AIR for all non-empty blocks
bool world::scan_tri(int row, int idx, int depth, uint8_t &x, uint8_t &y, uint8_t &z, Block_t skip) {
    while(true) {
        unproject(row, idx, depth, x, y, z);
        if(x >= WORLD_SIZE || y >= WORLD_HEIGHT || z >= WORLD_SIZE) return false;
        if(blocks[y][x][z] > skip) return true;
        depth++;
    }
}

// Search along a triangle in shadow-space for the first solid block under it
bool world::scan_shadow(int row, int idx, int depth, uint8_t &x, uint8_t &y, uint8_t &z) {
    while(true) {
        uint8_t sx, sy, sz;
        unproject(row, idx, depth, sx, sy, sz);
        if(sx >= WORLD_SIZE || sy >= WORLD_HEIGHT || sz >= WORLD_SIZE) return false;
        from_shadow_space(sx, sy, sz, x, y, z);
        if(blocks[y][x][z] > WATER) return true;
        depth++;
    }
}

// Removes the block at a given position from all world data structures and updates any blocks
// which become unshadowed
void world::remove_block(int x, int y, int z) {
    Block_t orig_block = blocks[y][x][z];
    blocks[y][x][z] = AIR;
    
    expand_draw_region(x, y, z);

    uint8_t i = 0;
    uint8_t tri_depths[6];


    int depth = project_view_depth(x, y, z);
    // Blank out all 6 triangles this block covers
    for(uint8_t s = 0; s < 3; s++) {
        int tri_grid_idx = project(x, y, z, s);
        for(uint8_t t = 0; t < 2; t++) {
            uint8_t tri_depth = tri_grid_depth[tri_grid_idx];
            tri_depths[i++] = tri_depth;

            // If the face is occluded, but the cell is flagged with water
            // manually search for the frontmost solid block face to check if
            // this face is really occluded
            // Ignore this step though if we're removing a water block
            if(tri_depth < depth && tri_grid_flags[tri_grid_idx] & WATER_MASK && (orig_block != WATER)) {
                int row = x + y + y + z + s;
                int idx = x + x + y + y + tri_grid_row_offset[row] + s + t;
                uint8_t ux, uy, uz;
                if(scan_tri(row, idx, tri_depth, ux, uy, uz, WATER)) {
                    tri_depth = project_view_depth(ux, uy, uz);
                }
                else
                {
                    tri_depth = 255;
                }
            }

            if(tri_depth >= depth) {
                tri_grid_tex[tri_grid_idx] = AIR;
                tri_grid_depth[tri_grid_idx] = 255;
                tri_grid_flags[tri_grid_idx] = 0;
            }
            tri_grid_idx++;
        }
    }

    i = 0;

    // Redraw all the uncovered blocks
    for(uint8_t s = 0; s < 3; s++) {
        for(uint8_t t = 0; t < 2; t++) {
            int row = x + y + y + z + s;
            int idx = x + x + y + y + tri_grid_row_offset[row] + s + t;
            uint8_t ux, uy, uz;

            if(scan_tri(row, idx, tri_depths[i], ux, uy, uz, AIR)) {
                if(blocks[uy][ux][uz] == WATER) {
                    uint8_t wx, wy, wz;
                    wx = ux;
                    wy = uy;
                    wz = uz;

                    if(scan_tri(row, idx, tri_depths[i], ux, uy, uz, WATER)) {
                        set_block(ux, uy, uz, blocks[uy][ux][uz]);
                    }
                    set_water(wx, wy, wz);
                }
                else
                {
                    set_block(ux, uy, uz, blocks[uy][ux][uz]);
                }

            }
            i++;
        }
    }

    int shad_depth = project_light_depth(x, y, z);
    uint8_t sx, sy, sz;
    to_shadow_space(x, y, z, sx, sy, sz);
    // Blank out all 6 triangles this block shades
    for(uint8_t s = 0; s < 3; s++) {
        int tri_grid_idx = project(sx, sy, sz, s);
        if(tri_grid_shadow[tri_grid_idx] >= shad_depth) {
            tri_grid_shadow[tri_grid_idx] = 255;
        }
        tri_grid_idx++;
        if(tri_grid_shadow[tri_grid_idx] >= shad_depth) {
            tri_grid_shadow[tri_grid_idx] = 255;
        }
    }
    // Redraw all the unshadowed blocks
    for(uint8_t s = 0; s < 3; s++) {
        int row = sx + sy + sy + sz + s;
        int idx = sx + sx + sy + sy + tri_grid_row_offset[row] + s;
        uint8_t ux, uy, uz;
        if(scan_shadow(row, idx, shad_depth, ux, uy, uz)) {
            set_block_shadow(ux, uy, uz);
            refresh_shadows(ux, uy, uz);
        }
        idx++;
        if(scan_shadow(row, idx, shad_depth, ux, uy, uz)) {
            set_block_shadow(ux, uy, uz);
            refresh_shadows(ux, uy, uz);
        }
    }
}

// Inclusively fills the space within the provided bounds with the specified block
void world::fill_space(int x0, int y0, int z0, int x1, int y1, int z1, Block_t block) {
    for(int y = y0; y <= y1; y++) {
        for(int x = x0; x <= x1; x++) {
            for(int z = z0; z <= z1; z++) {
                blocks[y][x][z] = block;
            } 
        }
    }
}

// Adds a tree rooted at the provided position
void world::add_tree(int tree_x, int tree_y, int tree_z) {
    // Add leaves
    fill_space(tree_x - 2, tree_y + 3, tree_z - 2, tree_x + 2, tree_y + 4, tree_z + 2, LEAVES);
    fill_space(tree_x - 1, tree_y + 5, tree_z - 1, tree_x + 1, tree_y + 5, tree_z + 1, LEAVES);
    
    blocks[tree_y + 6][tree_x + 1][tree_z    ] = LEAVES;
    blocks[tree_y + 6][tree_x - 1][tree_z    ] = LEAVES;
    blocks[tree_y + 6][tree_x    ][tree_z + 1] = LEAVES;
    blocks[tree_y + 6][tree_x    ][tree_z - 1] = LEAVES;
    blocks[tree_y + 6][tree_x][tree_z] = LEAVES;
    // Add tree trunk
    fill_space(tree_x, tree_y, tree_z, tree_x, tree_y + 5, tree_z, WOOD);
}

void world::clear_world() {
    fill_space(0, 0, 0, WORLD_SIZE - 1, WORLD_HEIGHT - 1, WORLD_SIZE - 1, AIR);
}