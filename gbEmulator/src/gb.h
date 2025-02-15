#pragma once 

#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>

#include "cpu.h"
#include "mmu.h"
#include "ppu.h"
#include "renderingManager.h"
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
	GameBoy(GLFWwindow* window);
	~GameBoy();

	// pass empty string to savePath if no save file
	void readRom(const std::string path, const std::string savePath);
	
	uint8_t fetch();
	void decodeAndExecute(uint8_t opcode);

	void handleInterrupts();
	bool isCPUHalted();

	void checkForInput(GLFWwindow* window);
	void checkCartridgeType();
	void checkRomSize();
	void checkSramSize();
	
	bool keyPressed[379];
	bool keyDown[379];

	void enableInputRegisterBits(Button button);
	void disableInputRegisterBits(Button button);

	bool ssba = false;
	bool dPad = false;

	std::string filePath;

	// if the savePath is empty, it will save the file as the rom file path concatenated with _save
	void saveGame();
	void loadSave(std::string path);

	bool run();

	void processInput();

	bool validRomLoaded = false;

	bool restartRequested = false;
	bool loadSaveRequested = false;

	std::string saveFilePath = "";

	GLFWwindow* window;

	MMU mmu;
	PPU ppu;
	CPU cpu; 
	RenderingManager* renderingManager;
};