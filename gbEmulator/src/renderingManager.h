#pragma once 

class GameBoy;

class RenderingManager
{
public:
	RenderingManager(GameBoy& gb);

	~RenderingManager();

	void init(GLFWwindow* window);

	unsigned int shader;
	unsigned int vao;
	unsigned int displayTexture;
	unsigned int vbo;

	void initUI(GLFWwindow* window);
	void renderUI();
	void terminateUI();

private:
	GameBoy& gb;
	void createShaderProgram();
	void createVAO();
	void createDisplayTexture();
};