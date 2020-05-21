#version 430 core

layout(triangles) in;

uniform sampler2D heightMap;

in vec2 mapCoords[];
in vec3 mapPos[];

// Values that stay constant for the whole mesh.
uniform mat4 MVP;

out vec2 UV;

vec2 interpolate2D(vec2 v0, vec2 v1, vec2 v2)
{
	return vec2(gl_TessCoord.x) * v0 + vec2(gl_TessCoord.y) * v1 + vec2(gl_TessCoord.z) * v2;
}

vec3 interpolate3D(vec3 v0, vec3 v1, vec3 v2)
{
	return vec3(gl_TessCoord.x) * v0 + vec3(gl_TessCoord.y) * v1 + vec3(gl_TessCoord.z) * v2;
}

void main(){

	UV = interpolate2D(mapCoords[0], mapCoords[1], mapCoords[2]);
	vec3 modelSpace = interpolate3D(mapPos[0], mapPos[1], mapPos[2]);

	modelSpace.y += texture(heightMap, UV.xy).x;

	// Output position of the vertex, in clip space : MVP * position
	gl_Position =  MVP * vec4(modelSpace,1.0);

	
}