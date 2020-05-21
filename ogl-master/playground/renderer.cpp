#include "renderer.h"

Renderer::Renderer() {
	exitWindow = false;

	// Initialise GLFW
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		getchar();
		exitWindow = true;
		return;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(WINDOWWIDTH, WINDOWHEIGHT, "Cloud Experiments", NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
		getchar();
		glfwTerminate();
		exitWindow = true;
		return;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		exitWindow = true;
		return;
	}

	Initialize();
	return;
}

Renderer::~Renderer() {
	// Cleanup VBO
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteBuffers(1, &uvbuffer);
	glDeleteProgram(programID);
	glDeleteTextures(1, &Texture);
	glDeleteVertexArrays(1, &VertexArrayID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();
}

void Renderer::Initialize() {
	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	// Hide the mouse and enable unlimited mouvement
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Set the mouse at the center of the screen
	glfwPollEvents();
	glfwSetCursorPos(window, WINDOWWIDTH / 2, WINDOWHEIGHT / 2);

	// background
	glClearColor(0.7f, 0.5f, 0.9f, 0.0f);

	//Create Framebuffers
	glGenTextures(1, &bufferColourTex);
	glBindTexture(GL_TEXTURE_2D, bufferColourTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, WINDOWWIDTH, WINDOWHEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glGenTextures(1, &bufferDepthTex);
	glBindTexture(GL_TEXTURE_2D, bufferDepthTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, WINDOWWIDTH, WINDOWHEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	glGenFramebuffers(1, &bufferFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, bufferDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE || !bufferDepthTex || !bufferColourTex) {
		return;
	}

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	glPatchParameteri(GL_PATCH_VERTICES, 3);

	// Create and compile our GLSL program from the shaders
	programID = LoadShaders("Shaders/texturedVS.glsl", "Shaders/mountainFS.glsl", "Shaders/texturedTCS.glsl", "Shaders/texturedTES.glsl");
	cloudID = LoadShaders("Shaders/PassthroughVS.glsl", "Shaders/WorleyDensityFS.glsl");
	worleyShaderID = LoadComputeShader("Shaders/WorleyCS.glsl");

	// Get a handle for our "MVP" uniform
	MatrixID = glGetUniformLocation(programID, "MVP");

	// Get a handle for our "MVP" uniform
	cloudMatrixID = glGetUniformLocation(cloudID, "MVP");


	// Load the texture using any two methods
	//GLuint Texture = loadBMP_custom("uvtemplate.bmp");
	Texture = loadImage("Textures/heightmap.png");
	normTexture = loadImage("Textures/terrainNormals.png");

	// Get a handle for our "myTextureSampler" uniform
	TextureID = glGetUniformLocation(programID, "heightMap");
	normTextureID = glGetUniformLocation(programID, "normalMap");

	CreateNoiseTex();

	// Read our .obj file
	bool res = loadOBJ("Models/terrain.obj", vertices, uvs, normals);

	glGenBuffers(1, &cloudVertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, cloudVertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cloudVertices), cloudVertices, GL_STATIC_DRAW);

	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

	glGenBuffers(1, &uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);


	iTime = glGetUniformLocation(cloudID, "iTime");
	iResolution = glGetUniformLocation(programID, "iResolution");
	cloudResolution = glGetUniformLocation(cloudID, "iResolution");
	cameraPos = glGetUniformLocation(cloudID, "camPos");
	cameraDir = glGetUniformLocation(cloudID, "camDir");
	cameraRight = glGetUniformLocation(cloudID, "camRight");

	numSteps = glGetUniformLocation(cloudID, "numSteps");
	numLightSteps = glGetUniformLocation(cloudID, "numLightSteps");
	densityMult = glGetUniformLocation(cloudID, "densityMult");
	densityOfst = glGetUniformLocation(cloudID, "densityOfst");

	glUseProgram(programID);
	glUniform2f(iResolution, WINDOWWIDTH, WINDOWHEIGHT);
	glUseProgram(cloudID);
	glUniform2f(cloudResolution, WINDOWWIDTH, WINDOWHEIGHT);
	glUniform1f(glGetUniformLocation(cloudID, "zNear"), 0.1f);
	glUniform1f(glGetUniformLocation(cloudID, "zFar"), 100.0f);

	numStepsVal = 80.0f;
	numLightStepsVal = 8.0f;
	densityMultVal = 12.0f;
	densityOfstVal = 6.5f;
	glUniform1f(numSteps, numStepsVal);
	glUniform1f(numLightSteps, numLightStepsVal);
	glUniform1f(densityMult, densityMultVal);
	glUniform1f(densityOfst, densityOfstVal);

	glUseProgram(0);

	timePassed = 0;
	return;
}

void Renderer::CreateNoiseTex() {
	glUseProgram(worleyShaderID);
	glUniform1i(glGetUniformLocation(worleyShaderID, "destTex"), 0);
	glUniform1i(glGetUniformLocation(worleyShaderID, "detailTex"), 1);

	glGenTextures(1, &worleyTex);
	glGenTextures(1, &detailTex);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, worleyTex);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, 128, 128, 128, 0, GL_RGBA, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glBindImageTexture(0, worleyTex, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, detailTex);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, 64, 64, 64, 0, GL_RGBA, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glBindImageTexture(1, detailTex, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glDispatchCompute(128 / 8, 128 / 8, 128 / 8);

	worleyTexID = glGetUniformLocation(cloudID, "worleyTex");
	detailTexID = glGetUniformLocation(cloudID, "detailTex");
	bufferTexID = glGetUniformLocation(cloudID, "bufferTex");
	depthTexID = glGetUniformLocation(cloudID, "depthTex");

	return;
}

void Renderer::UpdateScene() {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS ||
		glfwWindowShouldClose(window) != 0) {
		exitWindow = true;
		return;
	}

	// Compute the MVP matrix from keyboard and mouse input
	computeMatricesFromInputs(window);
	ProjectionMatrix = getProjectionMatrix();
	ViewMatrix = getViewMatrix();

	if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) {
		glUniform1f(numSteps, ++numStepsVal);
		printf("numSteps: %1.0f \n", numStepsVal);
	}
	if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
		if (numStepsVal < 1.0f) numStepsVal = 1.0f;
		glUniform1f(numSteps, --numStepsVal);
		printf("numSteps: %1.0f \n", numStepsVal);
	}
	if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
		glUniform1f(numLightSteps, ++numLightStepsVal);
		printf("numLightSteps: %1.0f \n", numLightStepsVal);
	}
	if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
		if (numLightStepsVal < 1.0f) numLightStepsVal = 1.0f;
		glUniform1f(numLightSteps, --numLightStepsVal);
		printf("numLightSteps: %1.0f \n", numLightStepsVal);
	}

	if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) {
		densityMultVal += 0.1f;
		glUniform1f(densityMult, densityMultVal);
		printf("densityMult: %2.1f \n", densityMultVal);
	}
	if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) {
		densityMultVal -= 0.1f;
		glUniform1f(densityMult, densityMultVal);
		printf("densityMult: %2.1f \n", densityMultVal);
	}
	if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) {
		densityOfstVal += 0.1f;
		glUniform1f(densityOfst, densityOfstVal);
		printf("densityOfst: %2.1f \n", densityOfstVal);
	}
	if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
		densityOfstVal -= 0.1f;
		glUniform1f(densityOfst, densityOfstVal);
		printf("densityOfst: %2.1f \n", densityOfstVal);
	}

	timePassed += 0.16f;
	return;
}

void Renderer::RenderScene() {
	// Render to our framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	// Clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	RenderMountain();

	// Render to the screen
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// Clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	RenderClouds();
	// Swap buffers
	glfwSwapBuffers(window);
	glfwPollEvents();
	return;
}

void Renderer::RenderMountain() {
	// Use our shader
	glUseProgram(programID);
	glUniform1f(glGetUniformLocation(programID, "iTime"), timePassed);

	ModelMatrix = glm::translate(
		glm::scale(glm::mat4(1.0),glm::vec3(20.0,15.0,20.0)),
		glm::vec3(0.0,-0.5,0.0));
	MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

	// Send our transformation to the currently bound shader, 
	// in the "MVP" uniform
	glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

	// Bind our texture in Texture Unit 0
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, Texture);
	// Set our "myTextureSampler" sampler to use Texture Unit 0
	glUniform1i(TextureID, 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, normTexture);
	// Set our "myTextureSampler" sampler to use Texture Unit 1
	glUniform1i(normTextureID, 1);

	// 1rst attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
	);

	// 2nd attribute buffer : UVs
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glVertexAttribPointer(
		1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
		2,                                // size : U+V => 2
		GL_FLOAT,                         // type
		GL_FALSE,                         // normalized?
		0,                                // stride
		(void*)0                          // array buffer offset
	);

	// Draw the triangle !
	glDrawArrays(GL_PATCHES, 0, vertices.size());

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	return;
}

void Renderer::RenderClouds() {

	glEnable(GL_BLEND);
	
	// Use our shader
	glUseProgram(cloudID);
	glUniform1f(iTime, timePassed);

	// Bind our texture in Texture Unit 0
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, worleyTex);
	// Set our "myTextureSampler" sampler to use Texture Unit 0
	glUniform1i(worleyTexID, 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, bufferColourTex);
	glUniform1i(bufferTexID, 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, bufferDepthTex);
	glUniform1i(depthTexID, 2);

	// Bind our texture in Texture Unit
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_3D, detailTex);
	glUniform1i(detailTexID, 3);

	glUniform3fv(cameraPos, 1, &getCameraPosition()[0]);
	glUniform3fv(cameraDir, 1, &getCameraDirection()[0]);
	glUniform3fv(cameraRight, 1, &getCameraRight()[0]);

	// 1rst attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, cloudVertexbuffer);
	glVertexAttribPointer(
		0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,                  // size
		GL_FLOAT,           // type
		GL_FALSE,           // normalized?
		0,                  // stride
		(void*)0            // array buffer offset
	);

	// Draw the triangle !
	glDrawArrays(GL_TRIANGLES, 0, sizeof(cloudVertices));

	glDisableVertexAttribArray(0);

	glDisable(GL_BLEND);
}
