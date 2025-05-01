#pragma once

// C headers
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// C++ headers
#include <map>
#include <vector>
#include <array>
#include <string>
#include <algorithm>

#include "lodepng.h"
#include "glad.h"

#define MAX_PALETTES 64

typedef struct __attribute__((packed)) {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} rgb_t;

extern GLuint generateTextureFromPaletteRGB(rgb_t pal_rgb[256]);

// SFF
typedef struct {
	uint8_t Ver3, Ver2, Ver1, Ver0;
	uint32_t FirstSpriteHeaderOffset;
	uint32_t FirstPaletteHeaderOffset;
	uint32_t NumberOfSprites;
	uint32_t NumberOfPalettes;
} SffHeader;

class Sprite {
public:
	uint16_t Group;
	uint16_t Number;
	uint16_t Size[2];
	int16_t Offset[2];
	int palidx;
	int rle;
	uint8_t coldepth;
	unsigned int texture_id;
	size_t atlas_x, atlas_y;

	// Constructor!
	Sprite(uint16_t group, uint16_t number,
		uint16_t width, uint16_t height,
		int16_t offset_x, int16_t offset_y,
		int pal, int rle_type, uint8_t depth,
		unsigned int tex_id, size_t ax, size_t ay)
		: Group(group), Number(number), palidx(pal), rle(rle_type),
		coldepth(depth), texture_id(tex_id), atlas_x(ax), atlas_y(ay) {
		Size[0] = width;
		Size[1] = height;
		Offset[0] = offset_x;
		Offset[1] = offset_y;
	}
};

class Palette {
public:
	unsigned int texture_id;

	Palette(unsigned int id) : texture_id(id) {}
	// Constructor from .ACT filename
	Palette(const char* actFilename) {
		rgb_t pal_rgb[256];
		FILE* file = fopen(actFilename, "rb");
		printf("\n[mugen_sff.h] Open palette file: %s\n", actFilename);
		if (!file) {
			memset(pal_rgb, 0, sizeof(rgb_t) * 256);
			printf("Failed to open palette file: %s\n", actFilename);
			return;
		}

		size_t read = fread(pal_rgb, sizeof(pal_rgb), 1, file); // Read 256 RGB triplets
		if (read != 1) {
			memset(pal_rgb, 0, sizeof(rgb_t) * 256);
			printf("Failed to read palette data from file: %s\n", actFilename);
		}
		fclose(file);
		texture_id = generateTextureFromPaletteRGB(pal_rgb);
	}
};

typedef struct {
	char filename[256];
	SffHeader header;
	std::vector<Sprite> sprites;
	std::vector<Palette> palettes;
} Sff;

typedef struct {
	uint16_t width, height;
	struct stbrp_rect* rects;
	Sff* sff;
	int usePalette;
} Atlas;

// Load Sprite from file
int readSffHeader(Sff* sff, FILE* file, uint32_t* lofs, uint32_t* tofs);
int readSpriteHeaderV1(Sprite* sprite, FILE* file, uint32_t* ofs, uint32_t* size, uint16_t* link);
int readSpriteHeaderV2(Sprite* sprite, FILE* file, uint32_t* ofs, uint32_t* size, uint32_t lofs, uint32_t tofs, uint16_t* link);
uint8_t* readSpriteDataV1(Sprite* sprite, FILE* file, Sff* sff, uint64_t offset, uint32_t datasize, uint32_t nextSubheader, Sprite* prev, bool c00);
uint8_t* readSpriteDataV2(Sprite* sprite, FILE* file, uint64_t offset, uint32_t datasize, Sff* sff);

void spriteCopy(Sprite* dst, const Sprite* src);
void printSprite(Sprite* sprite);
int loadMugenSprite(const char* filename, Sff* sff);
void deleteMugenSprite(Sff& sff);
