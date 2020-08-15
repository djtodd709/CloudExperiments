#pragma once

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
#include <common/controls.h>
#include <common/objloader.hpp>

static const GLfloat cloudVertices[] = {
		-1.0f, -1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		 1.0f, -1.0f, 0.0f,
		 1.0f,  1.0f, 0.0f
};

static bool windowChanged = false;

class Renderer {
public:
	Renderer();
	~Renderer();

	bool isRunning() { return !exitWindow; };

	void UpdateScene();
	void RenderScene();
protected:
	void Initialize();
	void UpdateCloudUniforms();
	void CreateNoiseTex();
	void RenderUI();
	void RenderMountain();
	void PrepareCloudTextures();
	void RenderClouds();
	void RenderComputeClouds();
	void UpdateResolution();

	GLFWwindow* window;
	bool exitWindow;

	bool inMenu;
	bool pausePress;
	int subMenu;
	bool fpsCount;
	bool paused;

	bool usingCompute;
	bool drawMountains;
	float mountainHeight;

	float timePassed;

	GLuint vertexArrayID;
	GLuint programID;
	GLuint currentCloudID;
	GLuint cloudFragmentID;
	GLuint cloudComputeID;
	GLuint passthroughID;
	GLuint worleyShaderID;
	

	GLuint bufferColourTex;
	GLuint bufferDepthTex;
	GLuint bufferFBO;

	GLuint matrixID;
	GLuint cloudMatrixID;

	GLuint texture;
	GLuint textureID;
	GLuint normTexture;
	GLuint normTextureID;

	GLuint worleyTex;
	GLuint worleyTexID;
	GLuint detailTex;
	GLuint detailTexID;
	GLuint bufferTexID;
	GLuint depthTexID;
	GLuint finalTex;
	GLuint finalTexID;

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
	float numStepsVal;
	GLuint numLightSteps;
	float numLightStepsVal;
	GLuint densityMult;
	float densityMultVal;
	GLuint densityOfst;
	float densityOfstVal;
	GLuint lightCol;
	vec3 lightColVal;
	GLuint lightDir;
	vec3 lightDirVal;
	GLuint baseTransmittance;
	float baseTransmittanceVal;
	GLuint skyCol;
	vec3 skyColVal;
	GLuint cloudCol;
	vec3 cloudColVal;
	GLuint cloudScale;
	vec3 cloudScaleVal;
	GLuint detailScale;
	float detailScaleVal;
	GLuint cloudMin;
	vec3 cloudMinVal;
	GLuint cloudMax;
	vec3 cloudMaxVal;
	GLuint cloudSpeed;
	vec3 cloudSpeedVal;
	GLuint detailSpeed;
	vec3 detailSpeedVal;
	GLuint optFactor;
	float optFactorVal;
	GLuint forwardScattering;
	float forwardScatteringVal;
	GLuint backScattering;
	float backScatteringVal;
	GLuint baseBrightness;
	float baseBrightnessVal;
	GLuint phaseFactor;
	float phaseFactorVal;

	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;

	glm::mat4 ProjectionMatrix;
	glm::mat4 ViewMatrix;
	glm::mat4 ModelMatrix;
	glm::mat4 MVP;
};