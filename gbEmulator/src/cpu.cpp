#include "cpu.h"
#include "iostream"

CPU::CPU(MMU& mmu, PPU& ppu) 
	: mmu(mmu),
	  ppu(ppu)
{};

void CPU::setAF(uint16_t value)
{
	regs[A] = value >> 8;
	regs[F] = value & 0xFF;
}

void CPU::setBC(uint16_t value)
{
	regs[B] = value >> 8;
	regs[C] = value & 0xFF;
}

void CPU::setDE(uint16_t value)
{
	regs[D] = value >> 8;
	regs[E] = value & 0xFF;
}

void CPU::setHL(uint16_t value)
{
	regs[H] = value >> 8;
	regs[L] = value & 0xFF;
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

	/*std::cout << "SET FLAG VALUE: " << (int)value << "\n";*/
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
	uint8_t val = mmu.read8(address);
	AddCycle();

	return val;
}

void CPU::requestInterrupt(Interrupt type)
{
	uint8_t IF = mmu.read8(IF_ADDRESS);

	IF = IF | (1 << type);

	mmu.write8(IF_ADDRESS, IF);
}

void CPU::cancelInterrupt(Interrupt type)
{
	uint8_t IF = mmu.read8(IF_ADDRESS);

	IF = IF & ~(1 << type);

	mmu.write8(IF_ADDRESS, IF);
}

bool CPU::isInterruptRequested(Interrupt type)
{
	uint8_t IF = mmu.read8(IF_ADDRESS);

	return (IF >> type) & 1;
}

bool CPU::isInterruptEnabled(Interrupt type)
{
	uint8_t IE = mmu.read8(IE_ADDRESS);

	return (IE >> type) & 1;
}


void CPU::handleInterrupts()
{
	if (HALT)
	{
		uint8_t IE = mmu.read8(IE_ADDRESS);
		uint8_t IF = mmu.read8(IF_ADDRESS);

		if ((IE & IF) != 0)
		{
			//std::cout << "halt deactivated" << "\n";
			HALT = false;
		}
		
		//std::cout << "IE: " << std::dec << (int)IE << "\n";
		//std::cout << "IF: " << std::dec << (int)IF << "\n";
		
	}

	if (HALT)
	{
		return;
	}

	if (!IME)
	{
		return;
	}

	
	// two m-cycles while nothing happens
	AddCycle();
	AddCycle();
	
	if (isInterruptRequested(TIMER) && isInterruptEnabled(TIMER))
	{

		cancelInterrupt(TIMER);
		IME = 0;

		write8(--SP, PC >> 8);
		write8(--SP, PC & 0xFF);

		PC = 0x50;

		AddCycle();
	}
	
}


void CPU::write8(uint16_t address, uint8_t value)
{
	
	if (address == DIV_ADDRESS)
	{
		value = 0x00;
		DIV = 0;
	}

	if (address == TIMA_ADDRESS)
	{
		//std::cout << "wrote to TIMA" << "\n";
		timaReloadPending = false;

		cancelInterrupt(TIMER);
	}

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
	
	// TICK PPU 4X --> ppu.tick(4);
	if (timaReloadPending)
	{
		uint8_t TMA = mmu.read8(TMA_ADDRESS);
		mmu.write8(TIMA_ADDRESS, TMA);

		requestInterrupt(TIMER);

		timaReloadPending = false;
	}
	
	for (int i = 0; i < 4; i++)
	{
		tCycles++;
		DIV++;

		uint8_t bitPosition;
		uint8_t TAC = mmu.read8(TAC_ADDRESS);
		uint8_t tacClockSelect = TAC & 3;

		switch (tacClockSelect)
		{
			case 0x00:
			{
				bitPosition = 9;
				break;
			}
			case 0x01:
			{
				bitPosition = 3;
				break;
			}
			case 0x02:
			{
				bitPosition = 5;
				break;
			}
			case 0x03:
			{
				bitPosition = 7;
				break;
			}
		}

		bool selectedDIVBit = (DIV >> bitPosition) & 1;

		bool tacTimerEnableBit = (TAC >> 2) & 1;

		//std::cout << std::dec << (int)TAC << "\n";

		bool currentANDResult = selectedDIVBit && tacTimerEnableBit;

		if (lastANDResult == 1 && currentANDResult == 0)
		{
			
			uint8_t TIMA = mmu.read8(TIMA_ADDRESS);

			//std::cout << "TIMA: " << std::dec << (int)TIMA << "\n";

			if (TIMA == 0xFF)
			{
				//std::cout << "TIME RELOAD PENDING" << "\n";
				timaReloadPending = true;
				mmu.write8(TIMA_ADDRESS, 0x00);

			}
			else if(!timaReloadPending)
			{
				mmu.write8(TIMA_ADDRESS, TIMA + 1);
			}
		}
		
		lastANDResult = currentANDResult;

		divCycles++;
		
		if (divCycles >= 256)
		{
			mmu.write8(DIV_ADDRESS, DIV >> 8);

			//std::cout << "Full DIV: " << std::dec << (int)(DIV) << "\n";
			//std::cout << "DIV upper 8 bits value: " << std::dec << (int)(DIV >> 8) << "\n";

			divCycles = 0;
		}
	}
}

uint8_t CPU::fetch8()
{
	return read8(PC++);
}

uint16_t CPU::fetch16()
{
	uint16_t lsb = read8(PC++);
	uint16_t msb = read8(PC++);

	return (msb << 8) | lsb;
}

void CPU::decodeAndExecute(uint8_t opcode)
{
	// NOT IMPLEMENTED: STOP HALT

	if (updateIME)
	{
		IME = 1;

		updateIME = 0;
	}

	switch (opcode)
	{

		case 0x00:
		{
			// NOP --> does nothing 
			break;
		}

		// STOP
		case 0x10:
		{
			std::cout << "STOP called" << "\n";

			break;
		}

		// JR i8
		case 0x18:
		{
			int8_t i8 = (int8_t)fetch8();
			PC += i8;

			AddCycle();

			break;
		}

		// JR cc, i8
		case 0x20: case 0x28: case 0x30: case 0x38:
		{
			uint8_t cc = (opcode >> 3) & 3;
			switch (cc)
			{
				case 0x00:
				{

					//std::cout << "RUNNNING JR NZ, i8" << "\n";
					
					// JR NZ, i8
					int8_t i8 = (int8_t)fetch8();

					//std::cout << std::dec << "jp offset: " << (int)i8 << "\n";

					if (!getFlagZ())
					{

						//std::cout << std::dec << "FLAG WHEN JUMPING: " << getFlagZ() << "\n";
						PC += i8;

						AddCycle();
					}

					break;
				}
				case 0x01:
				{

					// JR Z, i8
					int8_t i8 = (int8_t)fetch8();

					if (getFlagZ())
					{
						PC += i8;

						AddCycle();
					}

					break;
				}
				case 0x02:
				{
					// JR NC, i8
					int8_t i8 = (int8_t)fetch8();

					if (!getFlagC())
					{
						PC += i8;

						AddCycle();
					}

					break;
				}
				case 0x03:
				{
					// JR C, i8
					int8_t i8 = (int8_t)fetch8();

					if (getFlagC())
					{
						PC += i8;

						AddCycle();
					}

					break;
				}
				
			}

			break;
		}

		// LD r16, u16
		case 0x01: case 0x11: case 0x21: case 0x31:
		{
			uint8_t r16 = (opcode >> 4) & 3;
			switch (r16)
			{
				case 0x00:
				{
					uint16_t u16 = fetch16();
					setBC(u16);

					break;
				}

				case 0x01:
				{
					uint16_t u16 = fetch16();
					setDE(u16);

					break;
				}

				case 0x02:
				{
					uint16_t u16 = fetch16();
					setHL(u16);

					break;
				}

				case 0x03:
				{
					uint16_t u16 = fetch16();
					SP = u16;

					break;
				}

			}

			break;
		}



		// LD (r16), A
		case 0x02: case 0x12: case 0x22: case 0x32:
		{
			uint8_t r16 = (opcode >> 4) & 3;

			switch (r16)
			{
				case 0:
				{
					write8(getBC(), regs[A]);

					break;
				}

				case 1:
				{
					write8(getDE(), regs[A]);

					break;
				}

				case 2:
				{
					write8(getHL(), regs[A]);

					setHL(getHL() + 1);

					break;
				}

				case 3:
				{
					write8(getHL(), regs[A]);

					setHL(getHL() - 1);

					break;
				}

			}

			break;
		}

		// LD (u16), SP
		case 0x08:
		{
			uint16_t u16 = fetch16();
			write16(u16, SP);

			break;
		}


		// LD A, (r16)
		case 0x0A: case 0x1A: case 0x2A: case 0x3A:
		{
			uint8_t r16 = (opcode >> 4) & 3;

			switch (r16)
			{
				case 0:
				{
					regs[A] = read8(getBC());

					break;
				}

				case 1:
				{
					regs[A] = read8(getDE());

					break;
				}

				case 2:
				{
					regs[A] = read8(getHL());

					setHL(getHL() + 1);

					break;
				}

				case 3:
				{
					regs[A] = read8(getHL());

					setHL(getHL() - 1);

					break;
				}

			}

			break;
		}


		// LD r8, u8
		case 0x06: case 0x0E: case 0x16: case 0x1E: case 0x26: case 0x2E: case 0x3E:
		{
			
			int8_t dest = (opcode >> 3) & 7;
			regs[dest] = fetch8();

			break;
		}

		// LD (HL), u8
		case 0x36:
		{
			uint8_t u8 = fetch8();
			write8(getHL(), u8);
			
			/*std::cout << "writing to the address in hl" << "\n";*/

			break;
		}


		
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

		// LD (HL), r8
		case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x77:
		{
			int8_t src = opcode & 7;
			write8(getHL(), regs[src]);

			break;
		}

		// LD r8, (HL)
		case 0x46: case 0x4E: case 0x56: case 0x5E: case 0x66: case 0x6E: case 0x7E:
		{
			int8_t dest = (opcode >> 3) & 7;
			int8_t src = read8(getHL());
			regs[dest] = src;
			
			/*std::cout << "PC: " << std::hex << PC << "\n";
			std::cout << std::dec << (int)opcode << "\n";

			std::cout << "ld hl" << "\n";
			std::cout << "address stored in HL " << std::hex << getHL() << "\n";
			std::cout << "dest: " << (int)dest << "\n";
			std::cout << "src: " << std::hex << (int)src << "\n";*/

			break;
		}

		// ADD HL, r16
		case 0x09: case 0x19: case 0x29: case 0x39:
		{ 
			uint8_t r16 = (opcode >> 4) & 3;
			switch (r16)
			{
				case 0:
				{
					setFlagN(0);
					setFlagH(((getHL() & 0xFFF) + (getBC() & 0xFFF)) > 0xFFF);
					setFlagC(((uint32_t)getHL() + (uint32_t)getBC()) > 0xFFFF);

					setHL(getHL() + getBC());
					
					AddCycle();

					break;
				}

				case 1:
				{
					setFlagN(0);
					setFlagH(((getHL() & 0xFFF) + (getDE() & 0xFFF)) > 0xFFF);
					setFlagC(((uint32_t)getHL() + (uint32_t)getDE()) > 0xFFFF);

					setHL(getHL() + getDE());

					AddCycle();

					break;
				}

				case 2:
				{
					setFlagN(0);
					setFlagH(((getHL() & 0xFFF) + (getHL() & 0xFFF)) > 0xFFF);
					setFlagC(((uint32_t)getHL() + (uint32_t)getHL()) > 0xFFFF);

					setHL(getHL() + getHL());

					AddCycle();

					break;
				}

				case 3:
				{
					setFlagN(0);
					setFlagH(((getHL() & 0xFFF) + (SP & 0xFFF)) > 0xFFF);
					setFlagC(((uint32_t)getHL() + (uint32_t)SP) > 0xFFFF);

					setHL(getHL() + SP);

					AddCycle();

					break;
				}
			}

			break;
		}

		// INC r16
		case 0x03: case 0x13: case 0x23: case 0x33:
		{
			uint8_t r16 = (opcode >> 4) & 3;
			switch (r16)
			{
				case 0:
				{
					setBC(getBC() + 1);
					
					AddCycle();
					break;
				}

				case 1:
				{
					setDE(getDE() + 1);

					AddCycle();
					break;
				}

				case 2:
				{
					setHL(getHL() + 1);

					AddCycle();
					break;
				}

				case 3:
				{
					SP += 1;

					AddCycle();
					break;
				}
			}
			break;
		}

		// DEC r16
		case 0x0B: case 0x1B: case 0x2B: case 0x3B:
		{
			uint8_t r16 = (opcode >> 4) & 3;
			switch (r16)
			{
				case 0:
				{
					setBC(getBC() - 1);

					AddCycle();
					break;
				}

				case 1:
				{
					setDE(getDE() - 1);

					AddCycle();
					break;
				}

				case 2:
				{
					setHL(getHL() - 1);

					AddCycle();
					break;
				}

				case 3:
				{
					SP -= 1;

					AddCycle();
					break;
				}
			}
			break;
		}

		// INC r8
		case 0x04: case 0x0C: case 0x14: case 0x1C: case 0x24: case 0x2C: case 0x3C:
		{
			
			uint8_t r8 = (opcode >> 3) & 7;

			//std::cout << "INC R8 r8: " << std::dec << (int)r8 << "\n";
			//std::cout << "INC R8 regs[r8] before: " << std::dec << (int)regs[r8] << "\n";
			

			setFlagN(0);
			setFlagH(((regs[r8] & 0x0F) + 1) > 0x0F);
			setFlagZ((uint8_t)(regs[r8] + 1) == 0);

			regs[r8] = regs[r8] + 1;

			//std::cout << "INC R8 regs[r8] after: " << std::dec << (int)regs[r8] << "\n";
			//std::cout << "Z FLAG: " << std::dec << (int)getFlagZ() << "\n";
			
			break;
		}

		// INC (HL)
		case 0x34:
		{
			uint8_t value = read8(getHL());

			setFlagN(0);
			setFlagH(((value & 0x0F) + 1) > 0x0F);
			setFlagZ((uint8_t)(value + 1) == 0);

			write8(getHL(), value + 1);

			break;
		}

		// DEC r8
		case 0x05: case 0x0D: case 0x15: case 0x1D: case 0x25: case 0x2D: case 0x3D:
		{

			uint8_t r8 = (opcode >> 3) & 7;

			setFlagN(1);
			setFlagH(((regs[r8] & 0x0F) == 0));
			setFlagZ((uint8_t)(regs[r8] - 1) == 0);

			regs[r8] = regs[r8] - 1;

			break;
		}

		// DEC (HL)
		case 0x35:
		{
			uint8_t value = read8(getHL());

			setFlagN(1);
			setFlagH(((value & 0x0F) == 0));
			setFlagZ((uint8_t)(value - 1) == 0);

			write8(getHL(), value - 1);

			break;
		}

		// RLCA
		case 0x07:
		{
			uint8_t value = regs[A];

			setFlagC((value >> 7) & 1);
			setFlagZ(0);
			setFlagN(0);
			setFlagH(0);

			regs[A] = (value << 1) | getFlagC();

			break;
		}


		// RRCA
		case 0x0F:
		{
			uint8_t value = regs[A];

			setFlagC(value & 1);
			setFlagZ(0);
			setFlagN(0);
			setFlagH(0);

			regs[A] = (value >> 1) | (getFlagC() << 7);

			break;
		}

		// RLA
		case 0x17:
		{
			uint8_t value = regs[A];

			uint8_t carry = getFlagC();
			setFlagC((value >> 7) & 1);
			setFlagZ(0);
			setFlagN(0);
			setFlagH(0);

			regs[A] = (value << 1) | carry;

			break;
		}

		// RRA
		case 0x1F:
		{
			uint8_t value = regs[A];

			uint8_t carry = getFlagC();
			setFlagC(value & 1);
			setFlagZ(0);
			setFlagN(0);
			setFlagH(0);

			regs[A] = (value >> 1) | (carry << 7);

			break;
		}

		// CPL
		case 0x2F:
		{
			regs[A] = ~regs[A];

			setFlagN(1);
			setFlagH(1);

			break;
		}

		// SCF
		case 0x37:
		{
			setFlagC(1);
			setFlagH(0);
			setFlagN(0);

			break;
		}

		// CCF
		case 0x3F:
		{
			setFlagC(!getFlagC());
			setFlagH(0);
			setFlagN(0);

			break;
		}

		// ALU A, r8
		case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x87:
		case 0x88: case 0x89: case 0x8A: case 0x8B: case 0x8C: case 0x8D: case 0x8F:
		case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x97:
		case 0x98: case 0x99: case 0x9A: case 0x9B: case 0x9C: case 0x9D: case 0x9F:
		case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4: case 0xA5: case 0xA7:
		case 0xA8: case 0xA9: case 0xAA: case 0xAB: case 0xAC: case 0xAD: case 0xAF:
		case 0xB0: case 0xB1: case 0xB2: case 0xB3: case 0xB4: case 0xB5: case 0xB7:
		case 0xB8: case 0xB9: case 0xBA: case 0xBB: case 0xBC: case 0xBD: case 0xBF:
		{
			
			uint8_t operation = (opcode >> 3) & 7;
			uint8_t r8 = opcode & 7;

			switch (operation)
			{
				// ADD A, r8
				case 0x00:
				{
					setFlagZ((uint8_t)(regs[A] + regs[r8]) == 0);
					setFlagN(0);
					setFlagH(((regs[A] & 0x0F) + (regs[r8] & 0x0F)) > 0x0F);
					setFlagC(((uint16_t)regs[A] + (uint16_t)regs[r8]) > 0xFF);
					
					regs[A] = regs[A] + regs[r8];
					
					break;
				}

				// ADC A, r8
				case 0x01:
				{
					bool carry = ((uint16_t)regs[A] + (uint16_t)regs[r8] + (uint16_t)getFlagC()) > 0xFF;

					setFlagZ((uint8_t)(regs[A] + regs[r8] + getFlagC()) == 0);
					setFlagN(0);
					setFlagH(( (regs[A] & 0x0F) + (regs[r8] & 0x0F) + getFlagC() ) > 0x0F);
					
					regs[A] = regs[A] + regs[r8] + getFlagC();

					setFlagC(carry);

					break;
				}

				// SUB A, r8
				case 0x02:
				{
					setFlagZ((uint8_t)(regs[A] - regs[r8]) == 0);
					setFlagN(1);
					setFlagH((regs[A] & 0x0F) < (regs[r8] & 0x0F));
					setFlagC(regs[A] < regs[r8]);

					regs[A] = regs[A] - regs[r8];

					break;
				}

				// SBC A, r8
				case 0x03:
				{
					bool carry = regs[A] < ((uint16_t)regs[r8] + getFlagC());

					setFlagZ((uint8_t)(regs[A] - regs[r8] - getFlagC()) == 0);
					setFlagN(1);
					setFlagH( (regs[A] & 0x0F) < ((regs[r8] & 0x0F) + getFlagC()) );
					
					regs[A] = regs[A] - regs[r8] - getFlagC();

					setFlagC(carry);

					break;
				}

				// AND A, r8
				case 0x04:
				{
					setFlagZ((uint8_t)(regs[A] & regs[r8]) == 0);
					setFlagN(0);
					setFlagH(1);
					setFlagC(0);

					regs[A] = regs[A] & regs[r8];

					break;
				}

				// XOR A, r8
				case 0x05:
				{
					setFlagZ((uint8_t)(regs[A] ^ regs[r8]) == 0);
					setFlagN(0);
					setFlagH(0);
					setFlagC(0);

					regs[A] = regs[A] ^ regs[r8];

					break;
				}

				// OR A, r8
				case 0x06:
				{
					setFlagZ((uint8_t)(regs[A] | regs[r8]) == 0);
					setFlagN(0);
					setFlagH(0);
					setFlagC(0);

					regs[A] = regs[A] | regs[r8];

					break;
				}

				// CP A, r8
				case 0x07:
				{
					setFlagZ((uint8_t)(regs[A] - regs[r8]) == 0);
					setFlagN(1);
					setFlagH((regs[A] & 0x0F) < (regs[r8] & 0x0F));
					setFlagC(regs[A] < regs[r8]);

					break;
				}

			}

			break;
		}

		// ALU A, (HL)
		case 0x86: case 0x8E: case 0x96: case 0x9E: case 0xA6: case 0xAE: case 0xB6: case 0xBE:
		{
			uint8_t operation = (opcode >> 3) & 7;
			uint8_t val = read8(getHL());

			switch (operation)
			{
				// ADD A, (HL)
				case 0x00:
				{
					setFlagZ((uint8_t)(regs[A] + val) == 0);
					setFlagN(0);
					setFlagH(((regs[A] & 0x0F) + (val & 0x0F)) > 0x0F);
					setFlagC(((uint16_t)regs[A] + (uint16_t)val) > 0xFF);

					regs[A] = regs[A] + val;

					break;
				}

				// ADC A, (HL)
				case 0x01:
				{
					bool carry = ((uint16_t)regs[A] + (uint16_t)val + (uint16_t)getFlagC()) > 0xFF;

					setFlagZ((uint8_t)(regs[A] + val + getFlagC()) == 0);
					setFlagN(0);
					setFlagH(((regs[A] & 0x0F) + (val & 0x0F) + getFlagC()) > 0x0F);

					regs[A] = regs[A] + val + getFlagC();

					setFlagC(carry);

					break;
				}

				// SUB A, (HL)
				case 0x02:
				{
					setFlagZ((uint8_t)(regs[A] - val) == 0);
					setFlagN(1);
					setFlagH((regs[A] & 0x0F) < (val & 0x0F));
					setFlagC(regs[A] < val);

					regs[A] = regs[A] - val;

					break;
				}

				// SBC A, (HL)
				case 0x03:
				{
					bool carry = regs[A] < ((uint16_t)val + getFlagC());

					setFlagZ((uint8_t)(regs[A] - val - getFlagC()) == 0);
					setFlagN(1);
					setFlagH((regs[A] & 0x0F) < ((val & 0x0F) + getFlagC()));

					regs[A] = regs[A] - val - getFlagC();

					setFlagC(carry);

					break;
				}

				// AND A, (HL)
				case 0x04:
				{
					setFlagZ((uint8_t)(regs[A] & val) == 0);
					setFlagN(0);
					setFlagH(1);
					setFlagC(0);

					regs[A] = regs[A] & val;

					break;
				}

				// XOR A, (HL)
				case 0x05:
				{
					setFlagZ((uint8_t)(regs[A] ^ val) == 0);
					setFlagN(0);
					setFlagH(0);
					setFlagC(0);

					regs[A] = regs[A] ^ val;

					break;
				}

				// OR A, (HL)
				case 0x06:
				{
					setFlagZ((uint8_t)(regs[A] | val) == 0);
					setFlagN(0);
					setFlagH(0);
					setFlagC(0);

					regs[A] = regs[A] | val;

					break;
				}

				// CP A, (HL)
				case 0x07:
				{
					setFlagZ((uint8_t)(regs[A] - val) == 0);
					setFlagN(1);
					setFlagH((regs[A] & 0x0F) < (val & 0x0F));
					setFlagC(regs[A] < val);

					break;
				}

			}

			break;
		}

		// POP r16
		case 0xC1: case 0xD1: case 0xE1: case 0xF1:
		{
			uint8_t r16 = (opcode >> 4) & 3;
			
			switch (r16)
			{
				case 0x00:
				{
					regs[C] = read8(SP++);
					regs[B] = read8(SP++);
					
					break;
				}

				case 0x01:
				{
					regs[E] = read8(SP++);
					regs[D] = read8(SP++);

					break;
				}

				case 0x02:
				{
					regs[L] = read8(SP++);
					regs[H] = read8(SP++);

					break;
				}

				case 0x03:
				{
					regs[F] = read8(SP++) & (0xFF << 4);
					regs[A] = read8(SP++);

					break;
				}
			}

			break;
		}


		// PUSH r16
		case 0xC5: case 0xD5: case 0xE5: case 0xF5:
		{
			uint8_t r16 = (opcode >> 4) & 3;

			switch (r16)
			{
				case 0x00:
				{

					AddCycle();

					write8(--SP, regs[B]);
					write8(--SP, regs[C]);

					break;
				}

				case 0x01:
				{

					AddCycle();

					write8(--SP, regs[D]);
					write8(--SP, regs[E]);

					break;
				}

				case 0x02:
				{

					AddCycle();

					write8(--SP, regs[H]);
					write8(--SP, regs[L]);

					break;
				}

				case 0x03:
				{
					AddCycle();
				
					write8(--SP, regs[A]);
					write8(--SP, (regs[F] & (0xFF << 4)));

					break;
				}
			}

			break;
		}

		// JP cc, u16
		case 0xC2: case 0xCA: case 0xD2: case 0xDA:
		{
			uint8_t cc = (opcode >> 3) & 3;
			switch (cc)
			{
				// NZ
				case 0x00:
				{
					uint16_t address = fetch16();

					if (!getFlagZ())
					{
						PC = address;

						AddCycle();
					}
					

					break;
				}

				// Z
				case 0x01:
				{
					uint16_t address = fetch16();

					if (getFlagZ())
					{
						PC = address;

						AddCycle();
					}

					break;
				}

				// NC
				case 0x02:
				{
					uint16_t address = fetch16();

					if (!getFlagC())
					{
						PC = address;

						AddCycle();
					}

					break;
				}

				// C
				case 0x03:
				{
					uint16_t address = fetch16();

					if (getFlagC())
					{
						PC = address;

						AddCycle();
					}

					break;
				}
			}

			break;
		}

		// JP u16
		case 0xC3:
		{
			uint16_t address = fetch16();

			PC = address;
			AddCycle();

			break;
		}

		// JP HL
		case 0xE9:
		{
			PC = getHL();
			
			break;
		}
		
		// CALL cc, u16
		case 0xC4: case 0xCC: case 0xD4: case 0xDC:
		{
			uint8_t cc = (opcode >> 3) & 3;

			switch (cc)
			{
				// NZ
				case 0x00:
				{
					uint16_t address = fetch16();

					if (!getFlagZ())
					{

						write8(--SP, PC >> 8);
						write8(--SP, PC & 0xFF);

						PC = address;

						AddCycle();
					}

					break;	
				}

				// Z
				case 0x01:
				{
					uint16_t address = fetch16();

					if (getFlagZ())
					{

						write8(--SP, PC >> 8);
						write8(--SP, PC & 0xFF);

						PC = address;

						AddCycle();
					}


					break;
				}

				// NC
				case 0x02:
				{
					uint16_t address = fetch16();

					if (!getFlagC())
					{

						write8(--SP, PC >> 8);
						write8(--SP, PC & 0xFF);

						PC = address;

						AddCycle();
					}

					break;
				}

				// C
				case 0x03:
				{
					uint16_t address = fetch16();

					if (getFlagC())
					{

						write8(--SP, PC >> 8);
						write8(--SP, PC & 0xFF);

						PC = address;

						AddCycle();
					}

					break;
				}
			}
			break;
		}

		// CALL u16
		case 0xCD:
		{
			uint16_t address = fetch16();

			write8(--SP, PC >> 8);
			write8(--SP, PC & 0xFF);

			PC = address;

			AddCycle();
			
			break;
		}

		// RET cc

		case 0xC0: case 0xC8: case 0xD0: case 0xD8:
		{
			uint8_t cc = (opcode >> 3) & 3;
			
			switch (cc)
			{
				// NZ
				case 0x00:
				{
					AddCycle();
					if (!getFlagZ())
					{
						uint8_t lsb = read8(SP++);
						uint8_t msb = read8(SP++);

						PC = (msb << 8) | lsb;

						AddCycle();
					}
					break;
				}

				// Z
				case 0x01:
				{
					AddCycle();
					if (getFlagZ())
					{
						uint8_t lsb = read8(SP++);
						uint8_t msb = read8(SP++);

						PC = (msb << 8) | lsb;

						AddCycle();
					}
					break;
				}

				// NC
				case 0x02:
				{
					AddCycle();
					if (!getFlagC())
					{
						uint8_t lsb = read8(SP++);
						uint8_t msb = read8(SP++);

						PC = (msb << 8) | lsb;

						AddCycle();
					}
					break;
				}

				// C
				case 0x03:
				{
					AddCycle();
					if (getFlagC())
					{
						uint8_t lsb = read8(SP++);
						uint8_t msb = read8(SP++);

						PC = (msb << 8) | lsb;

						AddCycle();
					}
					break;
				}
			}

			break;
		}


		// RET
		case 0xC9:
		{
			/*std::cout << "RET" << "\n";*/
		
			uint8_t lsb = read8(SP++);
			uint8_t msb = read8(SP++);

			PC = (msb << 8) | lsb;

			AddCycle();
			
			break;
		}

		// RETI
		case 0xD9:
		{
			uint8_t lsb = read8(SP++);
			uint8_t msb = read8(SP++);

			PC = (msb << 8) | lsb;

			IME = 1;

			AddCycle();

			break;
		}

		// LD (FF00+u8),A
		case 0xE0:
		{
			uint8_t u8 = fetch8();

			uint16_t address = 0xFF00 | u8;

			write8(address, regs[A]);

			break;
		}

		// LD A,(FF00+u8)
		case 0xF0:
		{
			uint8_t u8 = fetch8();

			uint16_t address = 0xFF00 | u8;

			regs[A] = read8(address);

			break;
		}

		// LD (FF00 + C), A 
		case 0xE2:
		{
		
			uint16_t address = 0xFF00 | regs[C];
			
			write8(address, regs[A]);

			break;
		}


		// LD (u16), A
		case 0xEA:
		{

			uint16_t address = fetch16();

			write8(address, regs[A]);

			break;
		}

		// LD A, (FF00 + C)
		case 0xF2:
		{
			uint16_t address = 0xFF00 | regs[C];

			regs[A] = read8(address);

			break;
		}


		// LD A,(u16)
		case 0xFA:
		{

			uint16_t address = fetch16();

			regs[A] = read8(address);

			break;
		}


		// ADD SP, i8
		case 0xE8:
		{
			int8_t i8 = (int8_t)fetch8();

			setFlagZ(0);
			setFlagN(0);
			setFlagH(((SP & 0x0F) + (i8 & 0x0F)) > 0x0F);
			setFlagC(((SP & 0xFF) + (i8 & 0xFF)) > 0xFF);
			
			AddCycle();

			SP += i8;

			AddCycle();

			break;
		}

		// LD HL, SP+i8
		case 0xF8:
		{
			int8_t i8 = (int8_t)fetch8();

			setFlagZ(0);
			setFlagN(0);
			setFlagH(((SP & 0x0F) + (i8 & 0x0F)) > 0x0F);
			setFlagC(((SP & 0xFF) + (i8 & 0xFF)) > 0xFF);

			setHL(SP + i8);

			AddCycle();

			break;
		}

		// LD SP, HL
		case 0xF9:
		{
			SP = getHL();

			AddCycle();

			break;
		}

		// ALU A, u8
		case 0xC6: case 0xCE: case 0xD6: case 0xDE: case 0xE6: case 0xEE: case 0xF6: case 0xFE:
		{
			uint8_t operation = (opcode >> 3) & 7;
			uint8_t u8 = fetch8();

			switch (operation)
			{
				// ADD A, u8
				case 0x00:
				{
					setFlagZ((uint8_t)(regs[A] + u8) == 0);
					setFlagN(0);
					setFlagH(((regs[A] & 0x0F) + (u8 & 0x0F)) > 0x0F);
					setFlagC(((uint16_t)regs[A] + (uint16_t)u8) > 0xFF);

					regs[A] = regs[A] + u8;

					break;
				}

				// ADC A, u8
				case 0x01:
				{
					bool carry = ((uint16_t)regs[A] + (uint16_t)u8 + (uint16_t)getFlagC()) > 0xFF;

					setFlagZ((uint8_t)(regs[A] + u8 + getFlagC()) == 0);
					setFlagN(0);
					setFlagH(((regs[A] & 0x0F) + (u8 & 0x0F) + getFlagC()) > 0x0F);

					regs[A] = regs[A] + u8 + getFlagC();

					setFlagC(carry);

					break;
				}

				// SUB A, u8
				case 0x02:
				{
					setFlagZ((uint8_t)(regs[A] - u8) == 0);
					setFlagN(1);
					setFlagH((regs[A] & 0x0F) < (u8 & 0x0F));
					setFlagC(regs[A] < u8);

					regs[A] = regs[A] - u8;

					break;
				}

				// SBC A, u8
				case 0x03:
				{
					bool carry = regs[A] < ((uint16_t)u8 + getFlagC());

					setFlagZ((uint8_t)(regs[A] - u8 - getFlagC()) == 0);
					setFlagN(1);
					setFlagH((regs[A] & 0x0F) < ((u8 & 0x0F) + getFlagC()));

					regs[A] = regs[A] - u8 - getFlagC();

					setFlagC(carry);

					break;
				}

				// AND A, u8
				case 0x04:
				{
					setFlagZ((uint8_t)(regs[A] & u8) == 0);
					setFlagN(0);
					setFlagH(1);
					setFlagC(0);

					regs[A] = regs[A] & u8;

					break;
				}

				// XOR A, u8
				case 0x05:
				{
					setFlagZ((uint8_t)(regs[A] ^ u8) == 0);
					setFlagN(0);
					setFlagH(0);
					setFlagC(0);

					regs[A] = regs[A] ^ u8;

					break;
				}

				// OR A, u8
				case 0x06:
				{
					setFlagZ((uint8_t)(regs[A] | u8) == 0);
					setFlagN(0);
					setFlagH(0);
					setFlagC(0);

					regs[A] = regs[A] | u8;

					break;
				}

				// CP A, u8
				case 0x07:
				{
					setFlagZ((uint8_t)(regs[A] - u8) == 0);
					setFlagN(1);
					setFlagH((regs[A] & 0x0F) < (u8 & 0x0F));
					setFlagC(regs[A] < u8);

					break;
				}
			}

			break;
		}

		// RST
		case 0xC7: case 0xCF: case 0xD7: case 0xDF: case 0xE7: case 0xEF: case 0xF7: case 0xFF:
		{

			uint16_t address = (opcode & 0b00111000);

			write8(--SP, PC >> 8);
			write8(--SP, PC & 0xFF);

			PC = address;

			AddCycle();

			break;
		}

		// DI
		case 0xF3:
		{
			IME = 0;

			break;
		}

		// EI
		case 0xFB:
		{
			updateIME = 1;

			break;
		}
		
		// HALT
		case 0x76:
		{
			//std::cout << "HALT CALLED" << "\n";
			HALT = true;

			break;
		}

		// DAA
		case 0x27:
		{
			if (getFlagN())
			{
				uint8_t adj = 0x00;

				if (getFlagH())
				{
					adj += 0x06;
				}

				if (getFlagC())
				{
					adj += 0x60;
				}
				
				regs[A] = regs[A] - adj;
			}
			else
			{
				uint8_t adj = 0x00;

				if (getFlagC() || (regs[A] > 0x99))
				{
					adj += 0x60;

					setFlagC(1);
				}

				if (getFlagH() || ((regs[A] & 0x0F) > 0x09))
				{
					adj += 0x06;
				}
			
				regs[A] = regs[A] + adj;
			}

			setFlagZ(regs[A] == 0);
			setFlagH(0);

			break;
		}


		// CB PREFIX INSTRUCTIONS

		case 0xCB:
		{
			uint8_t suffix = fetch8();
			
			switch (suffix)
			{
				case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x07:
				case 0x08: case 0x09: case 0x0A: case 0x0B: case 0x0C: case 0x0D: case 0x0F:
				case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x17:
				case 0x18: case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D: case 0x1F:
				case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x27:
				case 0x28: case 0x29: case 0x2A: case 0x2B: case 0x2C: case 0x2D: case 0x2F:
				case 0x30: case 0x31: case 0x32: case 0x33: case 0x34: case 0x35: case 0x37:
				case 0x38: case 0x39: case 0x3A: case 0x3B: case 0x3C: case 0x3D: case 0x3F:
				{
					uint8_t operation = (suffix >> 3) & 7;
					uint8_t r8 = suffix & 7;

					switch (operation)
					{
						// RLC r8
						case 0x00:
						{
							uint8_t value = regs[r8];

							setFlagC((value >> 7) & 1);
							setFlagN(0);
							setFlagH(0);

							regs[r8] = (value << 1) | getFlagC();

							setFlagZ(regs[r8] == 0);

							break;
						}

						// RRC r8
						case 0x01:
						{
							uint8_t value = regs[r8];

							setFlagC(value & 1);
							setFlagN(0);
							setFlagH(0);

							regs[r8] = (value >> 1) | (getFlagC() << 7);

							setFlagZ(regs[r8] == 0);

							break;
						}

						// RL r8
						case 0x02:
						{
							uint8_t value = regs[r8];

							uint8_t carry = getFlagC();
							setFlagC((value >> 7) & 1);
							setFlagN(0);
							setFlagH(0);

							regs[r8] = (value << 1) | carry;

							setFlagZ(regs[r8] == 0);

							break;
						}

						// RR r8
						case 0x03:
						{
							uint8_t value = regs[r8];

							uint8_t carry = getFlagC();
							setFlagC(value & 1);
							setFlagN(0);
							setFlagH(0);

							regs[r8] = (value >> 1) | (carry << 7);

							setFlagZ(regs[r8] == 0);

							break;
						}

						// SLA r8
						case 0x04:
						{

							uint8_t value = regs[r8];

							setFlagC((value >> 7) & 1);
							setFlagN(0);
							setFlagH(0);

							regs[r8] = (value << 1);

							setFlagZ(regs[r8] == 0);


							break;
						}

						// SRA r8
						case 0x05:
						{
							uint8_t value = regs[r8];

							uint8_t b7 = (regs[r8] >> 7) & 1;

							setFlagC(value & 1);
							setFlagN(0);
							setFlagH(0);

							regs[r8] = (value >> 1) | (b7 << 7);

							setFlagZ(regs[r8] == 0);

							break;
						}

						
						// SWAP r8
						case 0x06:
						{
							regs[r8] = (regs[r8] >> 4) | (regs[r8] << 4);

							setFlagC(0);
							setFlagN(0);
							setFlagH(0);
							setFlagZ(regs[r8] == 0);

							break;
						}

						// SRL r8
						case 0x07:
						{
							uint8_t value = regs[r8];

							setFlagC(value & 1);
							setFlagN(0);
							setFlagH(0);

							regs[r8] = (value >> 1);

							setFlagZ(regs[r8] == 0);

							break;
						}

					}

					break;
				}


				case 0x06: case 0x0E: case 0x16: case 0x1E: case 0x26: case 0x2E:  case 0x36: case 0x3E:
				{
					uint8_t operation = (suffix >> 3) & 7;
					uint8_t value = read8(getHL());

					switch (operation)
					{
						// RLC (HL)
						case 0x00:
						{
							setFlagC((value >> 7) & 1);
							setFlagN(0);
							setFlagH(0);

							uint8_t result = (value << 1) | getFlagC();
							write8(getHL(), result);

							setFlagZ(result == 0);

							break;
						}

						// RRC (HL)
						case 0x01:
						{
							setFlagC(value & 1);
							setFlagN(0);
							setFlagH(0);

							uint8_t result = (value >> 1) | (getFlagC() << 7);
							write8(getHL(), result);

							setFlagZ(result == 0);

							break;
						}

						// RL (HL)
						case 0x02:
						{
							
							uint8_t carry = getFlagC();
							setFlagC((value >> 7) & 1);

							setFlagN(0);
							setFlagH(0);

							uint8_t result = (value << 1) | carry;
							write8(getHL(), result);

							setFlagZ(result == 0);

							break;
						}

						// RR (HL)
						case 0x03:
						{
							uint8_t carry = getFlagC();
							setFlagC(value & 1);
							setFlagN(0);
							setFlagH(0);

							uint8_t result = (value >> 1) | (carry << 7);
							write8(getHL(), result);

							setFlagZ(result == 0);

							break;
						}

						// SLA (HL)
						case 0x04:
						{
							setFlagC((value >> 7) & 1);
							setFlagN(0);
							setFlagH(0);

							uint8_t result = (value << 1);
							write8(getHL(), result);

							setFlagZ(result == 0);

							break;
						}

						// SRA (HL)
						case 0x05:
						{
							uint8_t b7 = (value >> 7) & 1;

							setFlagC(value & 1);
							setFlagN(0);
							setFlagH(0);

							uint8_t result = (value >> 1) | (b7 << 7);
							write8(getHL(), result);

							setFlagZ(result == 0);

							break;
						}


						// SWAP (HL)
						case 0x06:
						{
							value = (value >> 4) | (value << 4);
							write8(getHL(), value);

							setFlagC(0);
							setFlagN(0);
							setFlagH(0);
							setFlagZ(value == 0);

							break;
						}

						// SRL (HL)
						case 0x07:
						{
							setFlagC(value & 1);
							setFlagN(0);
							setFlagH(0);

							uint8_t result = (value >> 1);
							write8(getHL(), result);

							setFlagZ(result == 0);

							break;
						}
					}

					break;
				}
				// BIT u3, r8
				case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x47:
				case 0x48: case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4D: case 0x4F:
				case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x57:
				case 0x58: case 0x59: case 0x5A: case 0x5B: case 0x5C: case 0x5D: case 0x5F:
				case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x67:
				case 0x68: case 0x69: case 0x6A: case 0x6B: case 0x6C: case 0x6D: case 0x6F:
				case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75: case 0x77:
				case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7F:
				{
					uint8_t bIndex = (suffix >> 3) & 7;
					uint8_t r8 = suffix & 7;
					
					uint8_t b = (regs[r8] >> bIndex) & 1;

					setFlagZ(b == 0);
					setFlagN(0);
					setFlagH(1);

					break;
				}

				// BIT u3, (HL)
				case 0x46: case 0x4E: case 0x56: case 0x5E: case 0x66: case 0x6E:  case 0x76: case 0x7E:
				{
					uint8_t bIndex = (suffix >> 3) & 7;
					
					uint8_t value = read8(getHL());

					uint8_t b = (value >> bIndex) & 1;

					setFlagZ(b == 0);
					setFlagN(0);
					setFlagH(1);

					break;
				}

				// RES u3, r8
				case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x87:
				case 0x88: case 0x89: case 0x8A: case 0x8B: case 0x8C: case 0x8D: case 0x8F:
				case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x97:
				case 0x98: case 0x99: case 0x9A: case 0x9B: case 0x9C: case 0x9D: case 0x9F:
				case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4: case 0xA5: case 0xA7:
				case 0xA8: case 0xA9: case 0xAA: case 0xAB: case 0xAC: case 0xAD: case 0xAF:
				case 0xB0: case 0xB1: case 0xB2: case 0xB3: case 0xB4: case 0xB5: case 0xB7:
				case 0xB8: case 0xB9: case 0xBA: case 0xBB: case 0xBC: case 0xBD: case 0xBF:
				{
					uint8_t bIndex = (suffix >> 3) & 7;
					uint8_t r8 = suffix & 7;

					regs[r8] = regs[r8] & ~(1 << bIndex);

					break;
				}

				// RES u3, (HL)
				case 0x86: case 0x8E: case 0x96: case 0x9E: case 0xA6: case 0xAE:  case 0xB6: case 0xBE:
				{
					uint8_t bIndex = (suffix >> 3) & 7;
					uint8_t value = read8(getHL());

					uint8_t result = value & ~(1 << bIndex);

					write8(getHL(), result);

					break;
				}

				// SET u3, r8
				case 0xC0: case 0xC1: case 0xC2: case 0xC3: case 0xC4: case 0xC5: case 0xC7:
				case 0xC8: case 0xC9: case 0xCA: case 0xCB: case 0xCC: case 0xCD: case 0xCF:
				case 0xD0: case 0xD1: case 0xD2: case 0xD3: case 0xD4: case 0xD5: case 0xD7:
				case 0xD8: case 0xD9: case 0xDA: case 0xDB: case 0xDC: case 0xDD: case 0xDF:
				case 0xE0: case 0xE1: case 0xE2: case 0xE3: case 0xE4: case 0xE5: case 0xE7:
				case 0xE8: case 0xE9: case 0xEA: case 0xEB: case 0xEC: case 0xED: case 0xEF:
				case 0xF0: case 0xF1: case 0xF2: case 0xF3: case 0xF4: case 0xF5: case 0xF7:
				case 0xF8: case 0xF9: case 0xFA: case 0xFB: case 0xFC: case 0xFD: case 0xFF:
				{
					uint8_t bIndex = (suffix >> 3) & 7;
					uint8_t r8 = suffix & 7;

					regs[r8] = regs[r8] | (1 << bIndex);

					break;
				}


				// SET u3, (HL)
				case 0xC6: case 0xCE: case 0xD6: case 0xDE: case 0xE6: case 0xEE:  case 0xF6: case 0xFE:
				{
					uint8_t bIndex = (suffix >> 3) & 7;
					uint8_t value = read8(getHL());

					uint8_t result = value | (1 << bIndex);

					write8(getHL(), result);

					break;
				}

			}

			break;
		}

	}

}