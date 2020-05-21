#version 430 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vertexPosition_modelspace;
layout(location = 1) in vec2 vertexUV;

// Output data ; will be interpolated for each fragment.
out vec2 UV;
out vec3 modelPos;

void main(){

	// Output position of the vertex, in clip space : MVP * position
	modelPos = vertexPosition_modelspace;

	// UV of the vertex. No special space for this one.
	UV = vertexUV;
}