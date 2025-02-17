
#include "gb.h"
#include "mmu.h"

#define JOYPAD_ADDRESS 0xFF00


GameBoy::GameBoy(GLFWwindow* window)
    : ppu(mmu), cpu(mmu, ppu), renderingManager(nullptr), window(window)
{
    // initialize glfw window;

    renderingManager = new RenderingManager(*this);

    // create shader program, vao, and display texture
    renderingManager->init(window);
    ppu.window = window;

    ppu.displayTexture = renderingManager->displayTexture;
    ppu.VAO = renderingManager->vao;
    ppu.shaderProgram = renderingManager->shader;

    // set joypad input registers to all 1's (off)
    mmu.ioRegs[0] = 0b00001111;
}

GameBoy::~GameBoy()
{
    delete renderingManager;
}

void GameBoy::readRom(const std::string rom, const std::string savePath)
{
    filePath = rom;

    std::ifstream is(rom.c_str(), std::ifstream::binary);
    if (is) 
    {
        is.seekg(0, is.end);
        int length = is.tellg();
        is.seekg(0, is.beg);

        char* buffer = new char[length];

        is.read(buffer, length);

        if (length <= 0)
        {
            std::cout << "Error: ROM is empty!" << "\n";
            validRomLoaded = false;
            return;
        }

        if (is)
            std::cout << "rom read successfully\n";
        else
            std::cout << "error: only " << is.gcount() << " could be read\n";

        std::cout << "rom length: " << length << "\n";

        for (int i = 0; i < length; i++)
        {
            mmu.fullrom.push_back(buffer[i]);            
        }

        checkCartridgeType();
        checkRomSize();
        checkSramSize();

        if (mmu.cartHasBattery)
            loadSave(savePath);
        
        // rom bank number can be 0 in MBC5, unlike in other MBC's
        if (mmu.mbc == MBC5)
            mmu.romBankNumber = 0;

        mmu.eRam.resize(mmu.sRamSize);
        
        is.close();
        delete[] buffer;
        
        validRomLoaded = true;
    }
    else
    {
        std::cout << "Error: Failed to read rom!" << "\n";
        validRomLoaded = false;
    }
}

void GameBoy::checkForInput(GLFWwindow* window)
{
    uint8_t reg = mmu.read8(JOYPAD_ADDRESS);

    ssba = !((reg >> 5) & 1);
    dPad = !((reg >> 4) & 1);
   
    if (!ssba && !dPad)
    {
        mmu.write8(JOYPAD_ADDRESS, 0xFF);
        return;
    }

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

void GameBoy::saveGame()
{
    std::string saveName;
    if (saveFilePath.empty())
    {
        saveName = filePath + "_save";
    }
    else
    {
        saveName = saveFilePath;
    }

    std::ofstream outputFileStream; 
    outputFileStream.open(saveName, std::ios::out | std::ios::binary);

    for (int i = 0; i < mmu.eRam.size(); i++)
        outputFileStream.write((char*)&mmu.eRam[i], sizeof(uint8_t));


    std::cout << "saved file!: " << saveName << "\n";

    outputFileStream.close();
}

void GameBoy::loadSave(std::string path)
{
    std::string saveName;
    if (path.empty())
    {
        saveName = filePath + "_save";
    }
    else
    {
        saveName = path;
    }
   
    std::ifstream is(saveName.c_str(), std::ifstream::binary);
    if (is)
    {
        is.seekg(0, is.end);
        int length = is.tellg();
        is.seekg(0, is.beg);

        char* buffer = new char[length];

        is.read(buffer, length);

        if (length <= 0)
        {
            std::cout << "Failed to read save file" << "\n";
        }

        if (is)
            std::cout << "save file read successfully\n";
        else
            std::cout << "error: only " << is.gcount() << " could be read\n";
            
        std::cout << "save file length: " << length << "\n";

        for (int i = 0; i < length; i++)
        {
            mmu.eRam.push_back(buffer[i]);
        }
    }
    else
    {
        std::cout << "Failed to read save file" << "\n";
    }
}

bool GameBoy::run()
{
    double lastFrameTime = 0;   
    double lastFrameTimeCycles = 0;
    double lastUpdateTimeCycles = 0;
   
    while (!glfwWindowShouldClose(window) && !restartRequested)
    {
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastUpdateTimeCycles;

        if (validRomLoaded)
        {
            if (!isCPUHalted())
            {
                uint8_t opcode = fetch();
                checkForInput(window);
                decodeAndExecute(opcode);
            }
            else
            {
                cpu.AddCycle();
            }
            handleInterrupts();
        }
        
        if ((currentTime - lastFrameTime) >= (1.0/60.0))
        {
            processInput();

            glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            if (validRomLoaded)
            {
                glUseProgram(ppu.shaderProgram);
                glBindTexture(GL_TEXTURE_2D, ppu.displayTexture);
                glBindVertexArray(ppu.VAO);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }

            renderingManager->renderUI();

            glfwSwapBuffers(window);
            glfwPollEvents();
            
            lastFrameTime = currentTime;
        }

        static int frameCount = 0;
        if (cpu.tCycles >= 70224)
        {
            frameCount++;
            while ((glfwGetTime() - lastUpdateTimeCycles) < fpsLimit)
            {
            }

            if (frameCount >= 10)
            {
                std::cout << "actual fps: " << 1 / (glfwGetTime() - lastUpdateTimeCycles) << "\n";
                frameCount = 0;
            }

            lastUpdateTimeCycles = glfwGetTime();
            cpu.tCycles = cpu.tCycles - 70224;
        }
    }
    renderingManager->terminateUI();
    
    return false;
}

void GameBoy::processInput()
{
    
    for (int i = 32; i < 349; i++)
    {
        if (glfwGetKey(window, i) == GLFW_PRESS && !keyDown[i])
        {
            keyPressed[i] = true;
        }
        else
        {
            keyPressed[i] = false;
        }

        if (glfwGetKey(window, i) == GLFW_PRESS)
        {
            keyDown[i] = true;
        }
        else
        {
            keyDown[i] = false;
        }
    }

    // game is saved automatically when the window closes, but there is also the option to save it manually
    if (keyPressed[GLFW_KEY_F3])
    {
        std::cout << "SAVING..." << "\n";
        saveGame();
        std::cout << "DONE" << "\n";
    }
    
}




void GameBoy::checkCartridgeType()
{
    std::cout << "Cartridge type: ";
    switch (mmu.fullrom[0x0147])
    {
    case 0x00:
        std::cout << "ROM ONLY" << "\n";
        mmu.mbc = MBC0;
        break;
    case 0x01:
        std::cout << "MBC1" << "\n";
        mmu.mbc = MBC1;
        break;
    case 0x02:
        std::cout << "MBC1+RAM" << "\n";
        mmu.mbc = MBC1;
        break;
    case 0x03:
        std::cout << "MBC1+RAM+BATTERY" << "\n";
        mmu.cartHasBattery = true;
        mmu.mbc = MBC1;
        break;
    case 0x0F:
        std::cout << "MBC3+TIMER+BATTERY" << "\n";
        mmu.cartHasBattery = true;
        mmu.cartridgeHasRTC = true;
        mmu.mbc = MBC3;
        break;
    case 0x10:
        std::cout << "MBC3+TIMER+RAM+BATTERY" << "\n";
        mmu.cartHasBattery = true;
        mmu.cartridgeHasRTC = true;
        mmu.mbc = MBC3;
        break;
    case 0x11:
        std::cout << "MBC3" << "\n";
        mmu.mbc = MBC3;
        break;
    case 0x12:
        std::cout << "MBC3+RAM" << "\n";
        mmu.mbc = MBC3;
        break;
    case 0x13:
        std::cout << "MBC3+RAM+BATTERY" << "\n";
        mmu.cartHasBattery = true;
        mmu.mbc = MBC3;
        break;
    case 0x19:
        std::cout << "MBC5" << "\n";
        mmu.mbc = MBC5;
        break;
    case 0x1A:
        std::cout << "MBC5+RAM" << "\n";
        mmu.mbc = MBC5;
        break;
    case 0x1B:
        std::cout << "MBC5+RAM+BATTERY" << "\n";
        mmu.cartHasBattery = true;
        mmu.mbc = MBC5;
        break;
    case 0x1C:
        std::cout << "MBC5+RUMBLE" << "\n";
        mmu.cartridgeHasRumble = true;
        mmu.mbc = MBC5;
        break;
    case 0x1D:
        std::cout << "MBC5+RUMBLE+RAM" << "\n";
        mmu.cartridgeHasRumble = true;
        mmu.mbc = MBC5;
        break;
    case 0x1E:
        std::cout << "MBC5+RUMBLE+RAM+BATTERY" << "\n";
        mmu.cartHasBattery = true;
        mmu.cartridgeHasRumble = true;
        mmu.mbc = MBC5;
        break;
    }
}
void GameBoy::checkRomSize()
{
    std::cout << "Rom size: ";
    switch (mmu.fullrom[0x0148])
    {
    case 0x00:
        std::cout << "32 KiB" << "\n";
        std::cout << "2 ROM banks" << "\n";
        mmu.romNumOfBanks = 2;
        break;
    case 0x01:
        std::cout << "64 KiB" << "\n";
        std::cout << "4 ROM banks" << "\n";
        mmu.romNumOfBanks = 4;
        break;
    case 0x02:
        std::cout << "128 KiB" << "\n";
        std::cout << "8 ROM banks" << "\n";
        mmu.romNumOfBanks = 8;
        break;
    case 0x03:
        std::cout << "256 KiB" << "\n";
        std::cout << "16 ROM banks" << "\n";
        mmu.romNumOfBanks = 16;
        break;
    case 0x04:
        std::cout << "512 KiB" << "\n";
        std::cout << "32 ROM banks" << "\n";
        mmu.romNumOfBanks = 32;
        break;
    case 0x05:
        std::cout << "1 MiB" << "\n";
        std::cout << "64 ROM banks" << "\n";
        mmu.romNumOfBanks = 64;
        break;
    case 0x06:
        std::cout << "2 MiB" << "\n";
        std::cout << "128 ROM banks" << "\n";
        mmu.romNumOfBanks = 128;
        break;
    case 0x07:
        std::cout << "4 MiB" << "\n";
        std::cout << "256 ROM banks" << "\n";
        mmu.romNumOfBanks = 256;
        break;
    case 0x08:
        std::cout << "8 MiB" << "\n";
        std::cout << "512 ROM banks" << "\n";
        mmu.romNumOfBanks = 512;
        break;
    case 0x52:
        std::cout << "1.1 MiB" << "\n";
        std::cout << "72 ROM banks" << "\n";
        mmu.romNumOfBanks = 72;
        break;
    case 0x53:
        std::cout << "1.2 MiB" << "\n";
        std::cout << "80 ROM banks" << "\n";
        mmu.romNumOfBanks = 80;
        break;
    case 0x54:
        std::cout << "1.5 MiB" << "\n";
        std::cout << "96 ROM banks" << "\n";
        mmu.romNumOfBanks = 96;
        break;
    }
}
void GameBoy::checkSramSize()
{

    std::cout << "SRAM size: ";
    switch (mmu.fullrom[0x0149])
    {
    case 0x00:
        std::cout << "0 KiB" << "\n";
        std::cout << "No SRAM" << "\n";
        mmu.sRamNumOfBanks = 0;
        mmu.sRamSize = 0x0;
        break;
    case 0x01:
        std::cout << "--" << "\n";
        std::cout << "Unused / 2KiB" << "\n";
        mmu.sRamNumOfBanks = 0;
        mmu.sRamSize = 0x800;
        break;
    case 0x02:
        std::cout << "8 KiB" << "\n";
        std::cout << "1 SRAM bank" << "\n";
        mmu.sRamNumOfBanks = 1;
        mmu.sRamSize = 0x2000;
        break;
    case 0x03:
        std::cout << "32 KiB" << "\n";
        std::cout << "4 RAM banks" << "\n";
        mmu.sRamNumOfBanks = 4;
        mmu.sRamSize = 0x8000;
        break;
    case 0x04:
        std::cout << "128 KiB" << "\n";
        std::cout << "16 RAM banks" << "\n";
        mmu.sRamNumOfBanks = 16;
        mmu.sRamSize = 0x20000;
        break;
    case 0x05:
        std::cout << "64 KiB" << "\n";
        std::cout << "8 RAM banks" << "\n";
        mmu.sRamNumOfBanks = 8;
        mmu.sRamSize = 0x10000;
        break;
    }
}

