#version 430 core

layout(location = 0) out vec4 fragColor;

uniform sampler2D heightMap;

in vec2 UV;

void main() {
	fragColor = vec4(texture(heightMap, UV).rgb, 1.0);
}
