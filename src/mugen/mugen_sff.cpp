#include "mugen_sff.h"

// Palette Texture as GL_UNSIGNED_BYTE
GLuint generateTextureFromPaletteRGBA(uint32_t pal_rgba[256]) {
	GLuint tex;
	uint8_t pal_byte[256 * 4];

	// Convert the RGBA values into bytes (0-255 range for each channel)
	for (int i = 0; i < 256; i++) {
		pal_byte[i * 4 + 0] = (pal_rgba[i] >> 0) & 0xFF;
		pal_byte[i * 4 + 1] = (pal_rgba[i] >> 8) & 0xFF;
		pal_byte[i * 4 + 2] = (pal_rgba[i] >> 16) & 0xFF;
		pal_byte[i * 4 + 3] = (pal_rgba[i] >> 24) & 0xFF;  // Alpha
	}

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pal_byte);

	return tex;
}

GLuint generateTextureFromPaletteRGB(rgb_t pal_rgb[256]) {
	GLuint tex;
	uint8_t pal_byte[256 * 4];

	// Convert the RGB values into bytes (0-255 range for each channel)
	for (int i = 0; i < 256; i++) {
		pal_byte[i * 4 + 0] = i ? pal_rgb[i].r : 0;
		pal_byte[i * 4 + 1] = i ? pal_rgb[i].g : 0;
		pal_byte[i * 4 + 2] = i ? pal_rgb[i].b : 0;
		pal_byte[i * 4 + 3] = i ? 255 : 0;  // Set alpha to 0 for the first color (or 1.0 if required)
	}

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pal_byte);

	return tex;
}

GLuint generateTextureFromPaletteACT(const char* actFilename) {
	GLuint tex;
	uint8_t pal_byte[256 * 4];
	rgb_t pal_rgb[256];

	FILE* file = fopen(actFilename, "rb");
	if (!file) {
		memset(pal_rgb, 0, sizeof(rgb_t) * 256);
		printf("Failed to open palette file: %s\n", actFilename);
	}

	size_t read = fread(pal_rgb, sizeof(pal_rgb), 1, file); // Read 256 RGB triplets
	if (read != 1) {
		memset(pal_rgb, 0, sizeof(rgb_t) * 256);
		printf("Failed to read palette data from file: %s\n", actFilename);
	}
	fclose(file);

	// Convert the RGB values into bytes (0-255 range for each channel) in reverse order
	for (int i = 0; i < 256; i++) {
		pal_byte[i * 4 + 0] = pal_rgb[255 - i].r;
		pal_byte[i * 4 + 1] = pal_rgb[255 - i].g;
		pal_byte[i * 4 + 2] = pal_rgb[255 - i].b;
		pal_byte[i * 4 + 3] = i ? 255 : 0;
		// pal_byte[i * 4 + 3] = i == 255 ? 0 : 255;	// in some char, this works :(
	}

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pal_byte);

	return tex;
}

GLuint generateTextureFromSprite(GLuint spr_w, GLuint spr_h, uint8_t* spr_px) {
	unsigned int tex;

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, spr_w, spr_h, 0, GL_RED, GL_UNSIGNED_BYTE, spr_px);
	return tex;
}

GLuint generateTextureRGBAFromSprite(GLuint spr_w, GLuint spr_h, uint8_t* spr_px) {
	GLuint tex;

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	// glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, spr_w, spr_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, spr_px);
	return tex;
}

void printSprite(Sprite* sprite) {
	printf("Sprite: Group %d, Number %d, Size (%d,%d), Offset (%d,%d), palidx %d, rle %d, coldepth %d\n",
		sprite->Group, sprite->Number,
		sprite->Size[0], sprite->Size[1],
		sprite->Offset[0], sprite->Offset[1],
		sprite->palidx, -sprite->rle, sprite->coldepth);
}

int readSffHeader(Sff* sff, FILE* file, uint32_t* lofs, uint32_t* tofs) {
	// Validate header by comparing 12 first bytes with "ElecbyteSpr\x0"
	char headerCheck[12];
	fread(headerCheck, 12, 1, file);
	if (memcmp(headerCheck, "ElecbyteSpr\0", 12) != 0) {
		fprintf(stderr, "Invalid SFF file [%s]\n", headerCheck);
		return -1;
	}

	// Read versions in the header
	if (fread(&sff->header.Ver3, 1, 1, file) != 1) {
		fprintf(stderr, "Error reading version\n");
		return -1;
	}
	if (fread(&sff->header.Ver2, 1, 1, file) != 1) {
		fprintf(stderr, "Error reading version\n");
		return -1;
	}
	if (fread(&sff->header.Ver1, 1, 1, file) != 1) {
		fprintf(stderr, "Error reading version\n");
		return -1;
	}
	if (fread(&sff->header.Ver0, 1, 1, file) != 1) {
		fprintf(stderr, "Error reading version\n");
		return -1;
	}
	uint32_t dummy;
	if (fread(&dummy, sizeof(uint32_t), 1, file) != 1) {
		fprintf(stderr, "Error reading dummy\n");
		return -1;
	}

	if (sff->header.Ver0 == 2) {
		for (int i = 0; i < 4; i++) {
			if (fread(&dummy, sizeof(uint32_t), 1, file) != 1) {
				fprintf(stderr, "Error reading dummy\n");
				return -1;
			}
		}
		// read FirstSpriteHeaderOffset
		if (fread(&sff->header.FirstSpriteHeaderOffset, sizeof(uint32_t), 1, file) != 1) {
			fprintf(stderr, "Error reading FirstSpriteHeaderOffset\n");
			return -1;
		}
		// read NumberOfSprites
		if (fread(&sff->header.NumberOfSprites, sizeof(uint32_t), 1, file) != 1) {
			fprintf(stderr, "Error reading NumberOfSprites\n");
			return -1;
		}
		// read FirstPaletteHeaderOffset
		if (fread(&sff->header.FirstPaletteHeaderOffset, sizeof(uint32_t), 1, file) != 1) {
			fprintf(stderr, "Error reading FirstPaletteHeaderOffset\n");
			return -1;
		}
		// read NumberOfPalettes
		if (fread(&sff->header.NumberOfPalettes, sizeof(uint32_t), 1, file) != 1) {
			fprintf(stderr, "Error reading NumberOfPalettes\n");
			return -1;
		}
		// read lofs
		if (fread(lofs, sizeof(uint32_t), 1, file) != 1) {
			fprintf(stderr, "Error reading lofs\n");
			return -1;
		}
		if (fread(&dummy, sizeof(uint32_t), 1, file) != 1) {
			fprintf(stderr, "Error reading dummy\n");
			return -1;
		}
		// read tofs
		if (fread(tofs, sizeof(uint32_t), 1, file) != 1) {
			fprintf(stderr, "Error reading tofs\n");
			return -1;
		}
	} else if (sff->header.Ver0 == 1) {
		// read NumberOfSprites
		if (fread(&sff->header.NumberOfSprites, sizeof(uint32_t), 1, file) != 1) {
			fprintf(stderr, "Error reading NumberOfSprites\n");
			return -1;
		}
		// read FirstSpriteHeaderOffset
		if (fread(&sff->header.FirstSpriteHeaderOffset, sizeof(uint32_t), 1, file) != 1) {
			fprintf(stderr, "Error reading FirstSpriteHeaderOffset\n");
			return -1;
		}
		sff->header.FirstPaletteHeaderOffset = 0;
		sff->header.NumberOfPalettes = 0;
		*lofs = 0;
		*tofs = 0;
	} else {
		fprintf(stderr, "Unsupported SFF version: %d\n", sff->header.Ver0);
		return -1;
	}

	return 0;
}

int readSpriteHeaderV1(Sprite& sprite, FILE* file, uint32_t* ofs, uint32_t* size, uint16_t* link) {
	// Read ofs
	if (fread(ofs, sizeof(uint32_t), 1, file) != 1) {
		fprintf(stderr, "Error reading ofs\n");
		return -1;
	}
	// Read size
	if (fread(size, sizeof(uint32_t), 1, file) != 1) {
		fprintf(stderr, "Error reading size\n");
		return -1;
	}
	if (fread(&sprite.Offset[0], sizeof(int16_t), 1, file) != 1) {
		fprintf(stderr, "Error reading sprite offset\n");
		return -1;
	}
	if (fread(&sprite.Offset[1], sizeof(int16_t), 1, file) != 1) {
		fprintf(stderr, "Error reading sprite offset\n");
		return -1;
	}
	// Read sprite header
	if (fread(&sprite.Group, sizeof(int16_t), 1, file) != 1) {
		fprintf(stderr, "Error reading sprite group\n");
		return -1;
	}
	if (fread(&sprite.Number, sizeof(int16_t), 1, file) != 1) {
		fprintf(stderr, "Error reading sprite number\n");
		return -1;
	}
	// Read the link to the next sprite header
	if (fread(link, sizeof(uint16_t), 1, file) != 1) {
		fprintf(stderr, "Error reading sprite link\n");
		return -1;
	}
	// Print sprite header information
	// printf("Sprite v1 Group, Number: %d,%d size=%u Offset=%u,%u\n", sprite.Group, sprite.Number, *size, sprite.Offset[0], sprite.Offset[1]);
	return 0;
}

int readSpriteHeaderV2(Sprite& sprite, FILE* file, uint32_t* ofs, uint32_t* size, uint32_t lofs, uint32_t tofs, uint16_t* link) {
	// Read sprite header
	if (fread(&sprite.Group, sizeof(int16_t), 1, file) != 1) {
		fprintf(stderr, "Error reading sprite group\n");
		return -1;
	}
	if (fread(&sprite.Number, sizeof(int16_t), 1, file) != 1) {
		fprintf(stderr, "Error reading sprite number\n");
		return -1;
	}
	if (fread(&sprite.Size[0], sizeof(int16_t), 1, file) != 1) {
		fprintf(stderr, "Error reading sprite size\n");
		return -1;
	}
	if (fread(&sprite.Size[1], sizeof(int16_t), 1, file) != 1) {
		fprintf(stderr, "Error reading sprite size\n");
		return -1;
	}
	if (fread(&sprite.Offset[0], sizeof(int16_t), 1, file) != 1) {
		fprintf(stderr, "Error reading sprite offset\n");
		return -1;
	}
	if (fread(&sprite.Offset[1], sizeof(int16_t), 1, file) != 1) {
		fprintf(stderr, "Error reading sprite offset\n");
		return -1;
	}
	// Read the link to the next sprite header
	if (fread(link, sizeof(uint16_t), 1, file) != 1) {
		fprintf(stderr, "Error reading sprite link\n");
		return -1;
	}
	char format;
	if (fread(&format, sizeof(char), 1, file) != 1) {
		fprintf(stderr, "Error reading sprite format\n");
		return -1;
	}
	sprite.rle = -format;
	// Read color depth
	if (fread(&sprite.coldepth, sizeof(uint8_t), 1, file) != 1) {
		fprintf(stderr, "Error reading color depth\n");
		return -1;
	}
	// Read ofs
	if (fread(ofs, sizeof(uint32_t), 1, file) != 1) {
		fprintf(stderr, "Error reading ofs\n");
		return -1;
	}
	// Read size
	if (fread(size, sizeof(uint32_t), 1, file) != 1) {
		fprintf(stderr, "Error reading size\n");
		return -1;
	}
	uint16_t tmp;
	// Read tmp
	if (fread(&tmp, sizeof(uint16_t), 1, file) != 1) {
		fprintf(stderr, "Error reading tmp\n");
		return -1;
	}
	sprite.palidx = tmp;
	// Read tmp
	if (fread(&tmp, sizeof(uint16_t), 1, file) != 1) {
		fprintf(stderr, "Error reading tmp\n");
		return -1;
	}
	if ((tmp & 1) == 0) {
		*ofs += lofs;
	} else {
		*ofs += tofs;
	}

	return 0;
}

uint32_t readSffHeader(Sff* sff, uint8_t* data, uint32_t* lofs, uint32_t* tofs) {
	uint32_t offset = 0;

	// Validate header by comparing the first 12 bytes with "ElecbyteSpr\x0"
	char headerCheck[12];
	memcpy(headerCheck, data + offset, 12);
	offset += 12;
	if (memcmp(headerCheck, "ElecbyteSpr\0", 12) != 0) {
		fprintf(stderr, "Invalid SFF file [%s]\n", headerCheck);
		return 0;
	}

	// Read versions in the header
	sff->header.Ver3 = *(data + offset++);
	sff->header.Ver2 = *(data + offset++);
	sff->header.Ver1 = *(data + offset++);
	sff->header.Ver0 = *(data + offset++);

	uint32_t dummy;
	memcpy(&dummy, data + offset, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	if (sff->header.Ver0 == 2) {
		for (int i = 0; i < 4; i++) {
			memcpy(&dummy, data + offset, sizeof(uint32_t));
			offset += sizeof(uint32_t);
		}

		// Read FirstSpriteHeaderOffset
		memcpy(&sff->header.FirstSpriteHeaderOffset, data + offset, sizeof(uint32_t));
		offset += sizeof(uint32_t);

		// Read NumberOfSprites
		memcpy(&sff->header.NumberOfSprites, data + offset, sizeof(uint32_t));
		offset += sizeof(uint32_t);

		// Read FirstPaletteHeaderOffset
		memcpy(&sff->header.FirstPaletteHeaderOffset, data + offset, sizeof(uint32_t));
		offset += sizeof(uint32_t);

		// Read NumberOfPalettes
		memcpy(&sff->header.NumberOfPalettes, data + offset, sizeof(uint32_t));
		offset += sizeof(uint32_t);

		// Read lofs
		memcpy(lofs, data + offset, sizeof(uint32_t));
		offset += sizeof(uint32_t);

		memcpy(&dummy, data + offset, sizeof(uint32_t));
		offset += sizeof(uint32_t);

		// Read tofs
		memcpy(tofs, data + offset, sizeof(uint32_t));
		offset += sizeof(uint32_t);
	} else if (sff->header.Ver0 == 1) {
		// Read NumberOfSprites
		memcpy(&sff->header.NumberOfSprites, data + offset, sizeof(uint32_t));
		offset += sizeof(uint32_t);

		// Read FirstSpriteHeaderOffset
		memcpy(&sff->header.FirstSpriteHeaderOffset, data + offset, sizeof(uint32_t));
		offset += sizeof(uint32_t);

		sff->header.FirstPaletteHeaderOffset = 0;
		sff->header.NumberOfPalettes = 0;
		*lofs = 0;
		*tofs = 0;
	} else {
		fprintf(stderr, "Unsupported SFF version: %d\n", sff->header.Ver0);
		return 0;
	}

	return offset;
}

void spriteCopy(Sprite& dst, const Sprite& src) {
	dst.Group = src.Group;
	dst.Number = src.Number;
	dst.Size[0] = src.Size[0];
	dst.Size[1] = src.Size[1];
	dst.Offset[0] = src.Offset[0];
	dst.Offset[1] = src.Offset[1];
	dst.palidx = src.palidx;
	dst.rle = src.rle;
	dst.coldepth = src.coldepth;
	dst.texture_id = src.texture_id;
}

uint8_t* RlePcxDecode(Sprite& s, uint8_t* srcPx, size_t srcLen) {
	if (srcLen == 0) {
		fprintf(stderr, "Warning: PCX data length is zero\n");
		return NULL;
	}

	size_t dstLen = s.Size[0] * s.Size[1];
	uint8_t* dstPx = (uint8_t*) malloc(dstLen);
	if (!dstPx) {
		fprintf(stderr, "Error allocating memory for PCX decoded data dstLen=%zu srcLen=%zu (%dx%d)\n", dstLen, srcLen, s.Size[0], s.Size[1]);
		return NULL;
	}

	size_t i = 0; // input pointer
	size_t j = 0; // output pointer

	while (i < srcLen && j < dstLen) {
		uint8_t byte = srcPx[i++];
		int count = 1;

		if ((byte & 0xC0) == 0xC0) { // RLE marker
			count = byte & 0x3F;
			if (i < srcLen) {
				byte = srcPx[i++];
			} else {
				fprintf(stderr, "Warning: RLE marker at end of data\n");
				break;
			}
		}

		while (count-- > 0 && j < dstLen) {
			dstPx[j++] = byte;
		}
	}

	if (j < dstLen) {
		fprintf(stderr, "Warning: decoded PCX data shorter than expected (%zu vs %zu)\n", j, dstLen);
		// Fill the remaining bytes with 0 (or a background color)
		memset(dstPx + j, 0, dstLen - j);
	}

	return dstPx;
}


uint8_t* Rle8Decode(Sprite& s, uint8_t* srcPx, size_t srcLen) {
	if (srcLen == 0) {
		fprintf(stderr, "Warning RLE8 data length is zero\n");
		return NULL;
	}

	size_t dstLen = s.Size[0] * s.Size[1];
	uint8_t* dstPx = (uint8_t*) malloc(dstLen);
	if (!dstPx) {
		fprintf(stderr, "Error allocating memory for RLE decoded data\n");
		return NULL;
	}
	size_t i = 0, j = 0;
	// Decode the RLE data
	while (j < dstLen) {
		long n = 1;
		uint8_t d = srcPx[i];
		if (i < (srcLen - 1)) {
			i++;
		}
		if ((d & 0xc0) == 0x40) {
			n = d & 0x3f;
			d = srcPx[i];
			if (i < (srcLen - 1)) {
				i++;
			}
		}
		for (; n > 0; n--) {
			if (j < dstLen) {
				dstPx[j] = d;
				j++;
			}
		}
	}
	return dstPx;
}

uint8_t* Rle5Decode(Sprite& s, uint8_t* srcPx, size_t srcLen) {
	if (srcLen == 0) {
		fprintf(stderr, "Warning RLE5 data length is zero\n");
		return NULL;
	}

	size_t dstLen = s.Size[0] * s.Size[1];
	uint8_t* dstPx = (uint8_t*) malloc(dstLen);
	if (!dstPx) {
		fprintf(stderr, "Error allocating memory for RLE decoded data\n");
		return NULL;
	}

	size_t i = 0, j = 0;
	while (j < dstLen) {
		int rl = (int) srcPx[i];
		if (i < srcLen - 1) {
			i++;
		}
		int dl = (int) (srcPx[i] & 0x7f);
		uint8_t c = 0;
		if (srcPx[i] >> 7 != 0) {
			if (i < srcLen - 1) {
				i++;
			}
			c = srcPx[i];
		}
		if (i < srcLen - 1) {
			i++;
		}
		while (1) {
			if (j < dstLen) {
				dstPx[j] = c;
				j++;
			}
			rl--;
			if (rl < 0) {
				dl--;
				if (dl < 0) {
					break;
				}
				c = srcPx[i] & 0x1f;
				rl = (int) (srcPx[i] >> 5);
				if (i < srcLen - 1) {
					i++;
				}
			}
		}
	}

	return dstPx;
}

uint8_t* Lz5Decode(Sprite& s, uint8_t* srcPx, size_t srcLen) {
	if (srcLen == 0) {
		fprintf(stderr, "Warning LZ5 data length is zero\n");
		return NULL;
	}

	size_t dstLen = s.Size[0] * s.Size[1];
	uint8_t* dstPx = (uint8_t*) malloc(dstLen);
	if (!dstPx) {
		fprintf(stderr, "Error allocating memory for LZ5 decoded data\n");
		return NULL;
	}

	// Decode the LZ5 data
	size_t i = 0, j = 0;
	long n = 0;
	uint8_t ct = srcPx[i], cts = 0, rb = 0, rbc = 0;
	if (i < srcLen - 1) {
		i++;
	}

	while (j < dstLen) {
		int d = (int) srcPx[i];
		if (i < srcLen - 1) {
			i++;
		}

		if (ct & (1 << cts)) {
			if ((d & 0x3f) == 0) {
				d = (d << 2 | (int) srcPx[i]) + 1;
				if (i < srcLen - 1) {
					i++;
				}
				n = (int) srcPx[i] + 2;
				if (i < srcLen - 1) {
					i++;
				}
			} else {
				rb |= (uint8_t) ((d & 0xc0) >> rbc);
				rbc += 2;
				n = (int) (d & 0x3f);
				if (rbc < 8) {
					d = (int) srcPx[i] + 1;
					if (i < srcLen - 1) {
						i++;
					}
				} else {
					d = (int) rb + 1;
					rb = rbc = 0;
				}
			}
			for (;;) {
				if (j < dstLen) {
					dstPx[j] = dstPx[j - d];
					j++;
				}
				n--;
				if (n < 0) {
					break;
				}
			}
		} else {
			if ((d & 0xe0) == 0) {
				n = (int) srcPx[i] + 8;
				if (i < srcLen - 1) {
					i++;
				}
			} else {
				n = d >> 5;
				d &= 0x1f;
			}
			while (n-- > 0 && j < dstLen) {
				dstPx[j] = (uint8_t) d;
				j++;
			}
		}
		cts++;
		if (cts >= 8) {
			ct = srcPx[i];
			cts = 0;
			if (i < srcLen - 1) {
				i++;
			}
		}
	}

	return dstPx;
}

uint8_t* PngDecode(Sprite& s, uint8_t* data, uint32_t datasize) {
	lodepng::State state;
	unsigned int width = 0, height = 0;

	unsigned status = lodepng_inspect(&width, &height, &state, data, datasize);
	if (status) {
		fprintf(stderr, "Error inspecting PNG data: %s\n", lodepng_error_text(status));
		return NULL;
	}

	if (s.rle == -10)
		state.info_raw.colortype = LCT_PALETTE;
	else
		state.info_raw.colortype = LCT_RGBA;

	if (state.info_png.color.bitdepth == 16)
		state.info_raw.bitdepth = 16;
	else
		state.info_raw.bitdepth = 8;

	uint8_t* dstPx;
	status = lodepng_decode(&dstPx, &width, &height, &state, data, datasize);

	if (status != 0) {
		fprintf(stderr, "Could not decode PNG image(%s)", lodepng_error_text(status));
		return NULL;
	}
	s.Size[0] = width;
	s.Size[1] = height;
	return dstPx;
}

int readPcxHeader(Sprite& s, FILE* file, uint64_t offset) {
	fseek(file, offset, SEEK_SET);
	uint16_t dummy;
	if (fread(&dummy, sizeof(uint16_t), 1, file) != 1) {
		fprintf(stderr, "Error reading uint16_t dummy\n");
		return -1;
	}
	uint8_t encoding, bpp;
	if (fread(&encoding, sizeof(uint8_t), 1, file) != 1) {
		fprintf(stderr, "Error reading uint8_t encoding\n");
		return -1;
	}
	if (fread(&bpp, sizeof(uint8_t), 1, file) != 1) {
		fprintf(stderr, "Error reading uint8_t bpp\n");
		return -1;
	}
	if (bpp != 8) {
		fprintf(stderr, "Invalid PCX color depth: expected 8-bit, got %d", bpp);
		return -1;
	}
	uint16_t rect[4];
	if (fread(rect, sizeof(uint16_t), 4, file) != 4) {
		fprintf(stderr, "Error reading rectangle\n");
		return -1;
	}
	fseek(file, offset + 66, SEEK_SET);
	uint16_t bpl;
	if (fread(&bpl, sizeof(uint16_t), 1, file) != 1) {
		fprintf(stderr, "Error reading bpl\n");
		return -1;
	}
	s.Size[0] = rect[2] - rect[0] + 1;
	s.Size[1] = rect[3] - rect[1] + 1;
	s.rle = -1;	// -1 for PCX
	return 0;
}

uint8_t* readSpriteDataV1(Sprite& s, FILE* file, Sff* sff, uint64_t offset, uint32_t datasize, uint32_t nextSubheader, Sprite* prev, bool c00) {
	if (nextSubheader > offset) {
		// Ignore datasize except last
		datasize = nextSubheader - offset;
	}

	uint8_t ps;
	if (fread(&ps, sizeof(uint8_t), 1, file) != 1) {
		fprintf(stderr, "Error reading sprite ps data\n");
		return NULL;
	}
	bool paletteSame = ps != 0 && prev != NULL;
	if (readPcxHeader(s, file, offset) != 0) {
		fprintf(stderr, "Error reading sprite PCX header\n");
		return NULL;
	}

	fseek(file, offset + 128, SEEK_SET);
	// uint32_t palHash = 0;
	uint32_t palSize;
	if (c00 || paletteSame) {
		palSize = 0;
	} else {
		palSize = 768;
	}
	if (datasize < 128 + palSize) {
		datasize = 128 + palSize;
	}

	uint8_t* px = NULL;
	size_t srcLen = datasize - (128 + palSize);
	uint8_t* srcPx = (uint8_t*) malloc(srcLen);
	if (!srcPx) {
		fprintf(stderr, "Error allocating memory for sprite data\n");
		return NULL;
	}
	if (fread(srcPx, srcLen, 1, file) != 1) {
		fprintf(stderr, "Error reading sprite PCX data pixel\n");
		return NULL;
	}

	// printf("PCX: ps=%d ", ps);
	if (paletteSame) {
		if (prev != NULL) {
			s.palidx = prev->palidx;
			// printf("Info: Same palette (%d,%d) with (%d,%d) = %d\n", s->Group, s->Number, prev->Group, prev->Number, prev->palidx);
		}
		if (s.palidx < 0) {
			fprintf(stderr, "Error: invalid prev palette index %d\n", prev->palidx);
			return NULL;
		}
		px = RlePcxDecode(s, srcPx, srcLen);
	} else {
		if (c00) {
			fseek(file, offset + datasize - 768, 0);
		}
		rgb_t pal_rgb[256];
		if (fread(pal_rgb, sizeof(pal_rgb), 1, file) != 1) {
			fprintf(stderr, "Error reading palette rgb data\n");
			return NULL;
		}
		sff->palettes.emplace_back(generateTextureFromPaletteRGB(pal_rgb));
		s.palidx = sff->palettes.size() - 1;
		px = RlePcxDecode(s, srcPx, srcLen);
	}
	free(srcPx);
	return px;
}

uint8_t* readSpriteDataV2(Sprite& s, FILE* file, uint64_t offset, uint32_t datasize, Sff* sff) {
	uint8_t* px = NULL;
	if (s.rle > 0) return NULL;

	if (s.rle == 0) {
		px = (uint8_t*) malloc(datasize);
		if (!px) {
			fprintf(stderr, "Error allocating memory for sprite data\n");
			return NULL;
		}
		// Read sprite data
		fseek(file, offset, SEEK_SET);
		if (fread(px, datasize, 1, file) != 1) {
			fprintf(stderr, "Error reading V2 uncompress sprite data\n");
			free(px);
			return NULL;
		}
	} else {
		size_t srcLen;
		uint8_t* srcPx = NULL;
		fseek(file, offset + 4, SEEK_SET);
		int format = -s.rle;
		int rc;

		if (datasize < 4) {
			datasize = 4;
		}
		srcLen = datasize - 4;
		srcPx = (uint8_t*) malloc(srcLen);
		if (!srcPx) {
			fprintf(stderr, "Error allocating memory for sprite data\n");
			return NULL;
		}
		rc = fread(srcPx, srcLen, 1, file);
		if (rc != 1) {
			fprintf(stderr, "Error reading V2 RLE sprite data.\n");
			free(srcPx);
			return NULL;
		}

		switch (format) {
		case 2:
			px = Rle8Decode(s, srcPx, srcLen);
			break;
		case 3:
			px = Rle5Decode(s, srcPx, srcLen);
			break;
		case 4:
			px = Lz5Decode(s, srcPx, srcLen);
			break;
		case 10:
		case 11:
		case 12:
			px = PngDecode(s, srcPx, srcLen);
			break;
		}
		free(srcPx);
	}
	return px;
}

int loadMugenSprite(const char* filename, Sff* sff) {
	FILE* file;
	file = fopen(filename, "rb");
	if (!file) {
		printf("Error: can not open file %s\n", filename);
		return -1;
	}
	strncpy(sff->filename, filename, 255);

	uint32_t lofs, tofs;
	if (readSffHeader(sff, file, &lofs, &tofs) != 0) {
		printf("Error: reading header %s\n", filename);
		return -1;
	}

	// Print version
	if (sff->header.Ver0 != 1) {
		std::map<std::array<int, 2>, int> uniquePals;
		sff->palettes.clear();
		sff->palettes.reserve(sff->header.NumberOfPalettes);
		for (uint32_t i = 0; i < sff->header.NumberOfPalettes; i++) {
			fseek(file, sff->header.FirstPaletteHeaderOffset + i * 16, SEEK_SET);
			int16_t gn[3];
			if (fread(gn, sizeof(uint16_t), 3, file) != 3) {
				printf("Failed to read palette group: %s", filename);
				return -1;
			}
			// printf("Palette %d: Group %d, Number %d, ColNumber %d\n", i, gn[0], gn[1], gn[2]);

			uint16_t link;
			if (fread(&link, sizeof(uint16_t), 1, file) != 1) {
				printf("Failed to read palette link: %s", filename);
				return -1;
			}
			// printf("Palette link: %d\n", link);

			uint32_t ofs, siz;
			if (fread(&ofs, sizeof(uint32_t), 1, file) != 1) {
				printf("Failed to read palette offset: %s", filename);
				return -1;
			}
			if (fread(&siz, sizeof(uint32_t), 1, file) != 1) {
				printf("Failed to read palette size: %s", filename);
				return -1;
			}

			// Check if the palette is unique
			std::array<int, 2> key = { gn[0], gn[1] };
			if (uniquePals.find(key) == uniquePals.end()) {
				fseek(file, lofs + ofs, SEEK_SET);
				uint32_t rgba[256];
				if (fread(rgba, sizeof(uint32_t), 256, file) != 256) {
					printf("Failed to read palette data: %s", filename);
					return -1;
				}
				sff->palettes[i].texture_id = generateTextureFromPaletteRGBA(rgba);
				uniquePals[key] = i;
			} else {
				// If the palette is not unique, use the existing one
				printf("Palette %d(%d,%d) is not unique, using palette %d\nUntested code\n", i, gn[0], gn[1], uniquePals[key]);
				sff->palettes[i].texture_id = sff->palettes[uniquePals[key]].texture_id;
			}
		}
	}

	sff->sprites.clear();
	sff->sprites.reserve(sff->header.NumberOfSprites);
	Sprite* prev = NULL;
	long shofs = sff->header.FirstSpriteHeaderOffset;
	for (uint32_t i = 0; i < sff->header.NumberOfSprites; i++) {
		uint32_t xofs, size;
		uint16_t indexOfPrevious;
		fseek(file, shofs, SEEK_SET);
		switch (sff->header.Ver0) {
		case 1:
			if (readSpriteHeaderV1(sff->sprites[i], file, &xofs, &size, &indexOfPrevious) != 0) {
				return -1;
			}
			break;
		case 2:
			if (readSpriteHeaderV2(sff->sprites[i], file, &xofs, &size, lofs, tofs, &indexOfPrevious) != 0) {
				return -1;
			}
			break;
		}

		if (size == 0) {
			if (indexOfPrevious < i) {
				spriteCopy(sff->sprites[i], sff->sprites[indexOfPrevious]);
				// printf("Info: Sprite[%d] use prev Sprite[%d]\n", i, indexOfPrevious);
			} else {
				printf("Warning: Sprite %d has no size\n", i);
				sff->sprites[i].palidx = 0;
			}
		} else {
			uint8_t* data = NULL;
			bool character = true;
			switch (sff->header.Ver0) {
			case 1:
				data = readSpriteDataV1(sff->sprites[i], file, sff, shofs + 32, size, xofs, prev, character);
				if (!data) {
					fprintf(stderr, "Error reading sprite v1 data\n");
					return -1;
				}
				break;
			case 2:
				data = readSpriteDataV2(sff->sprites[i], file, xofs, size, sff);
				if (!data) {
					fprintf(stderr, "Error reading sprite v2 data\n");
					return -1;
				}
				break;
			default:
				fprintf(stderr, "Warning: unsupported sprite version: %d.x.x.x\n", sff->header.Ver0);
				break;
			}
			if (data) {
				if (sff->sprites[i].rle == -11 || sff->sprites[i].rle == -12)	// PNG Image (RGBA)
					sff->sprites[i].texture_id = generateTextureRGBAFromSprite(sff->sprites[i].Size[0], sff->sprites[i].Size[1], data);
				else	// Paletted Image (R only)
					sff->sprites[i].texture_id = generateTextureFromSprite(sff->sprites[i].Size[0], sff->sprites[i].Size[1], data);
				free(data);
			}

			// if use previous sprite Group 9000 and Number 0 only (fix for SFF v1)
			if (sff->sprites[i].Group == 9000) {
				if (sff->sprites[i].Number == 0) {
					prev = &sff->sprites[i];
				}
			} else {
				prev = &sff->sprites[i];
			}
		}

		if (sff->header.Ver0 == 1) {
			shofs = xofs;
		} else {
			shofs += 28;
		}

		// printSprite(&sff->sprites[i]);
	}

	// if SFF == v1 then update total palette
	if (sff->header.Ver0 == 1) {
		sff->header.NumberOfPalettes = sff->palettes.size();
	}

	return 0;
}

void deleteMugenSprite(Sff& sff) {
	uint32_t i;
	for (i = 0; i < sff.header.NumberOfPalettes; i++) {
		glDeleteTextures(1, &sff.palettes[i].texture_id);
	}
	for (i = 0; i < sff.header.NumberOfSprites; i++) {
		glDeleteTextures(1, &sff.sprites[i].texture_id);
	}
}

int exportRGBASpriteAsPng(Sprite& s, const char* filename) {
	if (s.rle != -11 && s.rle != -12) {	// PNG Image (RGBA)
		fprintf(stderr, "Error: sprite is not a RGBA image\n");
		return -1;
	}

	LodePNGState state;
	lodepng_state_init(&state);

	// Set color type to RGBA
	state.info_raw.colortype = LCT_RGBA;
	state.info_raw.bitdepth = 8;
	state.info_png.color.colortype = LCT_RGBA;
	state.info_png.color.bitdepth = 8;

	// Load the sprite data from GPU
	uint8_t data[s.Size[0] * s.Size[1] * 4]; // 4 bytes per pixel (RGBA)
	glBindTexture(GL_TEXTURE_2D, s.texture_id);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);  // Ensure byte-aligned rows
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	// Save the PNG file
	unsigned char* png = NULL;
	size_t pngsize = 0;
	int err_code = lodepng_encode(&png, &pngsize, data, s.Size[0], s.Size[1], &state);
	if (!err_code) {
		err_code = lodepng_save_file(png, pngsize, filename);
		if (err_code) {
			fprintf(stderr, "Error saving PNG file: %s\n", lodepng_error_text(err_code));
		}
	} else {
		fprintf(stderr, "Error encoding PNG data: %s\n", lodepng_error_text(err_code));
	}
	return 0;
}

int exportPalettedSpriteAsPng(Sprite& s, GLuint pal_texture_id, const char* filename) {
	if (s.rle != -1 && s.rle != -2 && s.rle != -3 && s.rle != -4 && s.rle != -10) {	// Paletted Image (R only)
		fprintf(stderr, "Error: sprite is not a paletted image. Compression method: %d\n", s.rle);
		return -1;
	}

	LodePNGState state;
	lodepng_state_init(&state);
	// Set color type to palette
	state.info_raw.colortype = LCT_PALETTE;
	state.info_raw.bitdepth = 8;
	state.info_png.color.colortype = LCT_PALETTE;
	state.info_png.color.bitdepth = 8;

	// Load the palette RGBA data from palette texture (in GPU)
	uint8_t palette_rgba[256 * 4]; // 256 colors, 4 bytes per color (RGBA)
	glBindTexture(GL_TEXTURE_2D, pal_texture_id);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);  // Ensure byte-aligned rows
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, palette_rgba);
	for (int i = 0; i < 256; i++) {
		// lodepng_palette_add(&state.info_raw, R,G,B,A);
		lodepng_palette_add(&state.info_raw, palette_rgba[i * 4 + 0], palette_rgba[i * 4 + 1], palette_rgba[i * 4 + 2], palette_rgba[i * 4 + 3]);
	}
	lodepng_palette_add(&state.info_png.color, 0, 0, 0, 0);	// atleast one color is needed for info_png palette. it will crash if not added

	// Load the sprite data from GPU
	uint8_t data[s.Size[0] * s.Size[1]];
	glBindTexture(GL_TEXTURE_2D, s.texture_id);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_BYTE, data);

	// Save the PNG file
	unsigned char* png = NULL;
	size_t pngsize = 0;
	int err_code = lodepng_encode(&png, &pngsize, data, s.Size[0], s.Size[1], &state);
	if (!err_code) {
		err_code = lodepng_save_file(png, pngsize, filename);
		if (err_code) {
			fprintf(stderr, "Error saving PNG file: %s\n", lodepng_error_text(err_code));
		}
	} else {
		fprintf(stderr, "Error encoding PNG data: %s\n", lodepng_error_text(err_code));
	}
	lodepng_state_cleanup(&state);
	free(png);
	return err_code;
}
