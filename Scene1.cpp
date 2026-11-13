#include "SceneDirector.h"
#include "Common.h"

// ============================================================================
// SCENE 1: 4-PANEL STORYBOARD CHOREOGRAPHY
// ============================================================================
// - Panel 1 (0.0s -  5.0s): Void black space with faint stars
// - Panel 2 (5.0s - 11.5s): Smooth glide to reveal full sunlit Mars disc
// - Panel 3 (11.5s - 19.5s): Interstellar horizon bowl scale shot with micro craft dot
// - Panel 4 (19.5s - 28.0s): Detailed 3D spinning Endurance centered inside "M A R T I A N"
// ============================================================================

void updateScene1(float dt) {
    if (seqTime < 11.5f) {
        // Panels 1 & 2: Craft is off-screen / dormant in distance
        endPosX = 0.0f;
        endPosY = 1.15f;
        endPosZ = 1.50f;
    } else if (seqTime < 19.5f) {
        // Panel 3: Interstellar scale pinpoint dot centered in Mars horizon bowl
        float t = (seqTime - 11.5f) / 8.0f;
        endPosX = lerpf(-0.05f, 0.05f, t);
        endPosY = lerpf( 1.18f, 1.12f, t);
        endPosZ = 1.50f;
    } else {
        // Panel 4: Heroic 3D Endurance dead-center in front of camera
        float t = (seqTime - 19.5f) / 8.5f;
        float e = smoothstep(0.0f, 1.0f, t);
        endPosX = 0.0f;
        endPosY = lerpf(-0.02f, 0.02f, e);
        endPosZ = 3.60f;
    }
    endTanX = 0.0f; endTanY = 0.0f; endTanZ = 1.0f;

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

    if (seqTime < 5.0f) {
        // Panel 1: Pure deep-black cosmic void (looking away into deep space starfield)
        float t = seqTime / 5.0f;
        float e = smoothstep(0.0f, 1.0f, t);
        eyeX = lerpf(-3.90f, -3.80f, e);
        eyeY = lerpf( 0.00f,  0.00f, e);
        eyeZ = lerpf( 7.60f,  7.50f, e);

        targetX = lerpf(18.00f, 16.50f, e); // looking into starfield (+X)
        targetY = 0.00f;
        targetZ = lerpf( 7.60f,  7.50f, e);
    } else if (seqTime < 11.5f) {
        // Panel 2: Glided smoothly to Mars (camera pans from void to full center Mars disc)
        float t = (seqTime - 5.0f) / 6.5f;
        float e = smoothstep(0.0f, 1.0f, t);

        eyeX = lerpf(-3.80f, -3.85f, e);
        eyeY = 0.00f;
        eyeZ = lerpf( 7.80f,  7.60f, e);

        targetX = lerpf(16.50f, 0.00f, e);
        targetY = 0.00f;
        targetZ = 0.00f;
    } else if (seqTime < 19.5f) {
        // Panel 3: Interstellar scale shot (Mars lower curved bowl + micro craft dot centered)
        float t = (seqTime - 11.5f) / 8.0f;
        float e = smoothstep(0.0f, 1.0f, t);

        eyeX = lerpf(0.00f, 0.04f, e);
        eyeY = lerpf(1.80f, 1.76f, e);
        eyeZ = lerpf(3.50f, 3.45f, e);

        targetX = 0.00f;
        targetY = 0.75f;
        targetZ = 0.00f;
    } else {
        // Panel 4: Heroic close-up on spinning Endurance centered inside "M A R T I A N"
        float t = (seqTime - 19.5f) / 8.5f;
        float e = smoothstep(0.0f, 1.0f, t);

        eyeX = lerpf(0.00f, 0.03f, e);
        eyeY = 0.00f;
        eyeZ = lerpf(5.20f, 5.10f, e);

        targetX = 0.00f;
        targetY = 0.00f;
        targetZ = 3.60f; // looking directly at Endurance
    }
}

