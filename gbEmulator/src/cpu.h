#pragma once 

#include <memory>
#include "mmu.h"
#include "ppu.h"

class CPU
{
public:
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
	
	uint16_t DIV = 0; 
	uint16_t divCycles = 0; 	// 16 bit counter, only put upper 8 bits in FF04, increment every 256 t-cycle, writing to FF04 sets it to 0.
	bool lastANDResult = 0;
	bool timaReloadPending = 0;

	// can the corresponding interrupt handlers.
	void handleInterrupts();

	bool HALT = false;
	bool IME = 0;
	bool updateIME = 0;
	
	void AddCycle(); // --> increment tCycles by 4 and tick systems 4x.

	unsigned int tCycles;

	void handleRTC();
	void handleTimers();
	void handleDMATransfer();

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