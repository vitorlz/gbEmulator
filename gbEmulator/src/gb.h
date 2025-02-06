#pragma once 

#include "cpu.h"
#include "mmu.h"
#include "ppu.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>

enum Button
{
	BUTTON_A,
	BUTTON_B,
	BUTTON_UP,
	BUTTON_DOWN,
	BUTTON_LEFT,
	BUTTON_RIGHT,
	BUTTON_START,
	BUTTON_SELECT,
};

class GameBoy
{
public:
	GameBoy();

	void readRom(const std::string path);
	
	uint8_t fetch();
	void decodeAndExecute(uint8_t opcode);

	void handleInterrupts();
	bool isCPUHalted();

	void checkForInput(GLFWwindow* window);
	
	bool keyPressed[379];
	bool keyDown[379];
	std::vector<uint8_t> fullrom;

	void enableInputRegisterBits(Button button);
	void disableInputRegisterBits(Button button);

	bool ssba = false;
	bool dPad = false;

	MMU mmu;
	PPU ppu;
	CPU cpu;	
};