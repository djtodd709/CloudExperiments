#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
using namespace std;

#include <stdlib.h>
#include <string.h>

#include <GL/glew.h>

#include "shader.hpp"

GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path, const char* tessC_file_path, const char* tessE_file_path){
	bool tess = true;

	if (tessC_file_path == NULL) {
		tess = false;
		tessC_file_path = " ";
		tessE_file_path = " ";
	}

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	GLuint TessCShaderID;
	GLuint TessEShaderID;

	if (tess) {
		TessCShaderID = glCreateShader(GL_TESS_CONTROL_SHADER);
		TessEShaderID = glCreateShader(GL_TESS_EVALUATION_SHADER);
	}

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open()){
		std::stringstream sstr;
		sstr << VertexShaderStream.rdbuf();
		VertexShaderCode = sstr.str();
		VertexShaderStream.close();
	}else{
		std::printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", vertex_file_path);
		getchar();
		return 0;
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::stringstream sstr;
		sstr << FragmentShaderStream.rdbuf();
		FragmentShaderCode = sstr.str();
		FragmentShaderStream.close();
	}

	// Read the Tess Shader code from the file
	std::string TessCShaderCode;
	std::ifstream TessCShaderStream(tessC_file_path, std::ios::in);
	std::string TessEShaderCode;
	std::ifstream TessEShaderStream(tessE_file_path, std::ios::in);

	if (tess) {
		if (TessCShaderStream.is_open()) {
			std::stringstream sstr;
			sstr << TessCShaderStream.rdbuf();
			TessCShaderCode = sstr.str();
			TessCShaderStream.close();
		}
		if (TessEShaderStream.is_open()) {
			std::stringstream sstr;
			sstr << TessEShaderStream.rdbuf();
			TessEShaderCode = sstr.str();
			TessEShaderStream.close();
		}
	}



	GLint Result = GL_FALSE;
	int InfoLogLength;


	// Compile Vertex Shader
	std::printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> VertexShaderErrorMessage(InfoLogLength+1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		std::printf("%s\n", &VertexShaderErrorMessage[0]);
	}



	// Compile Fragment Shader
	std::printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength+1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		std::printf("%s\n", &FragmentShaderErrorMessage[0]);
	}

	char const* TessCSourcePointer;
	char const* TessESourcePointer;

	if (tess) {
		// Compile TessC Shader
		std::printf("Compiling shader : %s\n", tessC_file_path);
		TessCSourcePointer = TessCShaderCode.c_str();
		glShaderSource(TessCShaderID, 1, &TessCSourcePointer, NULL);
		glCompileShader(TessCShaderID);

		// Check TessC Shader
		glGetShaderiv(TessCShaderID, GL_COMPILE_STATUS, &Result);
		glGetShaderiv(TessCShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
		if (InfoLogLength > 0) {
			std::vector<char> TessCShaderErrorMessage(InfoLogLength + 1);
			glGetShaderInfoLog(TessCShaderID, InfoLogLength, NULL, &TessCShaderErrorMessage[0]);
			std::printf("%s\n", &TessCShaderErrorMessage[0]);
		}


		// Compile TessE Shader
		std::printf("Compiling shader : %s\n", tessE_file_path);
		TessESourcePointer = TessEShaderCode.c_str();
		glShaderSource(TessEShaderID, 1, &TessESourcePointer, NULL);
		glCompileShader(TessEShaderID);

		// Check TessE Shader
		glGetShaderiv(TessEShaderID, GL_COMPILE_STATUS, &Result);
		glGetShaderiv(TessEShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
		if (InfoLogLength > 0) {
			std::vector<char> TessEShaderErrorMessage(InfoLogLength + 1);
			glGetShaderInfoLog(TessEShaderID, InfoLogLength, NULL, &TessEShaderErrorMessage[0]);
			std::printf("%s\n", &TessEShaderErrorMessage[0]);
		}
	}


	// Link the program
	std::printf("Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	if (tess) {
		glAttachShader(ProgramID, TessCShaderID);
		glAttachShader(ProgramID, TessEShaderID);
	}
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		std::vector<char> ProgramErrorMessage(InfoLogLength+1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		std::printf("%s\n", &ProgramErrorMessage[0]);
	}

	
	glDetachShader(ProgramID, VertexShaderID);
	glDetachShader(ProgramID, FragmentShaderID);
	
	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	if (tess) {
		glDetachShader(ProgramID, TessCShaderID);
		glDetachShader(ProgramID, TessEShaderID);

		glDeleteShader(TessCShaderID);
		glDeleteShader(TessEShaderID);
	}

	return ProgramID;
}

GLuint LoadComputeShader(const char* file_path) {
	GLuint ComputeShaderID = glCreateShader(GL_COMPUTE_SHADER);

	// Read the Compute Shader code from the file
	std::string ComputeShaderCode;
	std::ifstream ComputeShaderStream(file_path, std::ios::in);
	if (ComputeShaderStream.is_open()) {
		std::stringstream sstr;
		sstr << ComputeShaderStream.rdbuf();
		ComputeShaderCode = sstr.str();
		ComputeShaderStream.close();
	}
	else {
		std::printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", file_path);
		getchar();
		return 0;
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;


	// Compile Compute Shader
	std::printf("Compiling shader : %s\n", file_path);
	char const* ComputeSourcePointer = ComputeShaderCode.c_str();
	glShaderSource(ComputeShaderID, 1, &ComputeSourcePointer, NULL);
	glCompileShader(ComputeShaderID);

	// Check Vertex Shader
	glGetShaderiv(ComputeShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(ComputeShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> ComputeShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(ComputeShaderID, InfoLogLength, NULL, &ComputeShaderErrorMessage[0]);
		std::printf("%s\n", &ComputeShaderErrorMessage[0]);
	}

	// Link the program
	std::printf("Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, ComputeShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		std::printf("%s\n", &ProgramErrorMessage[0]);
	}


	glDetachShader(ProgramID, ComputeShaderID);
	glDeleteShader(ComputeShaderID);

	return ProgramID;
}
