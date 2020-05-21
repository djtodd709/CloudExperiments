#version 430 core

layout(location = 0) out vec4 fragColor;

uniform sampler2D heightMap;
uniform sampler2D normalMap;

in vec2 UV;

void main() {

	vec3 normal = texture(normalMap, UV).rgb;


	float sunAngle = dot(normalize(normal), normalize(vec3(1.0, -1.0, 1.0)));

	float addShadow = sunAngle < 0.5 ? 1.0 : sunAngle < 0.7 ? 0.5 : 0.2;

	fragColor = vec4(addShadow * texture(heightMap, UV).rgb*vec3(0.8,0.5,0.4), 1.0);
}
