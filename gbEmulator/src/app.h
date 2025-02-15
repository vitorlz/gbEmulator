#pragma once

#include "gb.h"

class App
{
public:
	App();

	const unsigned int SCR_WIDTH = 320;
	const unsigned int SCR_HEIGHT = 308;

	void initWindow();
	void onClose();
	void restart();
	void onResize(int width, int height);
	void run(std::string rom);

	GLFWwindow* window;
	std::unique_ptr<GameBoy> gb;
};