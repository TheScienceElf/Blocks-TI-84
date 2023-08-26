#pragma once
#include <stdint.h>

#define TEX_CNT 23

// The size of a single triangle texture
#define TEX_SIZE 128

// Flags for the alternate palettes
#define SHADOW 64
#define UNDERWATER 128

// Color for background drawing
#define SKY 0

// A texture is a set of 6 triangle sprites
typedef uint8_t Texture_t[6][TEX_SIZE];

// The color palette for all our textures
extern uint16_t tex_palette[256];


extern Texture_t textures[TEX_CNT];

extern Texture_t player_tex;

#define SHADOW_NONE 0
#define SHADOW_BOTTOM 4
#define SHADOW_TOP 8
#define SHADOW_FULL 12

#define SHADOW_MASK 12
#define SHADOW_OFFSET 2

extern uint8_t* shadow_masks[4][6];


#define WATER_NONE 0
#define WATER_HALF 16
#define WATER_FULL 32

#define WATER_MASK 48
#define WATER_OFFSET 4

extern uint8_t* water_masks[3][6];


void init_palette();