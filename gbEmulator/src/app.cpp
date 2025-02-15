#include "app.h"

App::App()
{
    initWindow();
	gb = std::make_unique<GameBoy>(window);
}

void App::restart()
{
	std::string rom = gb->filePath;
    std::string saveFilePath;
    
    if (!gb->loadSaveRequested)
        saveFilePath = "";
    else
        saveFilePath = gb->saveFilePath;

    gb = std::make_unique<GameBoy>(window);
    gb->readRom(rom, saveFilePath);
   
    if (!gb->run() && !glfwWindowShouldClose(window))
    {
        restart();
    };
}

void App::initWindow()
{
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "gbEmulator", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
    }
    glfwMakeContextCurrent(window);

    glfwSetWindowUserPointer(window, this);
    glfwSetWindowCloseCallback(window, [](GLFWwindow* win)
    {
        static_cast<App*>(glfwGetWindowUserPointer(win))->onClose();
    });
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* win, int width, int height)
    {
        static_cast<App*>(glfwGetWindowUserPointer(win))->onResize(width, height);
    });

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
    }

    // disable vsync
    glfwSwapInterval(0);
    
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT - 19);

}

void App::onClose()
{
    if (gb->mmu.cartHasBattery)
    {
        gb->saveGame();
    }
}

void App::onResize(int width, int height)
{
    glViewport(0, 0, width, height - 19);
}

void App::run(std::string rom)
{
    if (!rom.empty())
    {
        gb->readRom(rom, "");
    }

    if (!gb->run() && !glfwWindowShouldClose(window))
    {
        restart();
    };

    glfwTerminate();
}