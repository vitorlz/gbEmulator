#include <iostream>
#include <glad/glad.h>
#include "ppu.h"

#define OAM_START_ADDRESS 0xFE00
#define LY_ADDRESS 0xFF44
#define LYC_ADDRESS 0xFF45
#define LCDC_ADDRESS 0xFF40
#define STAT_ADDRESS 0xFF41

PPU::PPU(MMU& mmu) 
	: mmu(mmu), window(window)
{
	setLY(0);
	oamByteBuffer.reserve(4);
	spPixelFIFO.reserve(8);
	spritesBuffer.reserve(10);
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
		mmu.requestInterrupt(STAT);
	}

	oldStat = curStat;
}

void PPU::resetScanline()
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
	bgDrawingState = BG_FETCH_TILE_NUM;
	spDrawingState = SP_FETCH_TILE_NUM;
	numOfPixelsDiscarded = 0;
	scanlineDrawnPixels = 0;

	while (!bgPixelFIFO.empty())
	{
		bgPixelFIFO.pop();
	}

	while (!bgFetchBuffer.empty())
	{
		bgFetchBuffer.pop();
	}

	spPixelFIFO.clear();
	spritesBuffer.clear();

	lycEqualLy = checkLycEqualLy();
	statInterruptCheck();

	LX = 0;
	firstBgFetch = true;
	discardPixels = true;
	exittedDrawingMode = true;
	calculateDiscardedPixels = true;
	spriteFetchEnabled = false;
}

void PPU::resetFrame()
{
	setLY(0);
	resetScanline();

	windowPixelWasDrawn = false;
	exittedDrawingMode = false;
	firstHBlankCycle = true;
	scanlineCycles = 0;
	windowLineCounter = 0;
	wyEqualLyThisFrame = false;
	wyEqualLy = false;
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
	if (!isDisplayEnabled() && !displayDisabled && displayWasEnabledBefore)
	{
		disableDisplay();
	}

	if (displayDisabled && isDisplayEnabled())
	{
		displayDisabled = false;
		setMode(OAM_SCAN_2);
	}

	if (displayDisabled)
	{
		return;
	}

	// display has to be enabled at least once so that it can be disabled
	if (!displayWasEnabledBefore)
	{
		displayWasEnabledBefore = isDisplayEnabled();
	}

	scanlineCycles++;

	if (ppuMode == VBLANK_1)
	{
		checkIfVblankEnded();
	}
	
	oamScan();
	drawingMode();
	hBlankMode();

	if (scanlineCycles >= 456)
	{
		prepareNextScanline();
	}

	if (getLY() == 144 && ppuMode != VBLANK_1)
	{
		enterVBlank();
	}
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
		uint8_t obp0 = mmu.read8(0xFF48);

		uint8_t grayShade = (obp0 >> (2 * colorID)) & 3;

		return colors[grayShade];
	}
	else if (p == OBP1)
	{
		uint8_t obp1 = mmu.read8(0xFF49);

		uint8_t grayShade = (obp1 >> (2 * colorID)) & 3;

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
	
	stat = (stat & 0b11111011) | (b << 2);

	mmu.write8(STAT_ADDRESS, stat);
}

void PPU::draw()
{
	glBindTexture(GL_TEXTURE_2D, displayTexture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 160, 144, GL_RED, GL_UNSIGNED_BYTE, &LCD[0]);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void PPU::oamScan()
{
	if (scanlineCycles <= 80 && ppuMode != VBLANK_1)
	{
		if (scanlineCycles == 1)
		{
			setMode(OAM_SCAN_2);
			statInterruptCheck();
		}

		// oam scan --> check a new oam entry every 2 t-cycles
		if (scanlineCycles % 2 == 0)
		{
			uint8_t currentLY = getLY();

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

			// if hit a sprite, add to buffer
			if (xPos >= 0 && (currentLY + 16) >= yPos && (currentLY + 16) < (yPos + spriteHeight) && spritesBuffer.size() < 10)
			{
				Sprite sprite;
				SpriteFlags spriteFlags;

				sprite.yPos = yPos;
				sprite.xPos = xPos;
				sprite.tileNum = tileNum;
				sprite.height = spriteHeight;

				spriteFlags.prio = (flags >> 7) & 1;
				spriteFlags.yFlip = (flags >> 6) & 1;
				spriteFlags.xFlip = (flags >> 5) & 1;
				spriteFlags.palette = (flags >> 4) & 1;

				sprite.flags = spriteFlags;

				spritesBuffer.push_back(sprite);
			}

			oamByteBuffer.clear();
			oamScanCounter++;
		}
	}
	else
	{
		oamScanCounter = 0;
	}
}

void PPU::drawingMode()
{
	if (scanlineCycles > 80 && !exittedDrawingMode && ppuMode != VBLANK_1)
	{
		if (!wyEqualLyThisFrame)
		{
			wyEqualLyThisFrame = getLY() == getWY();
		}

		if (scanlineCycles == 81)
		{
			setMode(DRAWING_3);
			statInterruptCheck();
		}

		windowFetchCheck();

		// disable first fetch condition so that we can start discarding scx%8 pixels.
		if (scanlineCycles > 86 && firstBgFetch)
		{
			firstBgFetch = false;
		}

		if (bgDrawingState == BG_FETCH_TILE_NUM)
		{
			fetchBgTileNum();
		}
		else if (bgDrawingState == BG_FETCH_TILE_LOW)
		{
			fetchBgTileLow();
		}
		else if (bgDrawingState == BG_FETCH_TILE_HIGH)
		{
			fetchBgTileHigh();
		}
		else if (bgDrawingState == BG_PUSH_TO_FIFO)
		{
			bgPushToFIFO();
		}

		checkIfReachedSprite();

		if (spriteFetchEnabled)
		{
			// cancel sprite fetch if sprite rendering is disabled mid fetch
			checkSpriteFetchCancel();

			if (spDrawingState == SP_FETCH_TILE_NUM && spriteFetchEnabled)
			{
				fetchSpTileNum();
			}
			else if (spDrawingState == SP_FETCH_TILE_LOW && spriteFetchEnabled)
			{
				fetchSpTileLow();
			}
			else if (spDrawingState == SP_FETCH_TILE_HIGH_AND_PUSH && spriteFetchEnabled)
			{
				fetchSpTileHighAndPush();
			}
		}

		// discard scx%8 pixels from first fetch
		firstBgFetchDiscard();

		if (!bgPixelFIFO.empty())
		{
			pushToLCD();
			// DRAWING MODE ENDED --> RESET STATE
			if (LX == 168)
			{
				resetScanline();
			}
		}
	}
}

void PPU::hBlankMode()
{
	if (exittedDrawingMode && scanlineCycles < 456 && ppuMode != VBLANK_1)
	{
		if (firstHBlankCycle)
		{
			setMode(HBLANK_0);
			statInterruptCheck();
			firstHBlankCycle = false;
		}
	}
}

void PPU::prepareNextScanline()
{
	setLY(getLY() + 1);
	lycEqualLy = checkLycEqualLy();
	setSTATCoincidenceFlag(lycEqualLy);
	statInterruptCheck();

	if (windowPixelWasDrawn)
	{
		windowLineCounter++;
		windowPixelWasDrawn = false;
	}
	exittedDrawingMode = false;
	firstHBlankCycle = true;
	scanlineCycles = 0;
}

void PPU::enterVBlank()
{
	draw();
	setMode(VBLANK_1);
	mmu.requestInterrupt(VBLANK);
	windowLineCounter = 0;
	wyEqualLyThisFrame = false;
	wyEqualLy = false;
}

void PPU::disableDisplay()
{
	resetFrame();
	setMode(HBLANK_0);
	statInterruptCheck();
	setSTATCoincidenceFlag(false);
	displayDisabled = true;
}

void PPU::checkIfVblankEnded()
{
	if (getLY() == 154)
	{
		ppuMode = OAM_SCAN_2;
		setLY(0);
		lycEqualLy = checkLycEqualLy();
		setSTATCoincidenceFlag(lycEqualLy);
		statInterruptCheck();
	}
}

void PPU::windowFetchCheck()
{
	if (isWindowDisplayEnabled())
	{
		if (wyEqualLyThisFrame && (scanlineDrawnPixels >= getWX() - 7) && !fetchingWindow)
		{
			fetchingWindow = true;

			bgFetchTileNumCycles = 0;
			bgFetchTileLowCycles = 0;
			bgFetchTileHighCycles = 0;
			bgPushToFifoCycles = 0;
			fetcherXPositionCounter = 0;
			bgDrawingState = BG_FETCH_TILE_NUM;
			spriteFetchEnabled = false;
			spDrawingState = SP_FETCH_TILE_NUM;

			while (!bgPixelFIFO.empty())
			{
				bgPixelFIFO.pop();
			}

			while (!bgFetchBuffer.empty())
			{
				bgFetchBuffer.pop();
			}
			spPixelFIFO.clear();
		}
	}
	else
	{
		fetchingWindow = false;
	}
}

void PPU::fetchBgTileNum()
{
	bgFetchTileNumCycles++;

	// if not fetching window, fetch background
	if (!fetchingWindow && bgFetchTileNumCycles >= 2)
	{
		uint16_t backgroundMapStart = bgTileMapSelect() ? 0x9C00 : 0x9800;

		uint16_t xOffset = ((((fetcherXPositionCounter + getSCX())) / 8) & 0x1F) & 0x3FF;

		uint16_t yOffset = (32 * (((getLY() + getSCY()) & 0xFF) / 8)) & 0x3FF;

		currentBgTileNumber = mmu.read8(backgroundMapStart + xOffset + yOffset);

		bgDrawingState = BG_FETCH_TILE_LOW;
		bgFetchTileNumCycles = 0;
	}
	else if (fetchingWindow && bgFetchTileNumCycles >= 2)
	{
		uint16_t backgroundMapStart = windowTileMapSelect() ? 0x9C00 : 0x9800;

		uint16_t offset = (((fetcherXPositionCounter / 8) + (32 * (windowLineCounter / 8))));

		currentBgTileNumber = mmu.read8(backgroundMapStart + offset);

		bgDrawingState = BG_FETCH_TILE_LOW;
		bgFetchTileNumCycles = 0;
	}
}

void PPU::fetchBgTileLow()
{
	bgFetchTileLowCycles++;

	if (bgFetchTileLowCycles >= 2)
	{
		uint16_t baseAddress = tileDataSelect() ? 0x8000 : 0x9000;

		// 2 bits per pixel --> 2 bytes = 8 pixels --> the byte fetched here combined with the byte fetched in the next step will
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

		bgDrawingState = BG_FETCH_TILE_HIGH;
		bgFetchTileLowCycles = 0;
	}
}

void PPU::fetchBgTileHigh()
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
			bgDrawingState = BG_FETCH_TILE_NUM;
		}
		else
		{
			bgDrawingState = BG_PUSH_TO_FIFO;
		}

		bgFetchTileHighCycles = 0;
	}
}

void PPU::bgPushToFIFO()
{
	bgPushToFifoCycles++;

	if (bgPushToFifoCycles >= 2)
	{
		if (bgPixelFIFO.empty() && !bgFetchBuffer.empty() && !spriteFetchEnabled)
		{
			for (int i = 0; i < 8; i++)
			{
				bgPixelFIFO.push(bgFetchBuffer.front());
				bgFetchBuffer.pop();

			}
			fetcherXPositionCounter += 8;
			bgPushToFifoCycles = 0;
			bgDrawingState = BG_FETCH_TILE_NUM;
		}
	}
}

void PPU::checkIfReachedSprite()
{
	// CHECK IF REACHED A SPRITE 
	if (!spriteFetchEnabled && isSpriteEnabled())
	{
		for (int i = 0; i < spritesBuffer.size(); i++)
		{
			Sprite sprite = spritesBuffer[i];
			if (sprite.xPos - 8 == scanlineDrawnPixels)
			{
				spriteBeingFetched = sprite;
				spriteFetchEnabled = true;

				spritesBuffer.erase(spritesBuffer.begin() + i);

				break;
			}
		}
	}
}

void PPU::checkSpriteFetchCancel()
{
	if (!isSpriteEnabled())
	{
		spDrawingState = SP_FETCH_TILE_NUM;
		spriteFetchEnabled = false;
		spFetchTileNumCycles = 0;
		spFetchTileLowCycles = 0;
		spFetchTileHighCycles = 0;
		spPushToFifoCycles = 0;
	}
}

void PPU::fetchSpTileNum()
{
	spFetchTileNumCycles++;

	if (spFetchTileNumCycles >= 2)
	{
		currentSpTileNumber = spriteBeingFetched.tileNum;

		spFetchTileNumCycles = 0;
		spDrawingState = SP_FETCH_TILE_LOW;
	}
}

void PPU::fetchSpTileLow()
{
	spFetchTileLowCycles++;

	if (spFetchTileLowCycles >= 2)
	{
		int offset = 2 * (((getLY() + 16) - spriteBeingFetched.yPos) % spriteBeingFetched.height);

		if (spriteTallMode())
		{
			currentSpTileNumber = currentSpTileNumber & 0xFE;
		}

		if (spriteBeingFetched.flags.yFlip)
		{
			offset = (((spriteTallMode() == 1 ? 16 : 8) - 1) * 2) - offset;
		}

		spFetchFirstByteAddress = 0x8000 + (currentSpTileNumber * 16) + offset;

		spFetchFirstByte = mmu.read8(spFetchFirstByteAddress);

		spFetchTileLowCycles = 0;
		spDrawingState = SP_FETCH_TILE_HIGH_AND_PUSH;
	}
}

void PPU::fetchSpTileHighAndPush()
{
	spFetchTileHighCycles++;

	if (spFetchTileHighCycles >= 2)
	{
		uint16_t spFetchSecondByteAddress = spFetchFirstByteAddress + 1;
		spFetchSecondByte = mmu.read8(spFetchSecondByteAddress);

		int spFIFOSizeBeforePush = spPixelFIFO.size();
		int firstTransparentIndex = 0;
		bool firstTransparentOverlap = true;

		for (int i = 0; i < 8; i++)
		{
			Pixel pixel;

			unsigned int bitOffset;

			if (spriteBeingFetched.flags.xFlip)
			{
				bitOffset = i;
			}
			else
			{
				bitOffset = 7 - i;
			}

			uint8_t firstColorNumBit = (spFetchSecondByte >> bitOffset) & 1;
			uint8_t secondColorNumBit = (spFetchFirstByte >> bitOffset) & 1;

			pixel.xPos = spriteBeingFetched.xPos + i;
			pixel.colorNum = (firstColorNumBit) << 1 | secondColorNumBit;
			pixel.palette = spriteBeingFetched.flags.palette ? OBP1 : OBP0;
			pixel.backgroundPrio = spriteBeingFetched.flags.prio;

			// replace transparent pixels in the FIFO that overlap with this current pixel
			bool transparentPixelOverlap = false;
			if (spFIFOSizeBeforePush != 0)
			{
				for (int j = 0; j < spFIFOSizeBeforePush; j++)
				{
					Pixel fifoPixel = spPixelFIFO[j];
					if (((fifoPixel.xPos == pixel.xPos) && (fifoPixel.colorNum == 0 || (fifoPixel.backgroundPrio > pixel.backgroundPrio))))
					{
						if (pixel.colorNum != 0)
						{
							spPixelFIFO[j] = pixel;
							transparentPixelOverlap = true;
						}
					}
				}
			}

			if ((spPixelFIFO.size() < 8 && i >= spFIFOSizeBeforePush) && !transparentPixelOverlap)
			{
				spPixelFIFO.push_back(pixel);
			}
		}

		spFetchTileHighCycles = 0;
		spDrawingState = SP_FETCH_TILE_NUM;
		spriteFetchEnabled = false;

		for (int i = 0; i < spritesBuffer.size(); i++)
		{
			Sprite sprite = spritesBuffer[i];
			if (sprite.xPos - 8 == scanlineDrawnPixels)
			{
				spriteBeingFetched = sprite;
				spriteFetchEnabled = true;

				// remove sprite from buffer
				spritesBuffer.erase(spritesBuffer.begin() + i);

				break;
			}
		}
	}
}

void PPU::firstBgFetchDiscard()
{
	if (discardPixels && !firstBgFetch)
	{
		if (calculateDiscardedPixels)
		{
			if (fetchingWindow)
			{
				numOfPixelsDiscarded = 0;
			}
			else
			{
				numOfPixelsDiscarded = (getSCX() % 8);
			}

			pixelsToBeDiscarded = numOfPixelsDiscarded;
			calculateDiscardedPixels = false;
		}

		if (pixelsToBeDiscarded > 0)
		{
			bgFetchBuffer.pop();

			if (!spPixelFIFO.empty())
			{
				spPixelFIFO.erase(spPixelFIFO.begin());
			}
			pixelsToBeDiscarded--;
		}
		else
		{
			discardPixels = false;
		}
	}
}

void PPU::pushToLCD()
{
	if (!discardPixels && !spriteFetchEnabled)
	{
		Pixel bgPixel = bgPixelFIFO.front();

		bgPixelFIFO.pop();

		uint8_t pixelColor;

		// only push pixels after LX > 8 (after (scx%8) + 8 pixels where discarded). 
		if (LX >= 8)
		{
			if (bgAndWindowEnabled())
			{
				pixelColor = getPixelColor(bgPixel.palette, bgPixel.colorNum);
			}
			else
			{
				// background color number is 0 if background and window are disabled
				pixelColor = getPixelColor(bgPixel.palette, 0x00);
				bgPixel.colorNum = 0;
			}

			if (isSpriteEnabled() && !spPixelFIFO.empty())
			{
				Pixel spPixel = spPixelFIFO.front();

				spPixelFIFO.erase(spPixelFIFO.begin() + 0);

				if ((!(spPixel.backgroundPrio == 1 && (bgPixel.colorNum != 0))
					|| (spPixel.backgroundPrio == 0))
					&& spPixel.colorNum != 0)
				{
					pixelColor = getPixelColor(spPixel.palette, spPixel.colorNum);
				}
			}

			LCD[((160 * 144) - ((getLY() * 160) + (160 - (LX - 8))))] = pixelColor;
			scanlineDrawnPixels++;
		}

		if (fetchingWindow)
		{
			windowPixelWasDrawn = true;
		}

		LX++;
	}
}