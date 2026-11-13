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
    // Spacecraft positioned directly in front of the sunlit orange Martian face
    float t = seqTime / 26.0f;
    endPosX = lerpf(-1.65f, -1.45f, t);
    endPosY = lerpf( 0.22f,  0.15f, t);
    endPosZ = lerpf( 3.35f,  3.05f, t);
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
        // Phase 1: Pure deep-black space void (looking away into deep cosmic darkness)
        float t = seqTime / 6.0f;
        float e = smoothstep(0.0f, 1.0f, t);
        eyeX = lerpf(-3.90f, -3.80f, e);
        eyeY = lerpf( 0.55f,  0.50f, e);
        eyeZ = lerpf( 7.80f,  7.60f, e);

        targetX = lerpf(18.00f, 16.50f, e); // looking into pure black starfield (+X)
        targetY = lerpf( 0.55f,  0.50f, e);
        targetZ = lerpf( 7.80f,  7.60f, e);
    } else if (seqTime < 15.0f) {
        // Phase 2: Ultra-slow cinematic pan bringing sunlit Mars fully into frame (9 seconds)
        float t = (seqTime - 6.0f) / 9.0f;
        float e = smoothstep(0.0f, 1.0f, t);

        eyeX = lerpf(-3.80f, -3.75f, e);
        eyeY = lerpf( 0.50f,  0.00f, e);
        eyeZ = lerpf( 7.60f,  7.50f, e);

        targetX = lerpf(16.50f, 0.00f, e);
        targetY = lerpf( 0.50f, 0.00f, e);
        targetZ = lerpf( 7.60f, 0.00f, e);
    } else {
        // Phase 3: The Interstellar Scale Shot holds (Full Mars disc framed dead-center)
        float t = (seqTime - 15.0f) / 11.0f;
        float e = smoothstep(0.0f, 1.0f, t);

        eyeX = lerpf(-3.75f, -3.85f, e);
        eyeY = lerpf( 0.00f,  0.00f, e);
        eyeZ = lerpf( 7.50f,  7.40f, e);

        targetX = 0.00f;
        targetY = 0.00f;
        targetZ = 0.00f;
    }
}

