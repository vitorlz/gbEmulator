#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "renderingManager.h"
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <glad/glad.h>
#include <tinyfiledialogs.h>

#include "gb.h"

RenderingManager::RenderingManager(GameBoy& gb) : gb(gb) {}

void RenderingManager::init(GLFWwindow* window)
{
	createShaderProgram();
	createVAO();
	createDisplayTexture();
    initUI(window);
}


RenderingManager::~RenderingManager()
{
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(shader);
    glDeleteTextures(1, &displayTexture);
}

void RenderingManager::createDisplayTexture()
{
    glGenTextures(1, &displayTexture);
    glBindTexture(GL_TEXTURE_2D, displayTexture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 160, 144, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void RenderingManager::createShaderProgram()
{
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

    const char* vertexShaderSource = vertexCode.c_str();
    const char* fragmentShaderSource = fragmentCode.c_str();

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    shader = glCreateProgram();
    glAttachShader(shader, vertexShader);
    glAttachShader(shader, fragmentShader);
    glLinkProgram(shader);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void RenderingManager::createVAO()
{
    float vertices[] = {
        // positions   // texCoords
          -1.0f,  1.0f,  0.0f, 1.0f,
          -1.0f, -1.0f,  0.0f, 0.0f,
           1.0f, -1.0f,  1.0f, 0.0f,

          -1.0f,  1.0f,  0.0f, 1.0f,
           1.0f, -1.0f,  1.0f, 0.0f,
           1.0f,  1.0f,  1.0f, 1.0f
    };

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);

}

void RenderingManager::initUI(GLFWwindow* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void  RenderingManager::renderUI()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Load rom...")) 
            {   
                const char* filter[] = { "*.gb", "*.gbc"}; 
                const char* filePath = tinyfd_openFileDialog("Open ROM File", "", 2, filter, "ROM Files", 0);

                if (filePath)
                {
                    if (!gb.validRomLoaded)
                    {   
                        gb.readRom(filePath, "");
                    }
                    else
                    {
                        gb.restartRequested = true;
                        gb.filePath = filePath;
                    }
                }
                else 
                {
                    std::cout << "No file selected." << std::endl;
                }
            }
            if (ImGui::MenuItem("Load save file..."))
            { 
                const char* filePath = tinyfd_openFileDialog("Open Save File", "", 0, nullptr, "*", 0);

                if (filePath)
                {
                    gb.saveFilePath = filePath;
                    gb.restartRequested = true;
                    gb.loadSaveRequested = true;
                }
                else
                {
                    std::cout << "No file selected." << std::endl;
                }
            }
            if (ImGui::MenuItem("Save as..."))
            {
                std::string defaultName = gb.filePath + "_save";
                const char* filename = tinyfd_saveFileDialog("Save File As", defaultName.c_str(), 0, nullptr, NULL);

                if (filename != NULL) {
                    printf("Selected file: %s\n", filename);  
                    gb.saveFilePath = filename;
                    gb.saveGame();
                }
                else {
                    printf("No file selected.\n");
                }
            }
            if (ImGui::MenuItem("Save"))
            {
                gb.saveGame();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Speed"))
        {
            if (ImGui::MenuItem("1x"))
            {
                gb.fpsLimit = 1.0 / 60.0;
            }
            if (ImGui::MenuItem("2x"))
            {
                gb.fpsLimit = 1.0 / 120.0;
            }
            if (ImGui::MenuItem("4x"))
            {
                gb.fpsLimit = 1.0 / 240.0;
            }
            if (ImGui::MenuItem("MAX."))
            {
                gb.fpsLimit = 0.0;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void  RenderingManager::terminateUI()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}