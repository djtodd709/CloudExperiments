#version 430 core

uniform vec2 iResolution;
uniform vec3 camPos;
uniform vec3 camDir;
uniform vec3 camRight;

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

// Returns (dstToBox, dstInsideBox). If ray misses box, dstInsideBox will be zero
vec2 rayBoxDst(vec3 boundsMin, vec3 boundsMax, Ray ray) {
	// Adapted from: http://psgraphics.blogspot.com/2016/02/new-simple-ray-box-test-from-andrew.html
	// And: https://github.com/SebLague/Clouds

	vec3 invDir = vec3(1.0) / ray.direction;
	vec3 t0 = (boundsMin - ray.origin) * invDir;
	vec3 t1 = (boundsMax - ray.origin) * invDir;
	vec3 tmin = min(t0, t1);
	vec3 tmax = max(t0, t1);

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


void main()
{
	float fov = tan(45.0 * 0.5 * (3.1415926535897932384626433832795 / 180.0));	//FOV adjust
	float aspect = iResolution.x/ iResolution.y;
	vec2 p = (-iResolution.xy + 2.0 * gl_FragCoord.xy)/ iResolution.y;
	p*= fov;

	//vec3 normCamDir = normalize(camDir);
	//vec3 normCamRight = normalize(camRight);
	
	vec3 camUp = cross(camDir, camRight);
	//vec3 rayDir = normalize(normCamRight * p.x + camUp * -p.y + normCamDir);
	vec3 rayDir = normalize(camRight * p.x + camUp * -p.y + camDir);


	AABB cloudBox = AABB(vec3(-3.0,0.5,-3.0), vec3(3.0,0.6,3.0));
	Ray ray = Ray(camPos, rayDir);
	vec2 boxDist = rayBoxDst(cloudBox.boundsMin, cloudBox.boundsMax, ray);
	fragColor = vec4(boxDist.x,boxDist.y,0.0, 1.0);
}
