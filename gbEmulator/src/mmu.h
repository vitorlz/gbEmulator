#pragma once 

#include <cinttypes>

#define B 0
#define C 1
#define D 2
#define E 3
#define H 4
#define L 5
#define F 6
#define A 7

#define DIV_ADDRESS 0xFF04
#define TIMA_ADDRESS 0xFF05
#define TMA_ADDRESS 0xFF06
#define TAC_ADDRESS 0xFF07
#define IF_ADDRESS 0xFF0F
#define IE_ADDRESS 0xFFFF

enum Interrupt
{
	VBLANK = 0,
	STAT,
	TIMER,
	SERIAL,
	JOYPAD
};

class MMU
{
public:

	uint8_t testArray[0x10000];

	uint8_t bootrom[0x0100]; // --> this overlaps the rom when the program starts up and then handles control over to the actual 
	// cartridge rom at pc = 0x100. At the beginning when pc is at 0x00 - 0x100 range read boot rom array. Once pc reaches 0x100 subsequent memory
	// reads the cartridge array. Could probably be done with just a bool ex: if(pc == 0x0100 && !bootromDone) bootromDone = true
	
	uint8_t rom[0x8000]; // 16 KiB ROM bank 00 and 16 KiB ROM Bank 01–NN 
	uint8_t vRam[0x2000]; // 8 KiB Video RAM (VRAM)
	uint8_t eRam[0x2000]; // 8 KiB External RAM
	uint8_t wRam[0x2000]; // 8 KiB Work RAM(WRAM)

	// 0xE000 - 0xFDFF --> echo ram just read from wram --> wRam[address - 0xE000]

	uint8_t oam[0x00A0];  // Object attribute memory (OAM)
	uint8_t ioRegs[0x0080]; // I/O Registers
	uint8_t hRam[0x007F]; // High RAM (HRAM)
	uint8_t ie[0x0001]; // --> FFFF interrupt enable register (IE)

	// cpu will fetch the instruction 


	// whenever read using pc increase pc --> read8(PC++)
	uint8_t read8(uint16_t address);
	void write8(uint16_t address, uint8_t value);
	void writeToRom(uint16_t index, uint8_t value);


	// sets the corresponding bit in IF
	void requestInterrupt(Interrupt type);

	// unset the corresponding bit in IF
	void cancelInterrupt(Interrupt type);

	// return true if corresponding bit in IF is set
	bool isInterruptRequested(Interrupt type);
	// return true if the corresponding bit in IE is set
	bool isInterruptEnabled(Interrupt type);
};