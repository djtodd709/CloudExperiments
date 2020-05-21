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

float Worley(vec3 P, mat3 hashMat, int wrapFac)
{
    float scale = 128. / float(wrapFac);
    P /= scale;

    float Dist = 1.0;
    vec3 I = floor(P);
    vec3 F = fract(P);

    for (int X = -1; X <= 1; X++)
    for (int Y = -1; Y <= 1; Y++)
    for (int Z = -1; Z <= 1; Z++)
    {
        int newX = int((I + vec3(X, Y, Z)).x);
        int newY = int((I + vec3(X, Y, Z)).y);
        int newZ = int((I + vec3(X, Y, Z)).z);

        if (newX < 0) newX = wrapFac-1;
        if (newY < 0) newY = wrapFac - 1;
        if (newZ < 0) newZ = wrapFac - 1;
        if (newX >= wrapFac) newX = 0;
        if (newY >= wrapFac) newY = 0;
        if (newZ >= wrapFac) newZ = 0;
        float D = distance(hash3(newX, newY, newZ, hashMat) + vec3(X, Y, Z), F);
        Dist = min(Dist, D);
    }
    return Dist;
}

void main()
{
	ivec3 storePos = ivec3(gl_GlobalInvocationID);

    float persistance = 0.75;
    float maxVal = 1 + (persistance)+(persistance * persistance);

    float noiseSum = Worley(storePos, matR, 3) +
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
