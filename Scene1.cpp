#include "SceneDirector.h"
#include "Common.h"

// ============================================================================
// SCENE 1: THE COSMIC VOID & SLOW MARS REVEAL (INTERSTELLAR SCALE SHOT)
// ============================================================================
// - 0.0s - 3.5s: Deep cosmic space void (pitch black, Mars off-screen)
// - 3.5s - 7.5s: Mars glides majestically into the screen
// - 7.5s - 15.0s: The Interstellar scale shot (massive Mars, tiny dot craft in front)
// ============================================================================

void updateScene1(float dt) {
    // Spacecraft positioned directly in front of the sunlit Martian surface
    float t = seqTime / 26.0f;
    endPosX = lerpf(2.52f, 2.30f, t);
    endPosY = lerpf(0.50f, 0.38f, t);
    endPosZ = lerpf(-2.95f, -2.70f, t);
    endTanX = -0.65f; endTanY = 0.15f; endTanZ = 0.74f;

    // Ranger docked safely to Endurance in Scene 1
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
    if (seqTime < 6.0f) {
        // Phase 1: Pure deep-black space void (Mars completely off-screen)
        float t = seqTime / 6.0f;
        float e = smoothstep(0.0f, 1.0f, t);
        eyeX = lerpf(7.50f, 7.00f, e);
        eyeY = lerpf(3.00f, 2.40f, e);
        eyeZ = lerpf(-7.80f, -7.20f, e);

        targetX = lerpf(15.00f, 14.20f, e);
        targetY = lerpf( 6.50f,  5.80f, e);
        targetZ = lerpf(-12.00f, -11.20f, e);

        upX = -0.06f; upY = 0.998f; upZ = 0.0f;
    } else if (seqTime < 15.0f) {
        // Phase 2: Ultra-slow cinematic pan bringing Sunlit Mars into full frame (9 seconds)
        float t = (seqTime - 6.0f) / 9.0f;
        float e = smoothstep(0.0f, 1.0f, t);

        eyeX = lerpf(7.00f, 4.35f, e);
        eyeY = lerpf(2.40f, 0.95f, e);
        eyeZ = lerpf(-7.20f, -5.05f, e);

        targetX = lerpf(14.20f, 0.12f, e);
        targetY = lerpf( 5.80f, 0.04f, e);
        targetZ = lerpf(-11.20f, 0.05f, e);

        upX = lerpf(-0.06f, 0.04f, e);
        upY = lerpf(0.998f, 0.999f, e);
        upZ = 0.0f;
    } else {
        // Phase 3: The Interstellar Scale Shot holds on Sunlit Mars with tiny dot craft
        float t = (seqTime - 15.0f) / 11.0f;
        float e = smoothstep(0.0f, 1.0f, t);

        eyeX = lerpf(4.35f, 4.55f, e);
        eyeY = lerpf(0.95f, 1.02f, e);
        eyeZ = lerpf(-5.05f, -4.85f, e);

        targetX = lerpf(0.12f, 0.18f, e);
        targetY = lerpf(0.04f, 0.06f, e);
        targetZ = lerpf(0.05f, 0.08f, e);

        upX = 0.04f; upY = 0.999f; upZ = 0.0f;
    }
}

