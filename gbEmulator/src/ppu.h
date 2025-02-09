#pragma once

#include <vector>
#include <queue>
#include <memory>
#include "mmu.h"


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
	uint8_t colorNum;
	PALETTE palette;
	int xPos;
	
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
	unsigned int tileNum; // byte 2

	uint8_t height;

	// byte 3
	SpriteFlags flags;

};

enum DRAWINGSTATE
{
	// BACKGROUND AND WINDOW
	BG_FETCH_TILE_NUM,
	BG_FETCH_TILE_LOW,
	BG_FETCH_TILE_HIGH,
	BG_PUSH_TO_FIFO,

	// SPRITE
	SP_FETCH_TILE_NUM,
	SP_FETCH_TILE_LOW,
	SP_FETCH_TILE_HIGH_AND_PUSH,
	
};

class PPU
{
public:
	PPU(MMU& mmu);

	void tick();

	void statInterruptCheck();

	std::vector<Sprite> spritesBuffer;
	unsigned int oamScanCounter = 0;

	unsigned int bgFetchTileNumCycles = 0;
	unsigned int bgFetchTileLowCycles = 0;
	unsigned int bgFetchTileHighCycles = 0;
	unsigned int bgPushToFifoCycles = 0;

	unsigned int spFetchTileNumCycles = 0;
	unsigned int spFetchTileLowCycles = 0;
	unsigned int spFetchTileHighCycles = 0;
	unsigned int spPushToFifoCycles = 0;

	DRAWINGSTATE bgDrawingState = BG_FETCH_TILE_NUM;
	DRAWINGSTATE spDrawingState = SP_FETCH_TILE_NUM;

	unsigned int currentBgTileNumber;
	unsigned int fetcherXPositionCounter = 0;

	int scanlineDrawnPixels = 0;

	MODE ppuMode = OAM_SCAN_2;

	std::queue<Pixel> bgPixelFIFO;
	
	// using a vector instead of a queue because we have to replace transparent obj pixels with opaque ones
	std::vector<Pixel> spPixelFIFO;


	Sprite spriteBeingFetched;
	uint16_t spFetchFirstByteAddress;
	uint8_t spFetchFirstByte;
	uint8_t spFetchSecondByte;

	void setMode(MODE mode);

	bool lycEqualLy = false;
	bool fetchingWindow = false;

	bool oldStat = false;
	bool curStat = true;

	bool lastSpriteTall = false;

	// count scanline cycles;
	int scanlineCycles = 0;

	bool isDisplayEnabled();
	bool windowTileMapSelect();
	bool isWindowDisplayEnabled();
	bool tileDataSelect();
	bool bgTileMapSelect();
	bool spriteTallMode();
	bool isSpriteEnabled();
	bool bgAndWindowEnabled();
	void reset();

	uint16_t bgFetchFirstByteAddress;
	bool firstBgFetch = true;
	bool spriteFound = false;
	bool displayOff = false;

	uint8_t bgFetchFirstByte;
	uint8_t bgFetchSecondByte;

	unsigned int windowLineCounter = 0;

	uint8_t getSCX();
	uint8_t getSCY();
	uint8_t getLY();
	uint8_t getLYC();
	uint8_t getWY();
	uint8_t getWX();
	void setLY(uint8_t value);
	bool checkLycEqualLy();

	void setSTATCoincidenceFlag(bool b);

	uint8_t getPixelColor(PALETTE p, uint8_t colorID);

	uint8_t LCD[160 * 144];

	int pixelsToBeDiscarded = 0;
	int numOfPixelsDiscarded = 0;

	int LX = 0;

	bool wyEqualLy = false;
	bool wyEqualLyThisFrame = false;
	bool resetForWindowFetch = false;
	bool windowPixelWasDrawn = false;
	bool calculateDiscardedPixels = true;

	bool spriteFetchEnabled = false;

	bool discardPixels = true;

	int hBlankDuration = 0;
	bool exittedDrawingMode = false;
	bool firstHBlankCycle = true;
	unsigned int currentSpTileNumber;

	std::queue<Pixel> bgFetchBuffer;

	int vBlankCycleCounter = 0;

	uint8_t colors[4] =  { 0xFF, 0xAA, 0x55, 0x00 };

private:
	MMU& mmu;
};

