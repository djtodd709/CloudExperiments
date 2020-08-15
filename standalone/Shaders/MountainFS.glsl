#version 430 core

layout(location = 0) out vec4 fragColor;

uniform sampler2D heightMap;
uniform sampler2D normalMap;

uniform vec3 lightDir;

in vec2 UV;

void main() {

	vec3 normal = -1.0+2.0*texture(normalMap, UV).rgb;
	normal = vec3(-normal.r, normal.bg);

	float sunAngle = 0.5+ 0.5*dot(normalize(normal), vec3(lightDir));

	fragColor = vec4(sunAngle* texture(heightMap, UV).rgb*vec3(0.8,0.5,0.4), 1.0);
}
