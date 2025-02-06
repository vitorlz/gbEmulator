#include "gb.h"
#include "mmu.h"

#define JOYPAD_ADDRESS 0xFF00


GameBoy::GameBoy()
	:	ppu(mmu),
		cpu(mmu, ppu)
{

    mmu.ioRegs[0] = 0b00001111;
}

void GameBoy::readRom(const std::string rom)
{
    std::ifstream is(rom.c_str(), std::ifstream::binary);
    if (is) 
    {
        is.seekg(0, is.end);
        int length = is.tellg();
        is.seekg(0, is.beg);

        char* buffer = new char[length];

        is.read(buffer, length);

        if (is)
            std::cout << "rom read successfully\n";
        else
            std::cout << "error: only " << is.gcount() << " could be read\n";

        for (int i = 0; i < length; i++)
        {
            fullrom.push_back(buffer[i]);            
        }

        // put first 32kib of rom in mmu rom for now since we are using mbc0
        for (uint16_t i = 0; i < 0x8000; i++)
        {
            //mmu.rom[i] = fullrom[i];

            mmu.writeToRomMemory(i, fullrom[i]);
        }
        
        is.close();
        delete[] buffer;
    }
}


void GameBoy::checkForInput(GLFWwindow* window)
{
    uint8_t reg = mmu.read8(JOYPAD_ADDRESS);

    ssba = !((reg >> 5) & 1);
    dPad = !((reg >> 4) & 1);
   

    if (keyDown[GLFW_KEY_RIGHT] && dPad)
    {
        enableInputRegisterBits(BUTTON_RIGHT);
    }
    else if (dPad)
    {
        disableInputRegisterBits(BUTTON_RIGHT);
    }

    if (keyDown[GLFW_KEY_LEFT] && dPad)
    {
        enableInputRegisterBits(BUTTON_LEFT);
    }
    else if (dPad)
    {
        disableInputRegisterBits(BUTTON_LEFT);
    }

    if (keyDown[GLFW_KEY_UP] && dPad)
    {
        enableInputRegisterBits(BUTTON_UP);
    }
    else if (dPad)
    {
        disableInputRegisterBits(BUTTON_UP);
    }

    if (keyDown[GLFW_KEY_DOWN] && dPad)
    {
        enableInputRegisterBits(BUTTON_DOWN);
    }
    else if (dPad)
    {
        disableInputRegisterBits(BUTTON_DOWN);
    }

    if (keyDown[GLFW_KEY_X] && ssba)
    {
        enableInputRegisterBits(BUTTON_A);
    }
    else if (ssba)
    {
        disableInputRegisterBits(BUTTON_A);
    }

    if (keyDown[GLFW_KEY_Z] && ssba)
    {
        enableInputRegisterBits(BUTTON_B);
    }
    else if (ssba)
    {
        disableInputRegisterBits(BUTTON_B);
    }

    if (keyDown[GLFW_KEY_BACKSPACE] && ssba)
    {
        enableInputRegisterBits(BUTTON_SELECT);
    }
    else if (ssba)
    {
        disableInputRegisterBits(BUTTON_SELECT);
    }

    if (keyDown[GLFW_KEY_ENTER] && ssba)
    {
        enableInputRegisterBits(BUTTON_START);
    }
    else if (ssba)
    {
        disableInputRegisterBits(BUTTON_START);
    }

   

}

void GameBoy::enableInputRegisterBits(Button button)
{
    uint8_t reg = mmu.read8(JOYPAD_ADDRESS);

    if (button == BUTTON_A)
    {
        // using the cpu write functions because they prevent writing while a dma transfer is happening
        reg = reg & ~(1);

        mmu.write8(JOYPAD_ADDRESS, reg);
    }
    else if (button == BUTTON_B)
    {
        reg = reg & ~((uint8_t)(1 << 1));

        mmu.write8(JOYPAD_ADDRESS, reg);
    }
    else if (button == BUTTON_SELECT)
    {
        reg = reg & ~((uint8_t)(1 << 2));

        mmu.write8(JOYPAD_ADDRESS, reg);
    }
    else if (button == BUTTON_START)
    {
        reg = reg & ~((uint8_t)(1 << 3));

        mmu.write8(JOYPAD_ADDRESS, reg);
    }
    else if (button == BUTTON_RIGHT)
    {
        reg = reg & ~((uint8_t)1);

        mmu.write8(JOYPAD_ADDRESS, reg);
    }
    else if (button == BUTTON_LEFT)
    {
        reg = reg & ~((uint8_t)(1 << 1));

        mmu.write8(JOYPAD_ADDRESS, reg);
    }
    else if (button == BUTTON_UP)
    {
        reg = reg & ~((uint8_t)(1 << 2));

        mmu.write8(JOYPAD_ADDRESS, reg);
    }
    else if (button == BUTTON_DOWN)
    {
        reg = reg & ~((uint8_t)(1 << 3));

        mmu.write8(JOYPAD_ADDRESS, reg);
    }
}

void GameBoy::disableInputRegisterBits(Button button)
{
    uint8_t reg = mmu.read8(JOYPAD_ADDRESS);

    if (button == BUTTON_A)
    {
        // using the cpu write functions because they prevent writing while a dma transfer is happening
        reg = reg | 1;

        mmu.write8(JOYPAD_ADDRESS, reg);
    }
    else if (button == BUTTON_B)
    {
        reg = reg | (1 << 1);

        mmu.write8(JOYPAD_ADDRESS, reg);
    }
    else if (button == BUTTON_SELECT)
    {
        reg = reg | (1 << 2);

        mmu.write8(JOYPAD_ADDRESS, reg);
    }
    else if (button == BUTTON_START)
    {
        reg = reg | (1 << 3);

        mmu.write8(JOYPAD_ADDRESS, reg);
    }
    else if (button == BUTTON_RIGHT)
    {
        reg = reg | 1;

        mmu.write8(JOYPAD_ADDRESS, reg);
    }
    else if (button == BUTTON_LEFT)
    {
        reg = reg | (1 << 1);

        mmu.write8(JOYPAD_ADDRESS, reg);
    }
    else if (button == BUTTON_UP)
    {
        reg = reg | (1 << 2);

        mmu.write8(JOYPAD_ADDRESS, reg);
    }
    else if (button == BUTTON_DOWN)
    {
        reg = reg | (1 << 3);

        mmu.write8(JOYPAD_ADDRESS, reg);
    }
}


uint8_t GameBoy::fetch()
{
    return cpu.fetch8();
}

void GameBoy::decodeAndExecute(uint8_t opcode)
{
    cpu.decodeAndExecute(opcode);
}

void GameBoy::handleInterrupts()
{
    cpu.handleInterrupts();
}

bool GameBoy::isCPUHalted()
{
    return cpu.HALT;
}