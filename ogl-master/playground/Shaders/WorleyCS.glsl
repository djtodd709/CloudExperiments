#version 430

writeonly uniform image3D destTex;
writeonly uniform image3D detailTex;
layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

mat3 matR = mat3(56., 37., 81., -26., -49., 66., 29., -34., 48.);
mat3 matG = mat3(73., 39., 28., -14.,  92., 24., -13., -87., 26.);
mat3 matB = mat3(58., 42., 41., 67., -25., -59., -29., 47., 92.);
mat3 matRd = mat3(-38., 36., -45., 28., 96., 34., 54., -24., 53.);
mat3 matGd = mat3(33., -35., 52., 34., 25., -82., 63., 84., -26.);
mat3 matBd = mat3(-21., 33., -84., 48., -66., -35., -79., 73., 43.);


vec3 hash3(int pX, int pY, int pZ, mat3 hashMat)
{
	return fract(28.9 * cos(vec3(pX, pY, pZ) * hashMat));
}

vec3 hash3(vec3 p)
{
    return hash3(int(p.x), int(p.y), int(p.z), matR);
}

float Perlin(vec3 p)
{

    vec3 pInt = floor(p);
    vec3 pFrac = fract(p);

    vec3 w = pFrac * pFrac * (3.0 - 2.0 * pFrac);

    return 	mix(
        mix(
            mix(dot(pFrac - vec3(0, 0, 0), hash3(pInt + vec3(0, 0, 0))),
                dot(pFrac - vec3(1, 0, 0), hash3(pInt + vec3(1, 0, 0))),
                w.x),
            mix(dot(pFrac - vec3(0, 0, 1), hash3(pInt + vec3(0, 0, 1))),
                dot(pFrac - vec3(1, 0, 1), hash3(pInt + vec3(1, 0, 1))),
                w.x),
            w.z),
        mix(
            mix(dot(pFrac - vec3(0, 1, 0), hash3(pInt + vec3(0, 1, 0))),
                dot(pFrac - vec3(1, 1, 0), hash3(pInt + vec3(1, 1, 0))),
                w.x),
            mix(dot(pFrac - vec3(0, 1, 1), hash3(pInt + vec3(0, 1, 1))),
                dot(pFrac - vec3(1, 1, 1), hash3(pInt + vec3(1, 1, 1))),
                w.x),
            w.z),
        w.y);
}

float Worley(vec3 p, mat3 hashMat, int wrapFac)
{
    float scale = 128. / float(wrapFac);
    p /= scale;

    float dist = 1.0;
    vec3 pInt = floor(p);
    vec3 pFrac = fract(p);

    for (int x = -1; x <= 1; x++)
    for (int y = -1; y <= 1; y++)
    for (int z = -1; z <= 1; z++)
    {
        int newX = int((pInt + vec3(x, y, z)).x);
        int newY = int((pInt + vec3(x, y, z)).y);
        int newZ = int((pInt + vec3(x, y, z)).z);

        if (newX < 0) newX = wrapFac-1;
        if (newY < 0) newY = wrapFac - 1;
        if (newZ < 0) newZ = wrapFac - 1;
        if (newX >= wrapFac) newX = 0;
        if (newY >= wrapFac) newY = 0;
        if (newZ >= wrapFac) newZ = 0;
        float pDist = distance(hash3(newX, newY, newZ, hashMat) + vec3(x, y, z), pFrac);
        dist = min(dist, pDist);
    }
    return dist;
}

void main()
{
	ivec3 storePos = ivec3(gl_GlobalInvocationID);

    float persistance = 0.75;
    float maxVal = 1 + (persistance)+(persistance * persistance);

    float noiseSum = 1.0 * Perlin(storePos) +
        persistance * Worley(storePos, matG, 6) +
        persistance * persistance * Worley(storePos, matB, 10);

    float noiseSumD = Worley(storePos, matRd, 10) +
        persistance * Worley(storePos, matGd, 13) +
        persistance * persistance * Worley(storePos, matBd, 16);

    // keep inside range [0,1] as will be clamped in texture
    noiseSum /= maxVal;
    noiseSum = 1 - noiseSum;
    noiseSum = max(0.0,min(1.0,(noiseSum)));

    noiseSumD /= maxVal;
    noiseSumD = 1 - noiseSumD;
    noiseSumD = max(0.0, min(1.0, (noiseSumD)));

	imageStore(destTex, storePos, vec4(vec3(noiseSum), 1.0));

    if (storePos.x % 2 == 1 || storePos.y % 2 == 1 || storePos.z % 2 == 1) {
        return;
    }
    storePos /= 2;

    imageStore(detailTex, storePos, vec4(vec3(noiseSumD), 1.0));
}
