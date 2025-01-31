#pragma once 

#include <memory>
#include "mmu.h"
#include "ppu.h"

#define B 0
#define C 1
#define D 2
#define E 3
#define H 4
#define L 5
#define F 6
#define A 7

// cpu is going to tick the other systems.

class CPU
{
public:

	// THE DEFAULT VALUES ARE THE ONES SUPPOSED TO BE SET BY THE BOOT ROM

	CPU(MMU& mmu, PPU& ppu);

	uint8_t regs[8]{0xFF, 0x13, 0x00, 0xC1, 0x84, 0x03, 0x00, 0x01};

	void setAF(uint16_t value);
	void setBC(uint16_t value);
	void setDE(uint16_t value);
	void setHL(uint16_t value);

	uint16_t getAF();
	uint16_t getBC();
	uint16_t getDE();
	uint16_t getHL();

	void setFlagZ(bool value);
	void setFlagN(bool value);
	void setFlagH(bool value);
	void setFlagC(bool value);

	bool getFlagZ();
	bool getFlagN();
	bool getFlagH();
	bool getFlagC();

	uint16_t SP = 0xFFFE; // the stack resides towards the end of the gb's memory. It grows AWAY from the end of the memory, so the "TOP" of the
	uint16_t PC = 0x0100; // stack is actually in the stack's lowest memory address. That is why we decrement the stack pointer by 1 when pushing a byte
							// and increment when popping a byte.
	
	bool IME = 0;
	void AddCycle(); // --> increment tCycles by 4 and tick systems 4x.

	
	unsigned int tCycles;

	uint8_t fetch8(); // --> same as read but reads using PC and increases it
	uint16_t fetch16();// --> same as read16 but reads using PC twice and increases it twice

	// these are wrappers to mmu functions
	uint8_t read8(uint16_t address);
	void write8(uint16_t address, uint8_t value);

	uint16_t read16(uint16_t address); 
	void write16(uint16_t address, uint16_t value); 
						
	void decodeAndExecute(uint8_t opcode);
private:
	MMU& mmu;
	PPU& ppu;
};