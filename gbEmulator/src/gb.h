#pragma once 

#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>

#include "cpu.h"
#include "mmu.h"
#include "ppu.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>



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

	void enableInputRegisterBits(Button button);
	void disableInputRegisterBits(Button button);

	bool ssba = false;
	bool dPad = false;

	std::string filePath;

	void saveGame();
	void loadSave();

	MMU mmu;
	PPU ppu;
	CPU cpu;	
};