#include "mmu.h"
#include <cinttypes>
#include <iostream>

uint8_t MMU::read8(uint16_t address)
{

	// put if def right here and it will return before the rest of the if statements if the thing is defined
	//return testArray[address];

	// joypad not implemented yet so return 0xFF
	if (address == 0xFF00)
	{
		return 0xFF;
	}

	if (address <= 0x7FFF) // --> 16 kib rom
	{
		return rom[address - 0x0000]; // --> have to subtract starting memory position of array 
	}
	else if (address >= 0x8000 && address <= 0x9FFF)
	{
		return vRam[address - 0x8000];
	}
	else if (address >= 0xA000 && address <= 0XBFFF)
	{
		return eRam[address - 0xA000];
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
	if (address == 0xFF01)
	{
		std::cout <<  (char)value;
	}

	//testArray[address] = value;
	//if (address <= 0x7FFF) // --> 16 kib rom
	//{
	//	std::cout << "attempting to write to rom"// --> have to subtract starting memory position of array 
	//}



	if (address >= 0x8000 && address <= 0x9FFF)
	{
		vRam[address - 0x8000] = value;
	}
	else if (address >= 0xA000 && address <= 0XBFFF)
	{
		eRam[address - 0xA000] = value;
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



void MMU::writeToRom(uint16_t index, uint8_t value)
{

	// if testing use testArray
	rom[index] = value;
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

