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
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include "lodepng.h"
#include "glad.h"
#include "imstb_rectpack.h"

typedef struct __attribute__((packed)) {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} rgb_t;

extern GLuint generateTextureFromPaletteACT(const char* actFilename);

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
	Palette(const char* actFilename) : texture_id(generateTextureFromPaletteACT(actFilename)) {}
};

typedef struct {
	char filename[256];
	SffHeader header;
	std::vector<Sprite> sprites;
	std::vector<Palette> palettes;
	std::map<std::array<int, 2>, int> sprite_map; //map for sprite based on group and number
} Sff;

typedef struct {
	uint16_t width, height;
	struct stbrp_rect* rects;
	int usePalette;
} Atlas;

class DynamicLib {
public:
	DynamicLib(const std::string& path) {
#ifdef _WIN32
		handle = LoadLibraryA(path.c_str());
		if (!handle) {
			throw std::runtime_error("Failed to load library: " + path);
		}
#else
		handle = dlopen(path.c_str(), RTLD_LAZY);
		if (!handle) {
			throw std::runtime_error(std::string("Failed to load library: ") + dlerror());
		}
#endif
	}

	~DynamicLib() {
#ifdef _WIN32
		if (handle) FreeLibrary((HMODULE) handle);
#else
		if (handle) dlclose(handle);
#endif
	}

	template<typename Func>
	Func get(const std::string& name) {
#ifdef _WIN32
		FARPROC symbol = GetProcAddress((HMODULE) handle, name.c_str());
		if (!symbol) {
			throw std::runtime_error("Failed to find symbol: " + name);
		}
		return reinterpret_cast<Func>(symbol);
#else
		dlerror(); // clear any old error
		void* symbol = dlsym(handle, name.c_str());
		const char* error = dlerror();
		if (error) {
			throw std::runtime_error(std::string("Failed to find symbol: ") + error);
		}
		return reinterpret_cast<Func>(symbol);
#endif
	}

private:
	void* handle = nullptr;
};

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
int exportPalettedSpriteAsPng(Sprite& s, GLuint pal_texture_id, const char* filename);
int exportRGBASpriteAsPng(Sprite& s, const char* filename);