#include "ppu.h"

#define OAM_START_ADDRESS 0xFE00
#define LY_ADDRESS 0xFF44
#define LCDC_ADDRESS 0xFF40
#define STAT_ADDRESS 0xFF41


PPU::PPU(MMU& mmu) : mmu(mmu) {};


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

	// ----------------------------- OAM SCAN ---------------------------------------------------
	if (scanlineCycles <= 80)
	{	
		if (scanlineCycles == 1)
		{
			setMode(OAM_SCAN_2);
		}

		statInterruptCheck();

		// oam scan --> check a new oam entry every 2 t-cycles
		if (scanlineCycles % 2 == 0)
		{
			uint8_t currentLY = mmu.read8(LY_ADDRESS);
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

			bool tallMode = ((mmu.read8(LCDC_ADDRESS) >> 2) & 1) ? 1 : 0;

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
		oamScanCounter = 0;
	}
	
	// ----------------------------- DRAWING ---------------------------------------------------
	if (scanlineCycles > 80)
	{
		if (scanlineCycles == 81)
		{
			setMode(DRAWING_3);
		}


		statInterruptCheck();
	}

	if (scanlineCycles == 456)
	{
		scanlineCycles = 0;
	}

}