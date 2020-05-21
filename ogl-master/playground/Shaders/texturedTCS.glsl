#version 430 core

layout(vertices = 3) out;

in vec2 UV[];
in vec3 modelPos[];

// Output data ; will be interpolated for each fragment.
out vec2 mapCoords[];
out vec3 mapPos[];

void main(void) {
	gl_TessLevelInner[0] = 14;
	gl_TessLevelInner[1] = 14;
	gl_TessLevelOuter[0] = 14;
	gl_TessLevelOuter[1] = 14;
	gl_TessLevelOuter[2] = 14;
	gl_TessLevelOuter[3] = 14;

	//gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

	mapCoords[gl_InvocationID] = UV[gl_InvocationID];
	mapPos[gl_InvocationID] = modelPos[gl_InvocationID];

}