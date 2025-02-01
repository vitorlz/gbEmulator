#pragma once 

#include "cpu.h"
#include "mmu.h"
#include "ppu.h"

#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>

class GameBoy
{
public:
	GameBoy();

	void readRom(const std::string path);
	
	uint8_t fetch();
	void decodeAndExecute(uint8_t opcode);

	void handleInterrupts();
	bool isCPUHalted();

	std::vector<uint8_t> fullrom;

	MMU mmu;
	PPU ppu;
	CPU cpu;	
};