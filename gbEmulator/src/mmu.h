#pragma once 
#include <cinttypes>
#include <vector>

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

enum MBC
{
	MBC0,
	MBC1,
	MBC3,
	MBC5
};

class MMU
{
public:

	uint8_t testArray[0x10000];

	uint8_t bootrom[0x0100]; // --> this overlaps the rom when the program starts up and then handles control over to the actual 
	// cartridge rom at pc = 0x100. At the beginning when pc is at 0x00 - 0x100 range read boot rom array. Once pc reaches 0x100 subsequent memory
	// reads the cartridge array. Could probably be done with just a bool ex: if(pc == 0x0100 && !bootromDone) bootromDone = true
	
	uint8_t rom0[0x4000]; // 16 KiB ROM bank 00 and 16 KiB ROM Bank 01–NN 

	uint8_t romBanks[0x4000];

	uint8_t vRam[0x2000]; // 8 KiB Video RAM (VRAM)
	std::vector<uint8_t> eRam; // --> external ram can have variable size depending on mbc
	uint8_t wRam[0x2000]; // 8 KiB Work RAM(WRAM)

	// 0xE000 - 0xFDFF --> echo ram just read from wram --> wRam[address - 0xE000]

	uint8_t oam[0x00A0];  // Object attribute memory (OAM)
	uint8_t ioRegs[0x0080]; // I/O Registers
	uint8_t hRam[0x007F]; // High RAM (HRAM)
	uint8_t ie[0x0001]; // --> FFFF interrupt enable register (IE)

	std::vector<uint8_t> fullrom;

	MBC mbc;
	
	bool sRamEnabled = false;
	
	uint16_t romBankNumber = 1;
	uint8_t ramBankNumber = 0;
	uint16_t highBankNumber = 0;
	uint16_t zeroBankNumber = 0;
	bool modeFlag = 0;

	//bool rtcRegisterEnabled = false;

	uint8_t mappedRTCRegister = 0;

	bool cartridgeHasRTC = false;

	unsigned int rtcCycleCounter = 0;

	unsigned int romNumOfBanks = 0;
	unsigned int sRamSize = 0;
	unsigned int sRamNumOfBanks = 0;

	uint8_t lastLatchWrite = 0;

	struct RTC
	{
		// 6 bit seconds
		uint8_t s = 0;
		// 6 bit minutes
		uint8_t m = 0;
		// 5 bit hours
		uint8_t h = 0;
		// 8 bit --> lower 8 bits of rhte total 9-bit day counter
		uint8_t dl = 0;
		// bit 0 --> bit 8 of the day counter | bit 6 --> timer halt bit | bit 7 --> day counter carry bit
		uint8_t dh = 0;
		// whole 9-bit rtc day counter;
		uint16_t rtcDayCounter = 0;
	} rtc;

	struct RTClatched
	{
		uint8_t s = 0;
		uint8_t m = 0;
		uint8_t h = 0;
		uint8_t dl = 0;
		uint8_t dh = 0;
	} rtcLatched;

	bool latchOccurred = false;

	// whenever read using pc increase pc --> read8(PC++)
	uint8_t read8(uint16_t address);
	void write8(uint16_t address, uint8_t value);
	void writeToRomMemory(uint16_t index, uint8_t value);

	bool dmaTransferRequested = false;
	unsigned int dmaDelay = 0;
	uint16_t dmaSource;
	void dmaTransfer(unsigned int count);

	// sets the corresponding bit in IF
	void requestInterrupt(Interrupt type);

	// unset the corresponding bit in IF
	void cancelInterrupt(Interrupt type);

	// return true if corresponding bit in IF is set
	bool isInterruptRequested(Interrupt type);
	// return true if the corresponding bit in IE is set
	bool isInterruptEnabled(Interrupt type);

	uint8_t getMBC1HighBankNumber();
	
	uint8_t getMBC1ZeroBankNumber();
};