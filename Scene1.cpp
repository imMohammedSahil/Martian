#include "SceneDirector.h"
#include "Common.h"

// ============================================================================
// SCENE 1: CONTINUOUS ONE-TAKE CINEMATIC FLIGHT (ZERO CUTS)
// ============================================================================
// Shot 1 (0.0s -  4.5s): Deep space cosmic void
// Shot 2 (4.5s - 10.5s): Smooth continuous glide to center full sunlit Mars
// Shot 3 (10.5s - 17.5s): Smooth continuous dip into Mars horizon bowl + micro craft
// Shot 4 (17.5s - 28.0s): Smooth continuous push-in right to Endurance + Title Lock
// ============================================================================

struct CamKey {
    float t;
    float eyeX, eyeY, eyeZ;
    float tarX, tarY, tarZ;
};

static const CamKey CAM_KEYS[] = {
    // Shot 1 (0.0s - 4.5s): Deep cosmic void panning
    {  0.0f, -5.50f,  0.00f, 12.50f,  20.00f,  0.00f, 12.50f },
    {  4.5f, -5.50f,  0.00f, 12.50f,  20.00f,  0.00f, 12.50f },

    // Shot 2 (4.5s - 13.0s): Full Mars disc centered in frame. Endurance is a tiny stationary dot in orbit with flashing beacon.
    { 13.0f, -2.50f,  0.00f, 12.00f,   0.00f,  0.00f,  0.00f },

    // Shot 3 (13.0s - 22.0s): Camera physically flies continuously toward southern hemisphere; Mars expands to cover 100% of the screen.
    { 22.0f,  0.00f, -0.60f,  3.12f,   0.00f, -0.60f,  2.45f },

    // Shot 4 (22.0s - 35.0s): Mars fully covers screen. Camera physically glides right up to stationary spinning Endurance.
    // Title card fades in at 27.0s framing the craft at 'T'.
    { 28.5f,  0.00f, -0.60f,  2.97f,   0.00f, -0.60f,  2.45f },
    { 35.0f,  0.00f, -0.60f,  2.92f,   0.00f, -0.60f,  2.45f }
};
static const int NUM_CAM_KEYS = sizeof(CAM_KEYS) / sizeof(CAM_KEYS[0]);

void updateScene1(float dt) {
    // Endurance in low Martian orbit (Z = 2.45) where Mars completely covers the screen behind it
    endPosX = 0.0f;
    endPosY = -0.60f;
    endPosZ = 2.45f;
    endTanX = 0.0f; endTanY = 0.0f; endTanZ = 1.0f;

    rangerDetached = false;
    rangerPosX = endPosX;
    rangerPosY = endPosY;
    rangerPosZ = endPosZ;
    rangerDirX = endTanX;
    rangerDirY = endTanY;
    rangerDirZ = endTanZ;
    rangerPitchExtra = 0.0f;
    rangerRollAngle = ringSpinAngle * 0.2f;
    heatshieldThermal = 0.0f;
    rcsActive = 0.0f;
    mainBurnActive = 0.0f;
}

void computeCameraScene1(float& eyeX, float& eyeY, float& eyeZ,
                        float& targetX, float& targetY, float& targetZ,
                        float& upX, float& upY, float& upZ) {
    upX = 0.0f; upY = 1.0f; upZ = 0.0f;

    // Piecewise smoothstep flight path
    for (int i = 0; i < NUM_CAM_KEYS - 1; ++i) {
        if (seqTime <= CAM_KEYS[i + 1].t) {
            float t0 = CAM_KEYS[i].t;
            float t1 = CAM_KEYS[i + 1].t;
            float u  = smoothstep(t0, t1, seqTime);

            eyeX = lerpf(CAM_KEYS[i].eyeX, CAM_KEYS[i + 1].eyeX, u);
            eyeY = lerpf(CAM_KEYS[i].eyeY, CAM_KEYS[i + 1].eyeY, u);
            eyeZ = lerpf(CAM_KEYS[i].eyeZ, CAM_KEYS[i + 1].eyeZ, u);

            targetX = lerpf(CAM_KEYS[i].tarX, CAM_KEYS[i + 1].tarX, u);
            targetY = lerpf(CAM_KEYS[i].tarY, CAM_KEYS[i + 1].tarY, u);
            targetZ = lerpf(CAM_KEYS[i].tarZ, CAM_KEYS[i + 1].tarZ, u);
            return;
        }
    }

    // End hold
    eyeX = CAM_KEYS[NUM_CAM_KEYS - 1].eyeX;
    eyeY = CAM_KEYS[NUM_CAM_KEYS - 1].eyeY;
    eyeZ = CAM_KEYS[NUM_CAM_KEYS - 1].eyeZ;
    targetX = CAM_KEYS[NUM_CAM_KEYS - 1].tarX;
    targetY = CAM_KEYS[NUM_CAM_KEYS - 1].tarY;
    targetZ = CAM_KEYS[NUM_CAM_KEYS - 1].tarZ;
}

