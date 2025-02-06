#include "gb.h"
#include "mmu.h"

GameBoy::GameBoy()
	:	ppu(mmu),
		cpu(mmu, ppu)
{}

void GameBoy::readRom(const std::string rom)
{
    std::ifstream is(rom.c_str(), std::ifstream::binary);
    if (is) 
    {
        is.seekg(0, is.end);
        int length = is.tellg();
        is.seekg(0, is.beg);

        char* buffer = new char[length];

        is.read(buffer, length);

        if (is)
            std::cout << "rom read successfully\n";
        else
            std::cout << "error: only " << is.gcount() << " could be read\n";

        for (int i = 0; i < length; i++)
        {
            fullrom.push_back(buffer[i]);            
        }

        // put first 32kib of rom in mmu rom for now since we are using mbc0
        for (uint16_t i = 0; i < 0x8000; i++)
        {
            //mmu.rom[i] = fullrom[i];

            mmu.writeToRomMemory(i, fullrom[i]);
        }
        
        is.close();
        delete[] buffer;
    }
}

uint8_t GameBoy::fetch()
{
    return cpu.fetch8();
}

void GameBoy::decodeAndExecute(uint8_t opcode)
{
    cpu.decodeAndExecute(opcode);
}

void GameBoy::handleInterrupts()
{
    cpu.handleInterrupts();
}

bool GameBoy::isCPUHalted()
{
    return cpu.HALT;
}