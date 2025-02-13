#pragma once

#include <iostream>
#include "gb.h"
#include  "regs.h"

class JsonTest
{
public:
	JsonTest(GameBoy& gb);

	void RunTest(const std::string path);
	void RunAllTests();

	GameBoy& gb;
};