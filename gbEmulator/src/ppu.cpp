#include "ppu.h"
#include <iostream>

#define OAM_START_ADDRESS 0xFE00
#define LY_ADDRESS 0xFF44
#define LYC_ADDRESS 0xFF45
#define LCDC_ADDRESS 0xFF40
#define STAT_ADDRESS 0xFF41


PPU::PPU(MMU& mmu) 
	: mmu(mmu) 
{
	setLY(0);
};

void PPU::statInterruptCheck()
{
	// A STAT IRQ happens when both the corresponding bit in the STAT is enabled and the condition happens. 
	// VBLANK has its own interrupt
	curStat =
		(mmu.read8(STAT_ADDRESS) >> 3 & 1) && ppuMode == (MODE)0 ||
		(mmu.read8(STAT_ADDRESS) >> 4 & 1) && ppuMode == (MODE)1 ||
		(mmu.read8(STAT_ADDRESS) >> 5 & 1) && ppuMode == (MODE)2 ||
		(mmu.read8(STAT_ADDRESS) >> 6 & 1) && lycEqualLy;

	if (curStat && !oldStat)
	{

		if (lycEqualLy)
		{
			std::cout << "requesting lyc = ly" << "\n";
		}
		mmu.requestInterrupt(STAT);
	}

	oldStat = curStat;
}

void PPU::setMode(MODE mode)
{
	ppuMode = mode;

	// set ppu mode in STAT register (bit 0 and 1)
	uint8_t stat = mmu.read8(STAT_ADDRESS);

	stat = (stat & ~3) | (int)mode;

	mmu.write8(STAT_ADDRESS, stat);
}

void PPU::tick()
{

	scanlineCycles++;
	
	if (ppuMode == VBLANK_1)
	{
		if (getLY() == 153)
		{
			
			setMode(OAM_SCAN_2);
			
			setLY(0);
		}
		windowLineCounter = 0;
		wyEqualLyThisFrame = false;
		wyEqualLy = false;
	}

	// Check for LYC == LY interrupt
	lycEqualLy = checkLycEqualLy() ? 1 : 0;
	setSTATCoincidenceFlag(lycEqualLy);
	
	// ----------------------------- OAM SCAN ---------------------------------------------------
	if (scanlineCycles <= 80 && ppuMode != VBLANK_1)
	{	

		if (scanlineCycles == 1)
		{
			setMode(OAM_SCAN_2);
		}

		// oam scan --> check a new oam entry every 2 t-cycles
		if (scanlineCycles % 2 == 0)
		{
			uint8_t currentLY = getLY();
			std::vector<uint8_t> oamByteBuffer;
			// each sprite is 4 bytes --> read a sprite from oam
			for (int i = 0; i < 4; i++)
			{
				oamByteBuffer.push_back(mmu.read8(OAM_START_ADDRESS + (oamScanCounter * 4 + i)));
			}

			// sprite data
			uint8_t yPos = oamByteBuffer[0];
			uint8_t xPos = oamByteBuffer[1];
			uint8_t tileNum = oamByteBuffer[2];
			uint8_t flags = oamByteBuffer[3];

			bool tallMode = spriteTallMode();

			uint8_t spriteHeight = tallMode ? 16 : 8;

			// Check conditions and add sprite to spriteBuffer if all conditions pass:

			// x pos greater than 0:
			
			if (xPos > 0    &&    (currentLY + 16) >= yPos    &&   (currentLY + 16) < (yPos + spriteHeight)    &&    spritesBuffer.size() < 10)
			{
				Sprite sprite;
				SpriteFlags spriteFlags;

				sprite.yPos = yPos;
				sprite.xPos = xPos;
				sprite.tileNum = tileNum;
				
				spriteFlags.prio = (flags>> 7) & 1;
				spriteFlags.yFlip = (flags >> 6) & 1;
				spriteFlags.xFlip = (flags >> 5) & 1;
				spriteFlags.palette = (flags >> 4) & 1;

				sprite.flags = spriteFlags;
				
				spritesBuffer.push_back(sprite);
			}

			oamScanCounter++;
		}
	}
	else
	{
		spritesBuffer.clear();
		oamScanCounter = 0;
	}


	if (!wyEqualLyThisFrame)
	{
		wyEqualLyThisFrame = getLY() == getWY();
	}

	// ----------------------------- DRAWING ---------------------------------------------------
	if (scanlineCycles > 80 && !exittedDrawingMode && ppuMode != VBLANK_1)
	{
		if (scanlineCycles == 81)
		{
			setMode(DRAWING_3);
		}

		// disable first fetch condition so that we can start discarding scx%8 pixels.
		if (scanlineCycles > 86)
		{
			firstBgFetch = false;
		}

		if (drawingState == BG_FETCH_TILE_NUM)
		{
			bgFetchTileNumCycles++;

			// if not fetching window, fetch background
			if (!fetchingWindow && bgFetchTileNumCycles >= 2)
			{
				uint16_t backgroundMapStart = bgTileMapSelect() ? 0x9C00 : 0x9800;

				uint16_t xOffset = ((((fetcherXPositionCounter + getSCX())) / 8) & 0x1F) & 0x3FF;

				uint16_t yOffset = (32 * (((getLY() + getSCY()) & 0xFF) / 8)) & 0x3FF;

				currentBgTileNumber = mmu.read8(backgroundMapStart + xOffset + yOffset);

				drawingState = BG_FETCH_TILE_LOW;
				bgFetchTileNumCycles = 0;
			}	
			else if (fetchingWindow && bgFetchTileNumCycles >= 2)
			{
				uint16_t backgroundMapStart = windowTileMapSelect() ? 0x9C00 : 0x9800;

				uint16_t offset = (fetcherXPositionCounter / 8) + (32 * (windowLineCounter / 8));

				currentBgTileNumber = mmu.read8(backgroundMapStart + offset);

				drawingState = BG_FETCH_TILE_LOW;
				bgFetchTileNumCycles = 0;
			}
		}
		else if (drawingState == BG_FETCH_TILE_LOW)
		{
			bgFetchTileLowCycles++;

			if (bgFetchTileLowCycles >= 2)
			{
				uint16_t baseAddress = tileDataSelect() ? 0x8000 : 0x9000;

				// remember --> 2 bits per pixel --> 2 bytes = 8 pixels --> the byte fetched here combined with the byte fetched in the next step will
				// yield 8 pixels;
			
				if (baseAddress == 0x8000)
				{
					bgFetchFirstByteAddress = baseAddress + (currentBgTileNumber * 16);
				}
				else
				{
					bgFetchFirstByteAddress = baseAddress + ((int8_t)currentBgTileNumber * 16);
				}

				uint8_t offset;

				if (!fetchingWindow)
				{
					offset = 2 * ((getLY() + getSCY()) % 8);
				}
				else
				{
					offset = 2 * (windowLineCounter % 8);
				}
				

				bgFetchFirstByteAddress += offset;


				bgFetchFirstByte = mmu.read8(bgFetchFirstByteAddress);

				drawingState = BG_FETCH_TILE_HIGH;
				bgFetchTileLowCycles = 0;
			}

		}
		else if (drawingState == BG_FETCH_TILE_HIGH)
		{
			bgFetchTileHighCycles++;

			if (bgFetchTileHighCycles >= 2)
			{
				// the first time this happens in a scanline the status is fully reset

				uint16_t bgFetchSecondByteAddress = bgFetchFirstByteAddress + 1;

				bgFetchSecondByte = mmu.read8(bgFetchSecondByteAddress);
				

				for (int i = 0; i < 8; i++)
				{
					Pixel pixel;

					uint8_t firstColorNumBit = (bgFetchSecondByte >> (7 - i)) & 1;
					uint8_t secondColorNumBit = (bgFetchFirstByte >> (7 - i)) & 1;

					pixel.colorNum = (firstColorNumBit) << 1 | secondColorNumBit;

					pixel.palette = BGP;
					
					bgFetchBuffer.push(pixel);
				}

				// If this is the first fetch, keep these pixels stored in the fetch buffer and discard scx%8 of them. Fetch
				// again at the same fetcherXPositionCounter and discard 8 more pixels. After that, run normally.
				if (firstBgFetch)
				{
					drawingState = BG_FETCH_TILE_NUM;
					
				}
				else
				{

					drawingState = BG_PUSH_TO_FIFO;
				}

				bgFetchTileHighCycles = 0;


			}

		}
		else if (drawingState == BG_PUSH_TO_FIFO )
		{

			bgPushToFifoCycles++;

			if (bgPushToFifoCycles >= 2)
			{
				if (bgPixelFIFO.empty())
				{
					

					for (int i = 0; i < 8; i++)
					{
					
						bgPixelFIFO.push(bgFetchBuffer.front());
						bgFetchBuffer.pop();

						fetcherXPositionCounter++;
					}

					bgPushToFifoCycles = 0;
					drawingState = BG_FETCH_TILE_NUM;
					
				}



			}
			// Don't forget to leave the BG_PUSH_TO_FIFO state
		}
		

		if (discardPixels && !firstBgFetch)
		{
			if (calculateDiscardedPixels)
			{
				numOfPixelsDiscarded = (getSCX() % 8);
				pixelsToBeDiscarded = numOfPixelsDiscarded;
				calculateDiscardedPixels = false;
			}


			if (pixelsToBeDiscarded > 0)
			{
				bgFetchBuffer.pop();

				pixelsToBeDiscarded--;
			}
			else
			{
				discardPixels = false;
			}
		}

		// --------------------------- PUSH PIXELS TO LCD -----------------------------------------
		if (!bgPixelFIFO.empty())
		{	
			if(!discardPixels)
			{
				Pixel pixel = bgPixelFIFO.front();

				bgPixelFIFO.pop();

				uint8_t pixelColor;
				
			

				// only push pixels after LX > 8 (after (scx%8) + 8 pixels where discarded). 
				if (LX >= 8)
				{
					if (bgAndWindowEnabled())
					{
						pixelColor = getPixelColor(pixel.palette, pixel.colorNum);

					}
					else
					{
						// background color number is 0 if background and window are disabled
						pixelColor = getPixelColor(pixel.palette, 0x00);
					}

					LCD[((160 * 144) - ((getLY() * 160)  + (160 - (LX - 8))))] = pixelColor;
				}

				LX++;

				if (fetchingWindow)
				{
					windowPixelWasDrawn = true;
				}


				if ((isWindowDisplayEnabled() && wyEqualLyThisFrame && ((LX + 8) >= (getWX() - 7))))
				{
					fetchingWindow = true;
					
					if (resetForWindowFetch)
					{
						fetcherXPositionCounter = 0;
						drawingState = BG_FETCH_TILE_NUM;

						for (int i = 0; i < bgPixelFIFO.size(); i++)
						{
							bgPixelFIFO.pop();
						}

						resetForWindowFetch = false;
					}
				}
				else 
				{
					fetchingWindow = false;
					resetForWindowFetch = true;
				}
			}

			// DRAWING MODE ENDED
			if (LX == 160 + 8)
			{
				fetcherXPositionCounter = 0;
				fetchingWindow = false;
				resetForWindowFetch = true;
				bgFetchTileNumCycles = 0;
				bgFetchTileLowCycles = 0;
				bgFetchTileHighCycles = 0;
				bgPushToFifoCycles = 0;
				pixelsToBeDiscarded = 0;
				spFetchTileNumCycles = 0;
				spFetchTileLowCycles = 0;
				spFetchTileHighCycles = 0;
				spPushToFifoCycles = 0;
				drawingState = BG_FETCH_TILE_NUM;
				numOfPixelsDiscarded = 0;

				for (int i = 0; i < bgPixelFIFO.size(); i++)
				{
					bgPixelFIFO.pop();
				}

				while(!bgFetchBuffer.empty())
				{
					
					bgFetchBuffer.pop();
					
				}

				LX = 0;
				firstBgFetch = true;
				discardPixels = true;
				exittedDrawingMode = true;
				calculateDiscardedPixels = true;
			}
		}
	}

	// ---------------------------------------- HBLANK --------------------------------------------------

	if (exittedDrawingMode && scanlineCycles < 456 && ppuMode != VBLANK_1)
	{
		if (firstHBlankCycle)
		{
			setMode(HBLANK_0);
			firstHBlankCycle = false;
		}
		
		// do nothing for hblank duration 
	}

	if (scanlineCycles >= 456)
	{
		setLY(getLY() + 1);
		if (windowPixelWasDrawn)
		{
			windowLineCounter++;
			windowPixelWasDrawn = false;
		}

		exittedDrawingMode = false;
		firstHBlankCycle = true;
		scanlineCycles = 0;
	}

	if (getLY() == 144)
	{
		setMode(VBLANK_1);
		mmu.requestInterrupt(VBLANK);
	}
	
	
	statInterruptCheck();
}



bool PPU::isDisplayEnabled()
{
	return (mmu.read8(LCDC_ADDRESS) >> 7) & 1;
}

bool PPU::windowTileMapSelect()
{
	return (mmu.read8(LCDC_ADDRESS) >> 6) & 1;
}

bool PPU::isWindowDisplayEnabled()
{
	return (mmu.read8(LCDC_ADDRESS) >> 5) & 1;
}

bool PPU::tileDataSelect()
{
	return (mmu.read8(LCDC_ADDRESS) >> 4) & 1;
}

bool PPU::bgTileMapSelect()
{
	return (mmu.read8(LCDC_ADDRESS) >> 3) & 1;
}

bool PPU::spriteTallMode()
{

	return (mmu.read8(LCDC_ADDRESS) >> 2) & 1;
}

bool PPU::isSpriteEnabled()
{
	return (mmu.read8(LCDC_ADDRESS) >> 1) & 1;
}

bool PPU::bgAndWindowEnabled()
{
	return mmu.read8(LCDC_ADDRESS) & 1;
}

uint8_t PPU::getSCY()
{
	return mmu.read8(0xFF42);
}

uint8_t PPU::getSCX()
{
	return mmu.read8(0xFF43);
}

uint8_t PPU::getLY()
{
	return mmu.read8(LY_ADDRESS);
}

uint8_t PPU::getLYC()
{
	return mmu.read8(LYC_ADDRESS);
}

void PPU::setLY(uint8_t value)
{
	mmu.write8(LY_ADDRESS, value);
}

uint8_t PPU::getPixelColor(PALETTE p, uint8_t colorID)
{
	if (p == BGP)
	{
		uint8_t bgp = mmu.read8(0xFF47);

		uint8_t grayShade = (bgp >> (2 * colorID)) & 3;
		
		return colors[grayShade];
	}
	else if (p == OBP0)
	{
		uint8_t bgp = mmu.read8(0xFF48);

		uint8_t grayShade = (bgp >> (2 * colorID)) & 3;

		return colors[grayShade];
	}
	else if (p == OBP1)
	{
		uint8_t bgp = mmu.read8(0xFF49);

		uint8_t grayShade = (bgp >> (2 * colorID)) & 3;

		return colors[grayShade];
	}
}

uint8_t PPU::getWY()
{
	return mmu.read8(0xFF4A);
}

uint8_t PPU::getWX()
{
	return mmu.read8(0xFF4B);
}

bool PPU::checkLycEqualLy()
{
	return getLY() == getLYC();
}

void PPU::setSTATCoincidenceFlag(bool b)
{
	uint8_t stat = mmu.read8(STAT_ADDRESS);
	
	stat = (stat & ~(1 << 2)) | (b << 2);
}