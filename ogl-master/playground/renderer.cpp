#include "renderer.h"

Renderer::Renderer() {
	exitWindow = false;

	inMenu = false;
	subMenu = 0;
	fpsCount = true;
	paused = false;

	usingCompute = false;

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
	window = glfwCreateWindow(WINDOWWIDTH, WINDOWHEIGHT, "Cloud Playground", NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "Failed to open GLFW window.\n");
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

	// Initialize ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 430");

	glfwSwapInterval(0);

	Initialize();
	return;
}

Renderer::~Renderer() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	// Cleanup Textures and Buffers
	glDeleteTextures(1, &worleyTex);
	glDeleteTextures(1, &detailTex);

	glDeleteTextures(1, &bufferColourTex);
	glDeleteTextures(1, &bufferDepthTex);
	glDeleteFramebuffers(1, &bufferFBO);

	glDeleteTextures(1, &finalTex);

	glDeleteBuffers(1, &vertexbuffer);
	glDeleteBuffers(1, &uvbuffer);
	glDeleteBuffers(1, &cloudVertexbuffer);
	glDeleteProgram(programID);
	glDeleteTextures(1, &texture);
	glDeleteVertexArrays(1, &vertexArrayID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();
}

void WindowResize(GLFWwindow* window, int width, int height)
{
	WINDOWWIDTH = width;
	WINDOWHEIGHT = height;
	windowChanged = true;
}

void Renderer::Initialize() {
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwSetWindowSizeCallback(window, WindowResize);

	glfwPollEvents();
	glfwSetCursorPos(window, WINDOWWIDTH / 2, WINDOWHEIGHT / 2);

	glClearColor(0.5f, 0.5f, 0.5f, 0.0f);

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

	glGenVertexArrays(1, &vertexArrayID);
	glBindVertexArray(vertexArrayID);

	glPatchParameteri(GL_PATCH_VERTICES, 3);

	// Create and compile shaders
	programID = LoadShaders("Shaders/TexturedVS.glsl", "Shaders/MountainFS.glsl", "Shaders/TexturedTCS.glsl", "Shaders/TexturedTES.glsl");
	cloudFragmentID = LoadShaders("Shaders/PassthroughVS.glsl", "Shaders/CloudDensityFS.glsl");
	passthroughID = LoadShaders("Shaders/PassthroughTexVS.glsl", "Shaders/TexturedFS.glsl");
	worleyShaderID = LoadComputeShader("Shaders/WorleyCS.glsl");
	cloudComputeID = LoadComputeShader("Shaders/CloudDensityCS.glsl");

	if (usingCompute) {
		currentCloudID = cloudComputeID;
	}
	else {
		currentCloudID = cloudFragmentID;
	}

	matrixID = glGetUniformLocation(programID, "MVP");
	cloudMatrixID = glGetUniformLocation(currentCloudID, "MVP");

	texture = loadImage("Textures/heightmap.png");
	normTexture = loadImage("Textures/terrainNormals.png");

	textureID = glGetUniformLocation(programID, "heightMap");
	normTextureID = glGetUniformLocation(programID, "normalMap");

	//Generate compute texture
	glGenTextures(1, &finalTex);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, finalTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WINDOWWIDTH, WINDOWHEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);

	CreateNoiseTex();

	worleyTexID = glGetUniformLocation(currentCloudID, "worleyTex");
	detailTexID = glGetUniformLocation(currentCloudID, "detailTex");
	bufferTexID = glGetUniformLocation(currentCloudID, "bufferTex");
	depthTexID = glGetUniformLocation(currentCloudID, "depthTex");

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

	//Get handlers for cloud shader uniforms
	iTime = glGetUniformLocation(currentCloudID, "iTime");
	iResolution = glGetUniformLocation(programID, "iResolution");
	cloudResolution = glGetUniformLocation(currentCloudID, "iResolution");
	cameraPos = glGetUniformLocation(currentCloudID, "camPos");
	cameraDir = glGetUniformLocation(currentCloudID, "camDir");
	cameraRight = glGetUniformLocation(currentCloudID, "camRight");

	numSteps = glGetUniformLocation(currentCloudID, "numSteps");
	numLightSteps = glGetUniformLocation(currentCloudID, "numLightSteps");
	densityMult = glGetUniformLocation(currentCloudID, "densityMult");
	densityOfst = glGetUniformLocation(currentCloudID, "densityOfst");

	baseTransmittance = glGetUniformLocation(currentCloudID, "baseTransmittance");

	lightCol = glGetUniformLocation(currentCloudID, "lightCol");
	lightDir = glGetUniformLocation(currentCloudID, "lightDir");

	skyCol = glGetUniformLocation(currentCloudID, "skyCol");
	cloudCol = glGetUniformLocation(currentCloudID, "cloudCol");

	forwardScattering = glGetUniformLocation(currentCloudID, "forwardScattering");
	backScattering = glGetUniformLocation(currentCloudID, "backScattering");
	baseBrightness = glGetUniformLocation(currentCloudID, "baseBrightness");
	phaseFactor = glGetUniformLocation(currentCloudID, "phaseFactor");

	cloudMin = glGetUniformLocation(currentCloudID, "cloudMin");
	cloudMax = glGetUniformLocation(currentCloudID, "cloudMax");

	//Set initial values of shader uniforms
	glUseProgram(programID);
	glUniform2f(iResolution, WINDOWWIDTH, WINDOWHEIGHT);
	glUseProgram(currentCloudID);
	glUniform2f(glGetUniformLocation(cloudFragmentID, "iResolution"), WINDOWWIDTH, WINDOWHEIGHT);
	glUniform2f(glGetUniformLocation(cloudComputeID, "iResolution"), WINDOWWIDTH, WINDOWHEIGHT);
	glUniform1f(glGetUniformLocation(currentCloudID, "zNear"), 0.1f);
	glUniform1f(glGetUniformLocation(currentCloudID, "zFar"), 100.0f);

	numStepsVal = 0.25f;
	numLightStepsVal = 8.0f;
	densityMultVal = 12.0f;
	densityOfstVal = 0.7f;
	baseTransmittanceVal = 0.25f;
	lightColVal = vec3(1.0f);
	lightDirVal = normalize(vec3(0.5f, 1.0f, 0.5f));
	cloudScaleVal = vec3(2.0f,1.0f,2.0f);
	detailScaleVal = 1.0f;
	cloudSpeedVal = vec3(0.01f,0.0f,0.007f);
	detailSpeedVal = vec3(-0.008f, 0.0f, 0.005f);
	optFactorVal = 0.4f;

	cloudMinVal = vec3(-20.0, 0, -20.0);
	cloudMaxVal = vec3(20.0, 8.0, 20.0);

	skyColVal = vec3(0.58f, 0.66f, 0.81f);
	cloudColVal = vec3(1.0f);
	
	forwardScatteringVal = 0.6f;
	backScatteringVal = 0.5f;
	baseBrightnessVal = 1.0f;
	phaseFactorVal = 0.9f;

	glUseProgram(0);
	UpdateCloudUniforms();

	//Setup non-cloud initial values
	mountainHeight = -0.5f;
	drawMountains = true;
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

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_3D, worleyTex);
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_3D, detailTex);

	return;
}

void Renderer::UpdateCloudUniforms() {
	//Get handlers for correct shader (fragment/compute)
	glUseProgram(currentCloudID);
	cloudMatrixID = glGetUniformLocation(currentCloudID, "MVP");

	iTime = glGetUniformLocation(currentCloudID, "iTime");
	cloudResolution = glGetUniformLocation(currentCloudID, "iResolution");
	cameraPos = glGetUniformLocation(currentCloudID, "camPos");
	cameraDir = glGetUniformLocation(currentCloudID, "camDir");
	cameraRight = glGetUniformLocation(currentCloudID, "camRight");

	numSteps = glGetUniformLocation(currentCloudID, "numSteps");
	numLightSteps = glGetUniformLocation(currentCloudID, "numLightSteps");
	densityMult = glGetUniformLocation(currentCloudID, "densityMult");
	densityOfst = glGetUniformLocation(currentCloudID, "densityOfst");

	baseTransmittance = glGetUniformLocation(currentCloudID, "baseTransmittance");

	lightCol = glGetUniformLocation(currentCloudID, "lightCol");
	lightDir = glGetUniformLocation(currentCloudID, "lightDir");

	skyCol = glGetUniformLocation(currentCloudID, "skyCol");
	cloudCol = glGetUniformLocation(currentCloudID, "cloudCol");

	cloudScale = glGetUniformLocation(currentCloudID, "cloudScale");
	detailScale = glGetUniformLocation(currentCloudID, "detailScale");

	cloudSpeed = glGetUniformLocation(currentCloudID, "cloudSpeed");
	detailSpeed = glGetUniformLocation(currentCloudID, "detailSpeed");
	optFactor = glGetUniformLocation(currentCloudID, "optFactor");

	forwardScattering = glGetUniformLocation(currentCloudID, "forwardScattering");
	backScattering = glGetUniformLocation(currentCloudID, "backScattering");
	baseBrightness = glGetUniformLocation(currentCloudID, "baseBrightness");
	phaseFactor = glGetUniformLocation(currentCloudID, "phaseFactor");

	glUniform2f(glGetUniformLocation(currentCloudID, "iResolution"), WINDOWWIDTH, WINDOWHEIGHT);
	glUniform1f(glGetUniformLocation(currentCloudID, "zNear"), 0.1f);
	glUniform1f(glGetUniformLocation(currentCloudID, "zFar"), 100.0f);

	worleyTexID = glGetUniformLocation(currentCloudID, "worleyTex");
	detailTexID = glGetUniformLocation(currentCloudID, "detailTex");
	bufferTexID = glGetUniformLocation(currentCloudID, "bufferTex");
	depthTexID = glGetUniformLocation(currentCloudID, "depthTex");

	//Set uniform values
	glUniform1i(worleyTexID, 4);
	glUniform1i(detailTexID, 5);

	glUniform1f(numSteps, numStepsVal);
	glUniform1f(numLightSteps, numLightStepsVal);
	glUniform1f(densityMult, densityMultVal);
	glUniform1f(densityOfst, densityOfstVal);
	glUniform1f(baseTransmittance, baseTransmittanceVal);
	glUniform3fv(lightCol, 1, (float*)&lightColVal[0]);
	glUniform3fv(lightDir, 1, (float*)&normalize(lightDirVal)[0]);
	glUniform3fv(cloudScale, 1, (float*)&(1.0f / cloudScaleVal)[0]);
	glUniform1f(detailScale, detailScaleVal);
	glUniform3fv(cloudSpeed, 1, (float*)&cloudSpeedVal[0]);
	glUniform3fv(detailSpeed, 1, (float*)&detailSpeedVal[0]);
	glUniform1f(optFactor, optFactorVal);
	glUniform3fv(skyCol, 1, (float*)&skyColVal[0]);
	glUniform3fv(cloudCol, 1, (float*)&cloudColVal[0]);
	glUniform1f(forwardScattering, forwardScatteringVal);
	glUniform1f(backScattering, backScatteringVal);
	glUniform1f(baseBrightness, baseBrightnessVal);
	glUniform1f(phaseFactor, phaseFactorVal);
	glUniform3fv(cloudMin, 1, (float*)&cloudMinVal[0]);
	glUniform3fv(cloudMax, 1, (float*)&cloudMaxVal[0]);

	glUseProgram(0);
}

void Renderer::UpdateResolution() {
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	//Recreate Framebuffers
	glDeleteTextures(1, &bufferColourTex);
	glDeleteTextures(1, &bufferDepthTex);

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

	glDeleteFramebuffers(1, &bufferFBO);
	glGenFramebuffers(1, &bufferFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, bufferDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE || !bufferDepthTex || !bufferColourTex) {
		return;
	}

	//Generate compute texture
	glDeleteTextures(1, &finalTex);
	glGenTextures(1, &finalTex);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, finalTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WINDOWWIDTH, WINDOWHEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);

	glUniform2f(glGetUniformLocation(cloudFragmentID, "iResolution"), WINDOWWIDTH, WINDOWHEIGHT);
	glUniform2f(glGetUniformLocation(cloudComputeID, "iResolution"), WINDOWWIDTH, WINDOWHEIGHT);
}

void Renderer::UpdateScene() {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS ||
		glfwWindowShouldClose(window) != 0) {
		exitWindow = true;
		return;
	}

	if (windowChanged) {
		windowChanged = false;
		UpdateResolution();
		UpdateCloudUniforms();
	}

	glUseProgram(currentCloudID);
	// Compute the MVP matrix from keyboard and mouse input
	computeMatricesFromInputs(window, inMenu);
	ProjectionMatrix = getProjectionMatrix();
	ViewMatrix = getViewMatrix();

	//Handle settings menu
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

	//Update results in realtime
	if (subMenu == 1) {
		glUniform1f(densityMult, densityMultVal);
		glUniform1f(densityOfst, densityOfstVal);
		glUniform3fv(cloudScale, 1, (float*)&(1.0f/cloudScaleVal)[0]);
		glUniform1f(detailScale, detailScaleVal);
		glUniform3fv(cloudSpeed, 1, (float*)&cloudSpeedVal[0]);
		glUniform3fv(detailSpeed, 1, (float*)&detailSpeedVal[0]);
		glUniform3fv(cloudMin, 1, (float*)&cloudMinVal[0]);
		glUniform3fv(cloudMax, 1, (float*)&cloudMaxVal[0]);
	}
	if (subMenu == 2) {
		glUniform3fv(lightCol, 1, (float*)&lightColVal[0]);
		glUniform3fv(lightDir, 1, (float*)&normalize(lightDirVal)[0]);
		numStepsVal = max(0.01f, numStepsVal);
		glUniform1f(numSteps, numStepsVal);
		numLightStepsVal = max(0.0f, round(numLightStepsVal));
		glUniform1f(numLightSteps, numLightStepsVal);
		glUniform1f(optFactor, optFactorVal);
		glUniform3fv(skyCol, 1, (float*)&skyColVal[0]);
		glUniform3fv(cloudCol, 1, (float*)&cloudColVal[0]);
		glUniform1f(baseTransmittance, baseTransmittanceVal);
		glUniform1f(forwardScattering, forwardScatteringVal);
		glUniform1f(backScattering, backScatteringVal);
		glUniform1f(baseBrightness, baseBrightnessVal);
		glUniform1f(phaseFactor, phaseFactorVal);
	}
	
	if (!paused) {
		timePassed += ImGui::GetIO().DeltaTime;
	}
	glUseProgram(0);
	return;
}

void Renderer::RenderScene() {
	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (drawMountains) {
		RenderMountain();
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	RenderUI();
	ImGui::Render();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (usingCompute) {
		RenderComputeClouds();
	}
	else {
		RenderClouds();
	}

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
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
		//Setup UI size depending on submenu
		if (subMenu == 0) {
			ImGui::SetNextWindowSize(ImVec2(400.0f, 270.0f));
		}
		else if (subMenu == 1) {
			ImGui::SetNextWindowSize(ImVec2(400.0f, 360.0f));
		}
		else if (subMenu == 2) {
			ImGui::SetNextWindowSize(ImVec2(420.0f, 360.0f));
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
			ImGui::Text("\n");
			ImGui::Checkbox("Show FPS", &fpsCount);
			ImGui::Checkbox("Paused", &paused);
			if (ImGui::Button("Toggle Shader")) {
				usingCompute = !usingCompute;
				if (usingCompute) {
					currentCloudID = cloudComputeID;
				}
				else {
					currentCloudID = cloudFragmentID;
				}
				UpdateCloudUniforms();
			}
			ImGui::SameLine();
			if (usingCompute) {
				ImGui::Text("Current Shader: Compute");
			}
			else {
				ImGui::Text("Current Shader: Fragment");
			}

			ImGui::Text("\n");
			ImGui::Checkbox("Draw Mountains", &drawMountains);
			ImGui::SliderFloat("Mountain Height", &mountainHeight, -3.0f, 0.0f, "%2.1f");

			ImGui::Text("\n");
			if (ImGui::Button("View 1")) {
				paused = true;
				timePassed = 0;
				setCameraPosition(vec3(0, 5, 0));
				setCameraDirection(0,0);
				UpdateCloudUniforms();
			}
			ImGui::SameLine();
			if (ImGui::Button("View 2")) {
				paused = true;
				timePassed = 0;
				setCameraPosition(vec3(6, 0, 4));
				setCameraDirection(pi<float>() / 8.0f, -5.0f*pi<float>()/8.0f);
				UpdateCloudUniforms();
			}
			ImGui::SameLine();
			if (ImGui::Button("View 3")) {
				paused = true;
				timePassed = 0;
				setCameraPosition(vec3(35, 40, 15));
				setCameraDirection(-2.0f*pi<float>() / 8.0f, -5.0f * pi<float>() / 8.0f);
				UpdateCloudUniforms();
			}

			if (ImGui::Button("Clouds 1")) {
				paused = true;
				timePassed = 0;
				detailScaleVal = 1.0f;
				densityMultVal = 8.2f;
				densityOfstVal = 0.7f;
				UpdateCloudUniforms();
			}
			ImGui::SameLine();
			if (ImGui::Button("Clouds 2")) {
				paused = true;
				timePassed = 0;
				detailScaleVal = 0.5f;
				densityMultVal = -8.0f;
				densityOfstVal = 0.7f;
				UpdateCloudUniforms();
			}
			ImGui::SameLine();
			if (ImGui::Button("Clouds 3")) {
				paused = true;
				timePassed = 0;
				detailScaleVal = 0.6f;
				densityMultVal = 4.0f;
				densityOfstVal = 0.4f;
				UpdateCloudUniforms();
			}
		}
		else if (subMenu == 1) {
			ImGui::Text("\nCloud Shape");
			ImGui::SliderFloat3("Scale", (float*)&cloudScaleVal, 0.0f, 5.0f);
			ImGui::SliderFloat("Detail Scale", &detailScaleVal, 0.0f, 5.0f, "%2.1f");
			ImGui::Text("\nCloud Speed");
			ImGui::SliderFloat3("Main", (float*)&cloudSpeedVal, -0.05f, 0.05f);
			ImGui::SliderFloat3("Detail", (float*)&detailSpeedVal, -0.05f, 0.05f);
			ImGui::Text("\nDensity Texture Sampling");
			ImGui::SliderFloat("Multiplier", &densityMultVal, -10.0f, 20.0f, "%2.1f");
			ImGui::SliderFloat("Offset", &densityOfstVal, 0.0f, 1.0f, "%3.2f");
			ImGui::Text("\nCloud Boundaries");
			ImGui::SliderFloat3("Minimum", (float*)&cloudMinVal, -50.0f, 0.0f);
			ImGui::SliderFloat3("Maximum", (float*)&cloudMaxVal, 0.0f, 50.0f);
		}
		else if (subMenu == 2) {
			ImGui::Text("\nProperties");
			ImGui::ColorEdit3("Sun Colour", (float*)&lightColVal,
				ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoInputs
				);
			ImGui::SameLine();
			ImGui::ColorEdit3("Cloud Colour", (float*)&cloudColVal,
				ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoInputs
			);
			ImGui::SameLine();
			ImGui::ColorEdit3("Sky Colour", (float*)&skyColVal,
				ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoInputs
			);
			ImGui::SliderFloat3("Direction", (float*)&lightDirVal, -1.0f, 1.0f);
			ImGui::SliderFloat("Base Transmittance", &baseTransmittanceVal, 0.0f, 1.0f, "%3.2f");

			ImGui::Text("\nRaymarching Step Size");
			ImGui::SliderFloat("Camera -> Cloud", &numStepsVal, 0.01f, 1.0f, "%5.4f");
			ImGui::SliderFloat("Cloud -> Light", &numLightStepsVal, 0.0f, 50.0f, "%.0f");

			ImGui::Text("\n");
			ImGui::SliderFloat("Step Optimization", &optFactorVal, 0.0f, 1.0f, "%3.2f");
			ImGui::SliderFloat("Forward Scattering", &forwardScatteringVal, 0.0f, 1.0f, "%3.2f");
			ImGui::SliderFloat("Back Scattering", &backScatteringVal, 0.0f, 1.0f, "%3.2f");
			ImGui::SliderFloat("Base Brightness", &baseBrightnessVal, 0.0f, 1.0f, "%3.2f");
			ImGui::SliderFloat("Phase Factor", &phaseFactorVal, 0.0f, 1.0f, "%3.2f");
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
	glUseProgram(programID);
	glUniform1f(glGetUniformLocation(programID, "iTime"), timePassed);
	glUniform3fv(glGetUniformLocation(programID, "lightDir"), 1, (float*)&normalize(lightDirVal)[0]);

	ModelMatrix = glm::translate(
		glm::scale(glm::mat4(1.0),glm::vec3(20.0,15.0,20.0)),
		glm::vec3(0.0,mountainHeight,0.0));
	MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

	glUniformMatrix4fv(matrixID, 1, GL_FALSE, &MVP[0][0]);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glUniform1i(textureID, 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, normTexture);
	glUniform1i(normTextureID, 1);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,(void*)0);

	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,0,(void*)0);

	glDrawArrays(GL_PATCHES, 0, vertices.size());

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	return;
}

void Renderer::PrepareCloudTextures() {
	glUseProgram(currentCloudID);

	glUniform1f(iTime, timePassed);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, bufferColourTex);
	glUniform1i(bufferTexID, 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, bufferDepthTex);
	glUniform1i(depthTexID, 2);

	glUniform3fv(cameraPos, 1, &getCameraPosition()[0]);
	glUniform3fv(cameraDir, 1, &getCameraDirection()[0]);
	glUniform3fv(cameraRight, 1, &getCameraRight()[0]);
}

void Renderer::RenderClouds() {
	PrepareCloudTextures();

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, cloudVertexbuffer);
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,(void*)0);

	glDrawArrays(GL_TRIANGLES, 0, sizeof(cloudVertices));

	glDisableVertexAttribArray(0);
}

void Renderer::RenderComputeClouds() {
	PrepareCloudTextures();

	glUniform1i(glGetUniformLocation(cloudComputeID, "destTex"), 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, finalTex);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WINDOWWIDTH, WINDOWHEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindImageTexture(0, finalTex, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glDispatchCompute(WINDOWWIDTH / 8, WINDOWHEIGHT / 8, 1);

	glUseProgram(passthroughID);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, finalTex);
	glUniform1i(glGetUniformLocation(cloudComputeID, "tex"), 0);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, cloudVertexbuffer);
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,(void*)0);

	glDrawArrays(GL_TRIANGLES, 0, sizeof(cloudVertices));

	glDisableVertexAttribArray(0);
}