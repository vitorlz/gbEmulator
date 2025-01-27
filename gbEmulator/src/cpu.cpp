#include "cpu.h"
#include "iostream"

CPU::CPU(MMU& mmu, PPU& ppu) 
	: mmu(mmu),
	  ppu(ppu)
{};

void CPU::setAF(uint16_t value)
{
	regs[A] = value >> 8;
	regs[F] = value | 0xFF;
}

void CPU::setBC(uint16_t value)
{
	regs[B] = value >> 8;
	regs[C] = value | 0xFF;
}

void CPU::setDE(uint16_t value)
{
	regs[D] = value >> 8;
	regs[E] = value | 0xFF;
}

void CPU::setHL(uint16_t value)
{
	regs[H] = value >> 8;
	regs[L] = value | 0xFF;
}

uint16_t CPU::getAF()
{
	return (regs[A] << 8) | regs[F];
}
uint16_t CPU::getBC()
{
	return (regs[B] << 8) | regs[C];
}
uint16_t CPU::getDE()
{
	return (regs[D] << 8) | regs[E];
}

uint16_t CPU::getHL()
{
	return (regs[H] << 8) | regs[L];
}

void CPU::setFlagZ(bool value)
{
	regs[F] = (regs[F] & ~(1 << 7)) | (value << 7);
}
void CPU::setFlagN(bool value)
{
	regs[F] = (regs[F] & ~(1 << 6)) | (value << 6);
}
void CPU::setFlagH(bool value)
{
	regs[F] = (regs[F] & ~(1 << 5)) | (value << 5);
}
void CPU::setFlagC(bool value)
{
	regs[F] = (regs[F] & ~(1 << 4)) | (value << 4);
}

bool CPU::getFlagZ()
{
	return regs[F] >> 7;
}
bool CPU::getFlagN()
{
	return regs[F] >> 6 & 0x01;
}
bool CPU::getFlagH()
{
	return regs[F] >> 5 & 0x01;
}
bool CPU::getFlagC()
{
	return regs[F] >> 4 & 0x01;
}

uint8_t CPU::read8(uint16_t address)
{
	return mmu.read8(address);
	AddCycle();
}

void CPU::write8(uint16_t address, uint8_t value)
{
	mmu.write8(address, value);
	AddCycle();
}


uint16_t CPU::read16(uint16_t address)
{
	uint16_t lsb = read8(address);
	uint16_t msb = read8(address + 1);

	return (msb << 8) | lsb;
}

void CPU::write16(uint16_t address, uint16_t value)
{
	uint16_t lsb = value & 0xFF;
	uint16_t msb = value >> 8;

	write8(address, lsb);
	write8(address + 1, msb);
}

void CPU::AddCycle()
{
	tCycles += 4;
	// TICK PPU 4X --> ppu.tick(4);
}

uint8_t CPU::fetch8()
{
	return mmu.read8(PC++);
}

uint16_t CPU::fetch16()
{
	uint16_t lsb = read8(PC++);
	uint16_t msb = read8(PC++);

	return (msb << 8) | lsb;
}

void CPU::decodeAndExecute(uint8_t opcode)
{
	switch (opcode)
	{
		// LD r8, r8
		case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x47: 
		case 0x48: case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4D: case 0x4F:
		case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x57:
		case 0x58: case 0x59: case 0x5A: case 0x5B: case 0x5C: case 0x5D: case 0x5F:
		case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x67:
		case 0x68: case 0x69: case 0x6A: case 0x6B: case 0x6C: case 0x6D: case 0x6F:
		case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7F:
		{
			int8_t dest = (opcode >> 3) & 7;
			int8_t src = opcode & 7;
			regs[dest] = regs[src];
			
			break;
		}

		// LD r8, (HL)
		case 0x46: case 0x4E: case 0x56: case 0x5E: case 0x66: case 0x6E: case 0x7E:
		{
			int8_t dest = (opcode >> 3) & 7;
			int8_t src = read8(getHL());
			regs[dest] = src;
			
			/*std::cout << "ld hl" << "\n";
			std::cout << "address stored in HL " << std::hex << getHL() << "\n";
			std::cout << "dest: " << (int)dest << "\n";
			std::cout << "src: " << std::hex << (int)src << "\n";*/

			break;
		}
	
	}


}