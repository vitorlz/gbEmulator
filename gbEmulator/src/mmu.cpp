#include <cinttypes>
#include <iostream>
#include "mmu.h"

uint8_t MMU::read8(uint16_t address)
{

	// put if def right here and it will return before the rest of the if statements if the thing is defined
	// joypad not implemented yet so return 0xFF
	/*if (address == 0xFF00)
	{
		return 0xFF;
	}*/

	//return testArray[address];

	if (address <= 0x3FFF) // --> 16 kib rom
	{
		if (mbc == MBC0)
		{
			return fullrom[address];
		}
		else if (mbc == MBC1)
		{

			if (modeFlag == 0)
			{
				return fullrom[address];
			}
			else
			{
				return fullrom[0x4000 * getMBC1ZeroBankNumber() + address];
			}
			
		}
		 // --> have to subtract starting memory position of array 
	}
	else if (address >= 0x4000 && address <= 0x7FFF)
	{
		if (mbc == MBC0)
		{
			return fullrom[address];
		}
		else if (mbc == MBC1)
		{

			//std::cout << "mbc1 high bank number: " << std::dec << (int)getMBC1HighBankNumber() << "\n";
			return fullrom[0x4000 * getMBC1HighBankNumber() + (address - 0x4000)];
		}
	}
	else if (address >= 0x8000 && address <= 0x9FFF)
	{

		return vRam[address - 0x8000];
	}
	else if (address >= 0xA000 && address <= 0xBFFF)
	{
		if (mbc == MBC0)
		{
			return eRam[address - 0xA000];
		}
		else if (mbc == MBC1)
		{
			if (sRamEnabled)
			{
				if (sRamSize == 0x800 || sRamSize == 0x2000)
				{
					return eRam[(address - 0xA000) % sRamSize];
				}
				else if (sRamSize == 0x8000)
				{

					if (modeFlag == 1)
					{
						return eRam[0x2000 * ramBankNumber + (address - 0xA000)];
					}
					else
					{
						return eRam[address - 0xA000];
					}
					
				}
			}
			else
			{
				return 0xFF;
			}
			
		}
		
	}
	else if (address >= 0xC000 && address <= 0xDFFF)
	{
		return wRam[address - 0xC000];
	}
	else if (address >= 0xE000 && address <= 0xFDFF) // --> echo ram (mirror of a part of wRam)
	{
		return wRam[address - 0xE000];
	}
	else if (address >= 0xFE00 && address <= 0xFE9F)
	{
		return oam[address - 0xFE00];
	}
	else if (address >= 0xFEA0 && address <= 0xFEFF) // --> not usable --> return 0xFF on read -->  do nothhing on write
	{
		return 0xFF;
	}
	else if (address >= 0xFF00 && address <= 0xFF7F)
	{
		return ioRegs[address - 0xFF00];
	}
	else if (address >= 0xFF80 && address <= 0xFFFE)
	{
		return hRam[address - 0xFF80];
	}
	else if (address == 0xFFFF) // --> Interrupt Enable register (IE)
	{
		return ie[address - 0xFFFF];
	}
}

void MMU::write8(uint16_t address, uint8_t value)
{

	/*if (address == 0xFF01)
	{
		std::cout <<  (char)value;
	}*/

	if (address == 0xFF46)
	{
		dmaTransferRequested = true;

		dmaSource = dmaSource | ((uint16_t)value << 8);
	}

	// fs there is a dma transfer happening and the cpu tries to access  maybe block writes and reads to (address < 0xFF80 || address >  0xFFFE) during dma
	// usually games handle that and only do dma transfers during vblank
	
	//testArray[address] = value;
	//if (address <= 0x7FFF) // --> 16 kib rom
	//{
	//	std::cout << "attempting to write to rom"; // --> have to subtract starting memory position of array 
	//}

	if (mbc == MBC1)
	{
		if (address <= 0x1FFF)
		{

			if ((value & 15) == 10)
			{
				
				sRamEnabled = true;
			}
			else
			{
				sRamEnabled = false;
			}
		}

		if (address >= 0x2000 && address <= 0x3FFF)
		{
			// romBankNumber is a 5 bit register, it is never 0. If we try to write 0 to it we should write 1 instead.
			// upper bits are ignored.
			if ((value & 31) == 0)
			{
				romBankNumber = 1;
			}
			else
			{
				if (romNumOfBanks == 128 || romNumOfBanks == 64 || romNumOfBanks == 32)
				{
					romBankNumber = value & 31;
				}
				else if (romNumOfBanks == 16)
				{
					romBankNumber = value & 15;
				}
				else if (romNumOfBanks == 8)
				{
					romBankNumber = value & 7;
				}
				else if (romNumOfBanks == 4)
				{
					romBankNumber = value & 3;
				}
				else if (romNumOfBanks == 2)
				{
					romBankNumber = value & 1;
				}
			}
			
		}

		if (address >= 0x4000 && address <= 0x5FFF)
		{
			ramBankNumber = value & 3;

			//std::cout << "rambanknumber: " << std::dec << (int)ramBankNumber << "\n";
		}

		if (address >= 0x6000 && address <= 0x7FFF)
		{
			//std::cout << "toggling bank mode" << "\n";
			modeFlag = value & 1;
			//std::cout << "mode flag: " << modeFlag << "\n";
		}
	}


	if (address >= 0x8000 && address <= 0x9FFF)
	{
		vRam[address - 0x8000] = value;
	}
	else if (address >= 0xA000 && address <= 0XBFFF)
	{
		if (mbc == MBC0)
		{
			eRam[address - 0xA000] = value;
		}
		else if (mbc == MBC1)
		{
			if (sRamEnabled)
			{
				if (sRamSize == 0x800 || sRamSize == 0x2000)
				{
					eRam[(address - 0xA000) % sRamSize] = value;
				}
				else if (sRamSize == 0x8000)
				{

					if (modeFlag == 1)
					{
						//std::cout << "rambanknumber when fetching from eram: " << std::dec << (int)ramBankNumber << "\n";
						eRam[0x2000 * ramBankNumber + (address - 0xA000)] = value;
					}
					else
					{
						eRam[address - 0xA000] = value;
					}

				}
			}
		}
		
	}
	else if (address >= 0xC000 && address <= 0xDFFF)
	{
		wRam[address - 0xC000] = value;
	}
	else if (address >= 0xE000 && address <= 0xFDFF) // --> echo ram (mirror of a part of wRam)
	{
		wRam[address - 0xE000] = value;
	}
	else if (address >= 0xFE00 && address <= 0xFE9F)
	{					
		oam[address - 0xFE00] = value;				
	}												
	else if (address >= 0xFF00 && address <= 0xFF7F)
	{
		ioRegs[address - 0xFF00] = value;
	}
	else if (address >= 0xFF80 && address <= 0xFFFE)
	{
		hRam[address - 0xFF80] = value;
	}
	else if (address == 0xFFFF) // --> Interrupt Enable register (IE)
	{
		ie[address - 0xFFFF] = value;
	}
}

void MMU::writeToRomMemory(uint16_t index, uint8_t value)
{
	//testArray[index] = value;
	// if testing use testArray
	if (index <= 0x3FFF) // --> 16 kib rom
	{
		rom0[index] = value; // --> have to subtract starting memory position of array 
	}
	else if (index >= 0x4000 && index <= 0x7FFF)
	{
		romBanks[index - 0x4000] = value;
	}
}

void MMU::requestInterrupt(Interrupt type)
{
	uint8_t IF = read8(IF_ADDRESS);

	IF = IF | (1 << type);

	write8(IF_ADDRESS, IF);
}

void MMU::cancelInterrupt(Interrupt type)
{
	uint8_t IF = read8(IF_ADDRESS);

	IF = IF & ~(1 << type);

	write8(IF_ADDRESS, IF);
}

bool MMU::isInterruptRequested(Interrupt type)
{
	uint8_t IF = read8(IF_ADDRESS);

	return (IF >> type) & 1;
}

bool MMU::isInterruptEnabled(Interrupt type)
{
	uint8_t IE = read8(IE_ADDRESS);

	return (IE >> type) & 1;
}

void MMU::dmaTransfer(unsigned int count)
{
	uint8_t value = read8(dmaSource + (count - 1));
	write8(0xFE00 + (count - 1), value);

	//std::cout << "count " << count << "\n";
}


uint8_t MMU::getMBC1HighBankNumber()
{
	if (romNumOfBanks <= 32)
	{
		if (romNumOfBanks == 32)
		{
			highBankNumber = romBankNumber & 31;
		}
		else if (romNumOfBanks == 16)
		{
			highBankNumber = romBankNumber & 15;
		}
		else if (romNumOfBanks == 8)
		{
			highBankNumber = romBankNumber & 7;
		}
		else if (romNumOfBanks == 4)
		{
			highBankNumber = romBankNumber & 3;
		}
		else if (romNumOfBanks == 2)
		{
			highBankNumber = romBankNumber & 1;
		}
	}
	else if (romNumOfBanks == 64)
	{
		highBankNumber = (romBankNumber & 31) | ((ramBankNumber & 1) << 5);
	}
	else if (romNumOfBanks == 128)
	{
		highBankNumber = (romBankNumber & 31) | ((ramBankNumber & 3) << 5);
	}

	return highBankNumber;
}

uint8_t MMU::getMBC1ZeroBankNumber()
{
	if (romNumOfBanks <= 32)
	{
		zeroBankNumber = 0;
	}
	else if (romNumOfBanks == 64)
	{
		zeroBankNumber = zeroBankNumber | ((ramBankNumber & 1) << 5);
	}
	else if (romNumOfBanks == 128)
	{
		zeroBankNumber = zeroBankNumber | ((ramBankNumber & 3) << 5);
	}

	return zeroBankNumber;
}

