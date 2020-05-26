#include "renderer.h"

Renderer::Renderer() {
	exitWindow = false;

	inMenu = false;
	subMenu = 0;
	fpsCount = true;
	paused = false;

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

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 430");


	Initialize();
	return;
}

Renderer::~Renderer() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

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
	glClearColor(0.5f, 0.5f, 0.9f, 0.0f);

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

	lightCol = glGetUniformLocation(cloudID, "lightCol");
	lightDir = glGetUniformLocation(cloudID, "lightDir");

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
	lightColVal = vec3(1.0f);
	lightDirVal = normalize(vec3(0.5f, 1.0f, 0.5f));
	glUniform1f(numSteps, numStepsVal);
	glUniform1f(numLightSteps, numLightStepsVal);
	glUniform1f(densityMult, densityMultVal);
	glUniform1f(densityOfst, densityOfstVal);
	glUniform3fv(lightCol, 1, (float*)&lightColVal[0]);
	glUniform3fv(lightDir, 1, (float*)&lightDirVal[0]);

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
	computeMatricesFromInputs(window, inMenu);
	ProjectionMatrix = getProjectionMatrix();
	ViewMatrix = getViewMatrix();

	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
		if (!pausePress) {
			pausePress = true;
			if (inMenu) {
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				glfwSetCursorPos(window, WINDOWWIDTH / 2, WINDOWHEIGHT / 2);
				inMenu = false;
			}
			else {
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				inMenu = true;
			}
		}
	}
	else { pausePress = false; }

	if (subMenu == 1) {
		glUniform1f(densityMult, densityMultVal);
		glUniform1f(densityOfst, densityOfstVal);
	}
	if (subMenu == 2) {
		glUniform3fv(lightCol, 1, (float*)&lightColVal[0]);
		glUniform3fv(lightDir, 1, (float*)&normalize(lightDirVal)[0]);
		numStepsVal = max(0.0f, round(numStepsVal));
		glUniform1f(numSteps, numStepsVal);
		numLightStepsVal = max(0.0f, round(numLightStepsVal));
		glUniform1f(numLightSteps, numLightStepsVal);
	}
	
	if (!paused) {
		timePassed += ImGui::GetIO().DeltaTime;
	}
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

	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	RenderUI();
	ImGui::Render();
	// Clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	RenderClouds();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	// Swap buffers
	glfwSwapBuffers(window);
	glfwPollEvents();
	return;
}

void Renderer::RenderUI() {
	ImGuiWindowFlags window_flags = 0;
	window_flags |= ImGuiWindowFlags_NoTitleBar;
	window_flags |= ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoResize;
	window_flags |= ImGuiWindowFlags_NoCollapse;
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	if (inMenu) {
		if (subMenu == 0) {
			ImGui::SetNextWindowSize(ImVec2(340.0f, 150.0f));
		}
		else if (subMenu == 1) {
			ImGui::SetNextWindowSize(ImVec2(340.0f, 150.0f));
		}
		else if (subMenu == 2) {
			ImGui::SetNextWindowSize(ImVec2(360.0f, 300.0f));
		}



		ImGui::Begin("Options", (bool*)0, window_flags);
		ImGui::Text("Press P to close settings");
		if (ImGui::Button("General"))
			subMenu = 0;
		ImGui::SameLine();
		if (ImGui::Button("Structure"))
			subMenu = 1;
		ImGui::SameLine();
		if (ImGui::Button("Lighting"))
			subMenu = 2;

		if (subMenu == 0) {
			ImGui::Checkbox("Show FPS", &fpsCount);
			ImGui::Checkbox("Paused", &paused);
		}
		else if (subMenu == 1) {
			ImGui::Text("\nDensity Texture Sampling");
			ImGui::SliderFloat("Multiplier", &densityMultVal, -10.0f, 20.0f, "%2.1f");
			ImGui::SliderFloat("Offset", &densityOfstVal, -10.0f, 20.0f, "%2.1f");
		}
		else if (subMenu == 2) {
			ImGui::Text("\nLight Properties");
			ImGui::ColorEdit3("Colour", (float*)&lightColVal,
				ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoInputs
				);
			ImGui::SliderFloat3("Direction", (float*)&lightDirVal, -1.0f, 1.0f);
			ImGui::Text("\nRaymarching Steps");
			ImGui::SliderFloat("Camera -> Cloud", &numStepsVal, 0.0f, 150.0f, "%.0f");
			ImGui::SliderFloat("Cloud -> Light", &numLightStepsVal, 0.0f, 50.0f, "%.0f");
		}
	}
	else {
		ImGui::SetNextWindowSize(ImVec2(200.0f, 30.0f));
		ImGui::Begin("Options", (bool*)0, window_flags);
		ImGui::Text("Press P to adjust settings");
	}
	ImGui::End();

	if (fpsCount) {
		ImGui::SetNextWindowPos(ImVec2(WINDOWWIDTH-160, 0));
		ImGui::SetNextWindowSize(ImVec2(160.0f, 60.0f));
		ImGui::Begin("FPS", (bool*)0, window_flags);
		ImGui::Text("Application average \n%.3f ms/frame \n(%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
	}
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
