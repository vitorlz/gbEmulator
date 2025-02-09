
#include <fstream>
#include <sstream>
#include <iostream>

#include <nlohmann/json.hpp>

#include "JsonTest.h"



JsonTest::JsonTest(GameBoy& gb)
	: gb(gb) {}


void JsonTest::RunTest(const std::string path)
{
	std::cout << "Loading Test..." << "\n";

	bool cpuPass = true;
	bool ramPass = true;

	bool cpuFailed = false;
	bool ramFailed = false;

	std::ifstream f(path);
	if (!f.is_open())
	{
		std::cout << "Failed to open json file. \n";

		return;
	}
	nlohmann::json tests;
	f >> tests;
	f.close();


	uint8_t opcode = std::stoi(tests[0]["name"].get<std::string>().substr(0, 2), nullptr, 16);
	if (opcode != 0xCB)
	{
		std::cout << "TESTING OPCODE 0x" << std::hex << (int)opcode << "\n";
	}
	else
	{
		uint8_t suffix = std::stoi(tests[0]["name"].get<std::string>().substr(3, 5), nullptr, 16);
		std::cout << "TESTING OPCODE 0x" << std::hex << (int)opcode << std::hex << (int)suffix << "\n";
	}

	bool sample = false;

	for (const auto& test : tests)
	{	
		int testNumber = std::stoi(test["name"].get<std::string>().substr(2, 5), nullptr, 16);
	
		// set initial state
		if (test.contains("initial"))
		{
			gb.cpu.PC = test["initial"]["pc"];
			gb.cpu.SP = test["initial"]["sp"];
			gb.cpu.regs[A] = test["initial"]["a"];
			gb.cpu.regs[B] = test["initial"]["b"];
			gb.cpu.regs[C] = test["initial"]["c"];
			gb.cpu.regs[D] = test["initial"]["d"];
			gb.cpu.regs[E] = test["initial"]["e"];
			gb.cpu.regs[F] = test["initial"]["f"];
			gb.cpu.regs[H] = test["initial"]["h"];
			gb.cpu.regs[L] = test["initial"]["l"];

			int ime = test["initial"]["ime"];

			gb.cpu.IME = (bool)ime;
			gb.mmu.testArray[0xFFFF] = test["initial"]["ie"];

			for (const auto& mem : test["initial"]["ram"])
			{
				gb.mmu.testArray[mem[0]] = mem[1];

			}
		}

		// increment pc and execute instruction (simulate a fetch)
		gb.cpu.PC++;
		gb.cpu.AddCycle();
		gb.decodeAndExecute(opcode);
		
		// compare with final state
		if (test.contains("final"))
		{

			int ime = test["final"]["ime"];

			cpuPass = (gb.cpu.PC == test["final"]["pc"]) *
				(gb.cpu.SP == test["final"]["sp"]) *
				(gb.cpu.regs[A] == test["final"]["a"]) *
				(gb.cpu.regs[B] == test["final"]["b"]) *
				(gb.cpu.regs[C] == test["final"]["c"]) *
				(gb.cpu.regs[D] == test["final"]["d"]) *
				(gb.cpu.regs[E] == test["final"]["e"]) *
				(gb.cpu.regs[F] == test["final"]["f"]) *
				(gb.cpu.regs[H] == test["final"]["h"]) *
				(gb.cpu.regs[L] == test["final"]["l"]) *
				(gb.cpu.IME == (bool)ime);

			if (test["final"].contains("ei"))
			{
				cpuPass *= (gb.mmu.testArray[0xFFFF] == test["initial"]["ei"]);
			}
				

			if (!cpuPass && !sample)
			{

				std::cout << "TEST NUMBER : " << testNumber << "\n";
				std::cout << "CORRECT FINAL STATE: "
					<< "PC: " << test["final"]["pc"] << " "
					<< "SP: " << test["final"]["sp"] << " "
					<< "A: " << test["final"]["a"] << " "
					<< "B: " << test["final"]["b"] << " "
					<< "C: " << test["final"]["c"] << " "
					<< "D: " << test["final"]["d"] << " "
					<< "E: " << test["final"]["e"] << " "
					<< "F: " << test["final"]["f"] << " "
					<< "H: " << test["final"]["h"] << " "
					<< "L: " << test["final"]["l"] << " "
					<< "IME: " << test["final"]["ime"] << " "
					<< "IE: " << std::dec << test["final"]["ei"] << " "
					<< "\n";

				std::cout << "YOUR RESULT:         "
					<< "PC: " << std::dec << (int)gb.cpu.PC << " "
					<< "SP: " << std::dec << (int)gb.cpu.SP << " "
					<< "A: " << std::dec << (int)gb.cpu.regs[A] << " "
					<< "B: " << std::dec << (int)gb.cpu.regs[B] << " "
					<< "C: " << std::dec << (int)gb.cpu.regs[C] << " "
					<< "D: " << std::dec << (int)gb.cpu.regs[D] << " "
					<< "E: " << std::dec << (int)gb.cpu.regs[E] << " "
					<< "F: " << std::dec << (int)gb.cpu.regs[F] << " "
					<< "H: " << std::dec << (int)gb.cpu.regs[H] << " "
					<< "L: " << std::dec << (int)gb.cpu.regs[L] << " "
					<< "IME: " << std::dec << (int)gb.cpu.IME << " "
					<< "IE: " << std::dec << (int)gb.mmu.testArray[0xFFFF] << " "
					<< "\n";

				cpuFailed = true;
				sample = true;

				std::cout << "\n\n\n";
			}

			for (const auto& mem : test["final"]["ram"])
			{
				ramPass = gb.mmu.testArray[mem[0]] == mem[1];
				if (!ramPass)
				{
					std::cout << "CORRECT VALUE STORED IN ADDRESS " << mem[0] << " :" << mem[1] << "\n";
					std::cout << "ACTUAL VALUE: " << gb.mmu.testArray[mem[0]] << "\n";

					ramFailed = true;
				}
			}
		}

	}

	if (!cpuFailed && !ramFailed)
	{
		std::cout << "PASSED" << "\n\n\n";
	}

}

void JsonTest::RunAllTests()
{
	for (int i = 0; i <= 255; i++)
	{
		std::stringstream stream;
		stream << std::hex << i;
		std::string result(stream.str());

		if (result.size() == 1)
			result = "0" + result;

		if (result == "d3" || result == "db" || result == "dd" || result == "e3" || result == "e4" || result == "eb"
			|| result == "ec" || result == "ed" || result == "f4" || result == "fc" || result == "fd" || result == "cb")
		{
			continue;
		}

		std::string path = "res/jsonTests/" + result + ".json";

		RunTest(path);
	}

	for (int i = 0; i <= 255; i++)
	{

		std::stringstream stream;
		stream << std::hex << i;
		std::string result(stream.str());

		if (result.size() == 1)
			result = "0" + result;

		std::string path = "res/jsonTests/cb " + result + ".json";

		RunTest(path);
	}
}
