#version 430

uniform vec2 iResolution;
uniform float iTime;
uniform vec3 camPos;
uniform vec3 camDir;
uniform vec3 camRight;
uniform vec3 lightCol;
uniform vec3 lightDir;

uniform vec3 skyCol;
uniform vec3 cloudCol;

uniform vec3 cloudScale;
uniform float detailScale;

uniform vec3 cloudSpeed;
uniform vec3 detailSpeed;

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

uniform float baseTransmittance;

uniform float forwardScattering;
uniform float backScattering;
uniform float baseBrightness;
uniform float phaseFactor;

uniform float optFactor;

uniform vec3 cloudMin;
uniform vec3 cloudMax;

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

vec3 sampleAdjust;
vec3 sampleAdjustDetail;

AABB cloudBox = AABB(vec3(-20.0, 0, -20.0), vec3(20.0, 8, 20.0));

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
	vec3 edgeDst = min(samplePos - cloudBox.boundsMin, cloudBox.boundsMax - samplePos);
	float edgeFade = min(min(edgeDst.x, min(edgeDst.y, edgeDst.z)), 1.0);

	samplePos *= cloudScale;
	vec3 detPos = samplePos;

	samplePos = samplePos*0.03 + sampleAdjust;
	
	float sampled = min(1.0, (texture(worleyTex, samplePos).r - densityOfst) * densityMult);
	sampled *= edgeFade;
	if (sampled > 0.01) {
		detPos = detPos*0.15 * detailScale + sampleAdjustDetail;
		sampled = min(1.0, max(0.0, sampled - texture(detailTex, detPos).r));
	}
	return sampled;
}

float lightMarch(vec3 cloudPos) {
	
	Ray ray = Ray(cloudPos, lightDir);
	float dstInBox = rayBoxDst(cloudBox.boundsMin, cloudBox.boundsMax, ray).y;

	float stepSize = dstInBox / numLightSteps;
	float totalDensity = 0.0;

	for (int step = 0; step < numLightSteps; step++) {
		cloudPos += lightDir * stepSize;
		totalDensity += max(0.0, sampleDensity(cloudPos) * stepSize);
	}

	float transmittance = exp(-totalDensity);
	return baseTransmittance + transmittance*(1- baseTransmittance);
}

// Henyey-Greenstein
float hg(float a, float g) {
	float g2 = g * g;
	return (1 - g2) / (4 * 3.1415 * pow(1 + g2 - 2 * g * (a), 1.5));
}

float phase(vec3 rayDir) {
	float a = dot(rayDir, lightDir);
	float blend = .5;
	float hgBlend = hg(a, forwardScattering) * (1 - blend) + hg(a, -backScattering) * blend;
	return baseBrightness + hgBlend * phaseFactor;
}

vec3 skySample(vec3 rayDir) {
	float sun = dot(rayDir, lightDir) * 0.5 + 0.5;
	sun = 0.5 + 0.5*tanh(100.0*sun - 98.5);
	return vec3(sun) * lightCol + vec3(1 - sun) * skyCol;
}

void main()
{
	cloudBox = AABB(cloudMin, cloudMax);

	vec2 coords = gl_FragCoord.xy / iResolution.xy;

	sampleAdjust = iTime * cloudSpeed;
	sampleAdjustDetail = iTime * detailSpeed;
	
	float nonLinDepth = texture(depthTex, coords).x;
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
		if (nonLinDepth == 1.0) {
			fragColor = vec4(skySample(rayDir), 1.0);
		}
		else {
			fragColor = texture(bufferTex, coords);
		}
		return;
	}

	float phaseVal = phase(rayDir);

	float stepSize = numSteps;
	float dstLimit = min(depth-boxDist.x * cosTheta,boxDist.y);

	float dstTravelled = 0.0;
	float lightEnergy = 0.0;
	float transmittance = 1.0;

	float lastStepRoot = 0.0;

	while (dstTravelled < dstLimit) {
		vec3 texPos = camPos + (boxDist.x + dstTravelled) * rayDir;
		float density = sampleDensity(texPos);

		if (density * lastStepRoot < 0.0) {
			lastStepRoot = 0.0;
		}

		if (density > 0.01) {
			float lightTransmittance = lightMarch(texPos);
			lightEnergy += density * (stepSize+lastStepRoot*lastStepRoot) * transmittance * lightTransmittance * phaseVal;
			transmittance *= exp(-density * (stepSize + lastStepRoot * lastStepRoot));

			if (transmittance < 0.01) {
				break;
			}
		}

		lastStepRoot = optFactor*density;
		dstTravelled += stepSize + lastStepRoot *lastStepRoot;
	}
	//geometry intersection
	if (dstLimit < boxDist.y) {
		stepSize = dstLimit - (dstTravelled - stepSize);

		vec3 texPos = camPos + (boxDist.x + dstLimit) * rayDir;
		float density = sampleDensity(texPos);

		if (density > 0.01) {
			float lightTransmittance = lightMarch(texPos);
			lightEnergy += density * stepSize * transmittance * lightTransmittance;
			transmittance *= exp(-density * stepSize);
		}
	}

	vec3 bgCol;
	if (nonLinDepth == 1.0) {
		bgCol = skySample(rayDir);
	}
	else {
		bgCol = texture(bufferTex, coords).rgb;
	}
	
	vec3 cloudColFinal = lightEnergy * cloudCol;
	vec3 col = max(vec3(0.0),min(vec3(1.0),bgCol * transmittance + cloudColFinal));
	fragColor = vec4(col, 1.0);
	
}