#version 430 core

uniform vec2 iResolution;
uniform float iTime;
uniform vec3 camPos;
uniform vec3 camDir;
uniform vec3 camRight;

uniform float zNear;
uniform float zFar;

uniform sampler3D worleyTex;
uniform sampler3D detailTex;
uniform sampler2D bufferTex;
uniform sampler2D depthTex;

uniform float numSteps;
uniform float numLightSteps;

uniform float densityMult;
uniform float densityOfst;

// Output data
layout(location = 0) out vec4 fragColor;

struct Ray {
	vec3 origin;
	vec3 direction;
};

struct AABB {
	vec3 boundsMin;
	vec3 boundsMax;
};

AABB cloudBox = AABB(vec3(-20.0, 4, -20.0), vec3(20.0, 12, 20.0));

// Returns (dstToBox, dstInsideBox). If ray misses box, dstInsideBox will be zero
vec2 rayBoxDst(vec3 boundsMin, vec3 boundsMax, Ray ray) {
	// Adapted from: http://psgraphics.blogspot.com/2016/02/new-simple-ray-box-test-from-andrew.html
	// And: https://github.com/SebLague/Clouds

	vec3 invDir = vec3(1.0) / ray.direction;
	vec3 t0 = (boundsMin - ray.origin) * invDir;
	vec3 t1 = (boundsMax - ray.origin) * invDir;
	vec4 tmin = vec4(min(t0, t1), 1.0);
	vec4 tmax = vec4(max(t0, t1), 1.0);

	float dstA = max(max(tmin.x, tmin.y), tmin.z);
	float dstB = min(tmax.x, min(tmax.y, tmax.z));

	// CASE 1: ray intersects box from outside (0 <= dstA <= dstB)
				// dstA is dst to nearest intersection, dstB dst to far intersection

				// CASE 2: ray intersects box from inside (dstA < 0 < dstB)
				// dstA is the dst to intersection behind the ray, dstB is dst to forward intersection

				// CASE 3: ray misses box (dstA > dstB)

	float dstToBox = max(0, dstA);
	float dstInsideBox = max(0, dstB - dstToBox);
	return vec2(dstToBox, dstInsideBox);
}

float sampleDensity(vec3 samplePos) {
	vec3 detPos = samplePos;

	//LESS CODE HERE IF POSSIBLE.
	vec3 edgeDst = min(samplePos - cloudBox.boundsMin, cloudBox.boundsMax - samplePos);
	float edgeFade = min(min(edgeDst.x, min(edgeDst.y, edgeDst.z)), 1.0);

	samplePos *= 0.02;
	samplePos.x += iTime / 256.0;
	samplePos.z += iTime / 400.0;
	float sampled = min(1.0, max(0.0, texture(detailTex, samplePos).r * densityMult - densityOfst));
	sampled *= edgeFade;
	if (sampled > 0.01) {
		detPos *= 0.15;
		detPos.x += iTime / 136.0;
		detPos.z += iTime / 200.0;
		sampled = min(1.0, max(0.0, sampled - texture(detailTex, detPos).r));
	}
	return sampled;
}

vec3 LightDir = normalize(vec3(0.5, 1.0, 0.5));

float lightMarch(vec3 cloudPos) {
	//LightDir = normalize(vec3(cos(iTime/20.0), 1, sin(iTime / 20.0)));
	Ray ray = Ray(cloudPos, LightDir);
	float dstInBox = rayBoxDst(cloudBox.boundsMin, cloudBox.boundsMax, ray).y;

	float stepSize = dstInBox / numLightSteps;
	float totalDensity = 0.0;

	for (int step = 0; step < numLightSteps; step++) {
		cloudPos += LightDir * stepSize;
		totalDensity += max(0.0, sampleDensity(cloudPos) * stepSize);
	}

	float transmittance = exp(-totalDensity);
	return 0.25+transmittance*0.75;
}

void main()
{
	vec2 coords = gl_FragCoord.xy / iResolution.xy;

	if (gl_FragCoord.x <= 128 && gl_FragCoord.y <= 128) {
		
		fragColor = vec4(vec3(texture(worleyTex, vec3(gl_FragCoord.xy/128., iTime / 256.0))),1.0);
		return;
	}
	if (gl_FragCoord.x <= 192 && gl_FragCoord.y <= 64) {

		fragColor = vec4(vec3(texture(detailTex, vec3(gl_FragCoord.xy / 64., iTime / 256.0))), 1.0);
		return;
	}
	
	float nonLinDepth = texture2D(depthTex, coords).x;
	float z_n = 2.0 * nonLinDepth - 1.0;
	float depth = 2.0 * zNear * zFar / (zFar + zNear - z_n * (zFar - zNear));


	float fov = tan(45.0 * 0.5 * (3.1415926535897932384626433832795 / 180.0));	//FOV adjust
	vec2 p = (-iResolution.xy + 2.0 * gl_FragCoord.xy)/ iResolution.y;
	p*= fov;
	p.x *= (4.0 / 3.0)/(iResolution.x/iResolution.y);
	
	vec3 camUp = cross(camDir, camRight);
	vec3 rayDir = normalize(camRight * p.x + camUp * -p.y + camDir);

	float cosTheta = dot(camDir, rayDir);

	Ray ray = Ray(camPos, rayDir);
	vec2 boxDist = rayBoxDst(cloudBox.boundsMin, cloudBox.boundsMax, ray);
	if (boxDist.y <= 0 || boxDist.x * cosTheta > depth) {
		fragColor = texture(bufferTex, coords);
		return;
	}

	float stepSize = boxDist.y / numSteps;
	float dstLimit = min(depth-boxDist.x * cosTheta,boxDist.y);

	float dstTravelled = 0.0;
	float lightEnergy = 0.0;
	float transmittance = 1.0;

	while (dstTravelled < dstLimit) {
		vec3 texPos = camPos + (boxDist.x + dstTravelled) * rayDir;
		float density = sampleDensity(texPos);

		if (density > 0.01) {
			float lightTransmittance = lightMarch(texPos);
			lightEnergy += density * stepSize * transmittance * lightTransmittance;
			transmittance *= exp(-density *stepSize);

			if (transmittance < 0.01) {
				break;
			}
		}
		dstTravelled += stepSize;
	}

	vec3 bgCol = texture(bufferTex, coords).rgb;
	vec3 cloudCol = lightEnergy * vec3(1.0,.7,.7);
	vec3 col = max(vec3(0.0),min(vec3(1.0),bgCol * transmittance + cloudCol));
	fragColor = vec4(col, 1.0);
	
}