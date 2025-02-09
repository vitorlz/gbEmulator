#pragma once

#include <iostream>

#include "gb.h"

#define B 0
#define C 1
#define D 2
#define E 3
#define H 4
#define L 5
#define F 6
#define A 7

class JsonTest
{
public:
	JsonTest(GameBoy& gb);

	void RunTest(const std::string path);
	void RunAllTests();

	GameBoy& gb;
};