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
    // Spacecraft positioned directly in front of the illuminated Martian surface
    float t = seqTime / 15.0f;
    endPosX = lerpf(-2.70f, -2.42f, t);
    endPosY = lerpf( 0.54f,  0.42f, t);
    endPosZ = lerpf( 2.78f,  2.50f, t);
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
    if (seqTime < 3.5f) {
        // Looking out into deep pitch-black space void
        float t = seqTime / 3.5f;
        float e = smoothstep(0.0f, 1.0f, t);
        eyeX = lerpf(-7.20f, -6.80f, e);
        eyeY = lerpf( 2.80f,  2.20f, e);
        eyeZ = lerpf( 7.50f,  7.00f, e);

        targetX = lerpf(-14.00f, -13.50f, e);
        targetY = lerpf(  6.00f,   5.50f, e);
        targetZ = lerpf( 10.00f,   9.50f, e);

        upX = 0.06f; upY = 0.998f; upZ = 0.0f;
    } else if (seqTime < 7.5f) {
        // Slow majestic pan bringing Mars into full frame
        float t = (seqTime - 3.5f) / 4.0f;
        float e = smoothstep(0.0f, 1.0f, t);

        eyeX = lerpf(-6.80f, -4.85f, e);
        eyeY = lerpf( 2.20f,  1.05f, e);
        eyeZ = lerpf( 7.00f,  5.15f, e);

        targetX = lerpf(-13.50f,  0.22f, e);
        targetY = lerpf(  5.50f,  0.06f, e);
        targetZ = lerpf(  9.50f,  0.08f, e);

        upX = lerpf(0.06f, -0.04f, e);
        upY = lerpf(0.998f, 0.999f, e);
        upZ = 0.0f;
    } else {
        // The Interstellar Scale Shot: Mars fills screen, spacecraft is tiny dot in front
        float t = (seqTime - 7.5f) / 7.5f;
        float e = smoothstep(0.0f, 1.0f, t);

        eyeX = lerpf(-4.85f, -5.10f, e);
        eyeY = lerpf( 1.05f,  1.12f, e);
        eyeZ = lerpf( 5.15f,  4.95f, e);

        targetX = lerpf(0.22f, 0.28f, e);
        targetY = lerpf(0.06f, 0.08f, e);
        targetZ = lerpf(0.08f, 0.12f, e);

        upX = -0.04f; upY = 0.999f; upZ = 0.0f;
    }
}

