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
    // Spacecraft positioned directly in front of the brightly sunlit Martian face
    float t = seqTime / 26.0f;
    endPosX = lerpf(-1.52f, -1.35f, t);
    endPosY = lerpf( 0.32f,  0.22f, t);
    endPosZ = lerpf( 2.90f,  2.65f, t);
    endTanX = 0.65f; endTanY = -0.15f; endTanZ = -0.74f;

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
    upX = 0.0f; upY = 1.0f; upZ = 0.0f;

    if (seqTime < 6.0f) {
        // Phase 1: Pure deep-black space void (camera aimed up into cosmic dark, Mars off-screen)
        float t = seqTime / 6.0f;
        float e = smoothstep(0.0f, 1.0f, t);
        eyeX = lerpf(-3.40f, -3.20f, e);
        eyeY = lerpf( 0.70f,  0.60f, e);
        eyeZ = lerpf( 6.40f,  6.20f, e);

        targetX = lerpf(-3.40f, -3.20f, e);
        targetY = lerpf(16.00f, 14.50f, e); // looking high up into black void
        targetZ = lerpf( 6.40f,  6.20f, e);
    } else if (seqTime < 15.0f) {
        // Phase 2: Ultra-slow cinematic tilt down bringing sunlit Mars into frame (9 seconds)
        float t = (seqTime - 6.0f) / 9.0f;
        float e = smoothstep(0.0f, 1.0f, t);

        eyeX = lerpf(-3.20f, -3.10f, e);
        eyeY = lerpf( 0.60f,  0.55f, e);
        eyeZ = lerpf( 6.20f,  6.10f, e);

        targetX = lerpf(-3.20f, 0.20f, e);
        targetY = lerpf(14.50f, 0.05f, e);
        targetZ = lerpf( 6.20f, 0.00f, e);
    } else {
        // Phase 3: The Interstellar Scale Shot holds on sunlit Mars with tiny dot spacecraft
        float t = (seqTime - 15.0f) / 11.0f;
        float e = smoothstep(0.0f, 1.0f, t);

        eyeX = lerpf(-3.10f, -3.25f, e);
        eyeY = lerpf( 0.55f,  0.58f, e);
        eyeZ = lerpf( 6.10f,  5.95f, e);

        targetX = lerpf(0.20f, 0.25f, e);
        targetY = lerpf(0.05f, 0.07f, e);
        targetZ = lerpf(0.00f, 0.02f, e);
    }
}

