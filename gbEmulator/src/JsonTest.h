#pragma once

#include "gb.h"

#include <iostream>

class JsonTest
{
public:
	JsonTest(GameBoy& gb);

	void RunTest(const std::string path);
	void RunAllTests();

	GameBoy& gb;
};