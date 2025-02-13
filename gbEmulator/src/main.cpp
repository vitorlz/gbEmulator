#include <iomanip>
#include <iostream>
#include <limits>
#include <chrono>
#include <thread>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "gb.h"
#include  "regs.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void window_close_callback(GLFWwindow* window);
void processInput(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 320;
const unsigned int SCR_HEIGHT = 288;

GameBoy gb;

int main(int argc, char* argv[])
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "gbEmulator", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetWindowCloseCallback(window, window_close_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;

    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try
    {
        vShaderFile.open("res/shaders/pixel.vert");
        fShaderFile.open("res/shaders/pixel.frag");
        std::stringstream vShaderStream, fShaderStream;
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        vShaderFile.close();
        fShaderFile.close();
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    }
    catch (std::ifstream::failure e)
    {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ" << std::endl;
    }

    // compile shaders and link program

    const char* vertexShaderSource = vertexCode.c_str();
    const char* fragmentShaderSource = fragmentCode.c_str();

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    gb.ppu.shaderProgram = glCreateProgram();
    glAttachShader(gb.ppu.shaderProgram, vertexShader);
    glAttachShader(gb.ppu.shaderProgram, fragmentShader);
    glLinkProgram(gb.ppu.shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    float vertices[] = {
        // positions   // texCoords
          -1.0f,  1.0f,  0.0f, 1.0f,
          -1.0f, -1.0f,  0.0f, 0.0f,
           1.0f, -1.0f,  1.0f, 0.0f,

          -1.0f,  1.0f,  0.0f, 1.0f,
           1.0f, -1.0f,  1.0f, 0.0f,
           1.0f,  1.0f,  1.0f, 1.0f
    };
   
    unsigned int VBO;
    glGenVertexArrays(1, &gb.ppu.VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(gb.ppu.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);

    
    // create 160 * 144 texture for display buffer

    

    glGenTextures(1, &gb.ppu.displayTexture);
    glBindTexture(GL_TEXTURE_2D, gb.ppu.displayTexture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 160, 144, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, 0);

    
    std::string rom = argc > 1 ? argv[1] : "C:/dev/gbEmulator/gbEmulator/res/testroms/zelda.gb";
    gb.readRom(rom);

    glfwSwapInterval(0);

    double lastTime = 0;
    double currentTime = 0;

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // update loop --> fetch, decode, execute. In a nutshell we need 17556 m-cycles per screen. So in order to render 
        // at 59.7 fps we need we need 59.7 screens per second --> 1,048,576 cycles per second. This means that we will have to count the cycles
        // at every instruction and if the number of cycles per second surpasses 1,048,576 we will have to sleep and wait a bit. 
        // cpu instructions will tick other systems depending on how many cycles they take.
        // ppu should be updated every t-cycle, this means that we need to tick the ppu 4x for every m-cycle
        currentTime = glfwGetTime();
        
        if (!gb.isCPUHalted())
        {
            uint8_t opcode = gb.fetch();

            if (gb.keyPressed[GLFW_KEY_A])
            {
                std::cout << std::hex << "PC: " << (gb.cpu.PC - 1) << " opcode: " << std::hex << int(opcode) << "\n";
                std::cout << "STAT: " << std::hex << (int)gb.mmu.read8(0xFF41) << "\n";
                std::cout << "E REG: " << std::hex << (int)gb.cpu.regs[REG_A] << "\n";
                std::cout << "A REG: " << std::hex << (int)gb.cpu.regs[REG_A] << "\n";
                std::cout << "Z FLAG: " << gb.cpu.getFlagZ() << "\n";
                std::cout << "PPU STATE: " << gb.ppu.isDisplayEnabled() << "\n";
            }

            if (gb.keyPressed[GLFW_KEY_B])
            {
                std::cout << std::dec << "IE: " << (int)gb.mmu.read8(0xFFFF) << "\n";
                std::cout << "IME: " << gb.cpu.IME << "\n";
            }

            gb.checkForInput(window);
            gb.decodeAndExecute(opcode);  
        }
        else
        {
            gb.cpu.AddCycle();
        }

        gb.handleInterrupts();

      

        if (gb.cpu.tCycles >= 70224)
        {

            processInput(window);
            for (int i = 32; i < 349; i++)
            {
                if (glfwGetKey(window, i) == GLFW_PRESS && !gb.keyDown[i])
                {
                    gb.keyPressed[i] = true;

                    std::cout << "key pressed: " << i << "\n";
                }
                else
                {
                    gb.keyPressed[i] = false;
                }

                if (glfwGetKey(window, i) == GLFW_PRESS)
                {
                    gb.keyDown[i] = true;

                }
                else
                {
                    gb.keyDown[i] = false;
                }
            }

            if (gb.keyPressed[GLFW_KEY_F3])
            {
                std::cout << "SAVING..." << "\n";
                gb.saveGame();
                std::cout << "DONE" << "\n";
            }

           

            // fill 160x144 texture with display data and draw screen quad
            

            static int frameCount = 0;

            frameCount++;

            gb.cpu.tCycles = 0;

            glfwSwapBuffers(window);
            glfwPollEvents();

            //lastTime = currentTime;

           /* while ((glfwGetTime() - currentTime) < (1.0 / 73.5))
            {
                continue;
            }*/

            if (frameCount >= 60)
            {
                std::cout << "fps: " << 1 / (glfwGetTime() - currentTime) << "\n";
                frameCount = 0;
            }     
        }
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &gb.ppu.VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(gb.ppu.shaderProgram);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

void window_close_callback(GLFWwindow* window)
{
    if (gb.mmu.cartHasBattery)
    {
        gb.saveGame();
    }
}