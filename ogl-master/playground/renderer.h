#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include <common/shader.hpp>
#include <common/texture.hpp>
#include <common/controls.hpp>
#include <common/objloader.hpp>

static const GLfloat cloudVertices[] = {
		-1.0f, -1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,
		 1.0f,  1.0f, 0.0f
};

class Renderer {
public:
	Renderer();
	~Renderer();

	bool isRunning() { return !exitWindow; };

	void UpdateScene();
	void RenderScene();
protected:
	void Initialize();
	void CreateNoiseTex();
	void RenderUI();
	void RenderMountain();
	void RenderClouds();

	GLFWwindow* window;
	bool exitWindow;

	bool inMenu;
	bool pausePress;
	int subMenu;
	bool fpsCount;
	bool paused;

	GLuint VertexArrayID;
	GLuint programID;
	GLuint cloudID;
	GLuint worleyShaderID;

	GLuint bufferColourTex;
	GLuint bufferDepthTex;
	GLuint bufferFBO;

	GLuint MatrixID;
	GLuint cloudMatrixID;

	GLuint Texture;
	GLuint TextureID;
	GLuint normTexture;
	GLuint normTextureID;

	GLuint worleyTex;
	GLuint detailTex;
	GLuint worleyTexID;
	GLuint detailTexID;
	GLuint bufferTexID;
	GLuint depthTexID;

	GLuint cloudVertexbuffer;
	GLuint vertexbuffer;
	GLuint uvbuffer;

	GLuint iTime;
	GLuint iResolution;
	GLuint cloudResolution;
	GLuint cameraPos;
	GLuint cameraDir;
	GLuint cameraRight;

	GLuint numSteps;
	GLuint numLightSteps;
	GLuint densityMult;
	GLuint densityOfst;
	GLuint lightCol;
	GLuint lightDir;
	float numStepsVal;
	float numLightStepsVal;
	float densityMultVal;
	float densityOfstVal;
	vec3 lightColVal;
	vec3 lightDirVal;

	float timePassed;

	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals; // Won't be used at the moment.

	glm::mat4 ProjectionMatrix;
	glm::mat4 ViewMatrix;
	glm::mat4 ModelMatrix;
	glm::mat4 MVP;
};