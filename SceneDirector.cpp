#include "SceneDirector.h"
#include "Common.h"

// Analytical Ray-Sphere Eclipse Test for Mars Sun Occlusion
float computeSunMarsOcclusion(float camX, float camY, float camZ) {
    float rayDx = SUN_DIR_X;
    float rayDy = SUN_DIR_Y;
    float rayDz = SUN_DIR_Z;
    normalize3(rayDx, rayDy, rayDz);

    float ocX = -camX;
    float ocY = -camY;
    float ocZ = -camZ;

    float tProj = ocX * rayDx + ocY * rayDy + ocZ * rayDz;
    if (tProj <= 0.0f) {
        return 1.0f; // Sun is behind camera
    }

    float perpSq = (ocX * ocX + ocY * ocY + ocZ * ocZ) - (tProj * tProj);
    float dPerp = (perpSq > 0.0f) ? sqrtf(perpSq) : 0.0f;

    const float MARS_R_OCCL = MARS_RADIUS * 1.045f;
    if (dPerp < MARS_R_OCCL) {
        return 0.0f; // Completely occluded by Mars
    }

    const float MARS_ATMOS_EDGE = MARS_RADIUS * 1.085f;
    if (dPerp < MARS_ATMOS_EDGE) {
        return smoothstep(MARS_R_OCCL, MARS_ATMOS_EDGE, dPerp);
    }

    return 1.0f;
}

void updateSceneDirector(float dt) {
    marsSpinAngle += 3.5f * dt;
    if (marsSpinAngle >= 360.0f) marsSpinAngle -= 360.0f;

    phobosAngle += 0.045f * dt;
    ringSpinAngle += 24.0f * dt;
    if (ringSpinAngle >= 360.0f) ringSpinAngle -= 360.0f;

    // Multi-scene sequencing
    if (seqTime <= 35.0f) {
        updateScene1(dt);
    } else if (seqTime < 44.0f) {
        updateScene2(dt);
    } else if (seqTime < 65.0f) {
        updateScene3(dt);
    } else {
        updateScene4(dt);
    }
}

void computeCinematicCamera(float& eyeX, float& eyeY, float& eyeZ,
                            float& targetX, float& targetY, float& targetZ,
                            float& upX, float& upY, float& upZ) {
    upX = 0.0f; upY = 1.0f; upZ = 0.0f;

    if (seqTime <= 35.0f) {
        computeCameraScene1(eyeX, eyeY, eyeZ, targetX, targetY, targetZ, upX, upY, upZ);
    } else if (seqTime < 44.0f) {
        computeCameraScene2(eyeX, eyeY, eyeZ, targetX, targetY, targetZ, upX, upY, upZ);
    } else if (seqTime < 65.0f) {
        computeCameraScene3(eyeX, eyeY, eyeZ, targetX, targetY, targetZ, upX, upY, upZ);
    } else {
        computeCameraScene4(eyeX, eyeY, eyeZ, targetX, targetY, targetZ, upX, upY, upZ);
    }
}

