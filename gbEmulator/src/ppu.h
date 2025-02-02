#pragma once

#include <memory>
#include "mmu.h"
#include <vector>
#include <queue>

enum MODE
{
	HBLANK_0 = 0,
	VBLANK_1,
	OAM_SCAN_2,
	DRAWING_3
};

enum PALETTE
{
	// for sprites
	OBP0,
	OBP1, 

	// for background
	BGP
};

struct Pixel
{
	// color number (ignoring the palette) --> color value from tile data
	uint8_t color;
	PALETTE palette;
	
	// for CGB there is also a prite priority thing

	uint8_t backgroundPrio; // only relevant for sprites keeps the value of bit7 (obj to bg prio) of the sprite flags
};


struct SpriteFlags
{
	bool prio; // bit 7
	bool yFlip; // bit 6
	bool xFlip; // bit 5
	bool palette; // bit 4

	// bit 0-3 are CGB only
};

struct Sprite
{

	uint8_t yPos; // byte 0
	uint8_t xPos; // byte 1
	uint8_t tileNum; // byte 2

	// byte 3
	SpriteFlags flags;

};


class PPU
{
public:
	PPU(MMU& mmu);

	void tick();

	void statInterruptCheck();

	std::vector<Sprite> spritesBuffer;
	int oamScanCounter = 0;

	MODE ppuMode;

	std::queue<Pixel> bgPixelFIFO;
	std::queue<Pixel> spPixelFIFO;

	void setMode(MODE mode);

	bool lycEqualLy = false;

	bool oldStat = false;
	bool curStat = true;

	// count scanline cycles;
	int scanlineCycles = 0;

private:
	MMU& mmu;
};

