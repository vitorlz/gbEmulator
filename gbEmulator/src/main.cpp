#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iomanip>
#include <iostream>
#include <limits>
#include <chrono>
#include <thread>

#include "JsonTest.h"

#include "gb.h"


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 320;
const unsigned int SCR_HEIGHT = 288;

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
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

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

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

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
    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);

    
    // create 160 * 144 texture for display buffer

    unsigned int displayTexture;

    glGenTextures(1, &displayTexture);
    glBindTexture(GL_TEXTURE_2D, displayTexture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 160, 144, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, 0);

    GameBoy gb;   
    std::string rom = argc > 1 ? argv[1] : "res/testroms/pitman.gb";
    gb.readRom(rom.c_str());

    //JsonTest jsonTest(gb);
    //jsonTest.RunAllTests();

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

            // std::cout << std::hex << "PC: " << (gb.cpu.PC - 1) << " opcode: " << std::hex << int(opcode) << "\n";
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

            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            // fill 160x144 texture with display data and draw screen quad
            glUseProgram(shaderProgram);
            glBindTexture(GL_TEXTURE_2D, displayTexture);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 160, 144, GL_RED, GL_UNSIGNED_BYTE, &gb.ppu.LCD[0]);

            glBindVertexArray(VAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            static int frameCount = 0;

            frameCount++;

            gb.cpu.tCycles = 0;

            glfwSwapBuffers(window);
            glfwPollEvents();

            //lastTime = currentTime;

            while ((glfwGetTime() - currentTime) < (1.0 / 73.5))
            {
                continue;
            }

            if (frameCount >= 60)
            {
                std::cout << "fps: " << 1 / (glfwGetTime() - currentTime) << "\n";
                frameCount = 0;
            }     
        }
    }

    

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

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