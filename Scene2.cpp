#include "Scene2.h"
#include "SceneDirector.h"
#include "Common.h"
#include "Spacecraft.h"
#include <cmath>

// ============================================================================
// SCENE 2: STORYBOARD IMPLEMENTATION
// ============================================================================
// Shot 1 (35.0s - 39.0s): Title fades, Ranger emerges from backside of Endurance
// Shot 2 (39.0s - 46.0s): Sudden blackout cut -> Side profile, Mars on right,
//                        Ranger on left flying toward Mars with rear engines firing
// Shot 3 (46.0s - 56.0s): "It keeps going on" -> Ranger recedes into the distance
//                        becoming a tiny dot (speck) against the giant Mars curve
// ============================================================================

void updateScene2(float dt) {
    rangerDetached = true;

    // Endurance stays in low Mars orbit
    endPosX = 0.0f;
    endPosY = -0.60f;
    endPosZ = 2.45f;
    endTanX = 0.0f; endTanY = 0.0f; endTanZ = 1.0f;

    float t = seqTime;

    if (t < 39.0f) {
        // SHOT 1: BACKSIDE EMERGENCE (35.0s - 39.0s)
        // Ranger unlatches and flies out from the rear/underneath the Endurance hub
        float u = (t - 35.0f) / 4.0f;
        float smoothU = smoothstep(0.0f, 1.0f, u);

        rangerPosX = lerpf(0.0f, -0.15f, smoothU);
        rangerPosY = lerpf(-0.60f, -0.88f, smoothU); // drops down out from hub
        rangerPosZ = lerpf(2.45f, 2.18f, smoothU);  // emerges towards backside

        // Heading away and slightly downward
        rangerDirX = -0.15f;
        rangerDirY = -0.45f;
        rangerDirZ = -0.88f;
        normalize3(rangerDirX, rangerDirY, rangerDirZ);

        rangerPitchExtra = lerpf(0.0f, -18.0f, smoothU);
        rangerRollAngle  = lerpf(0.0f, 10.0f, smoothU);

        rcsActive = (t >= 35.5f && t <= 38.5f) ? 1.0f : 0.0f;
        mainBurnActive = 0.0f;
        heatshieldThermal = 0.0f;

        if (rcsActive > 0.0f && (rand() % 2 == 0)) {
            float side = (rand() % 2 == 0) ? -1.0f : 1.0f;
            spawnParticle(rangerPosX + side * 0.12f, rangerPosY, rangerPosZ,
                          side * 0.3f, -0.1f, 0.2f, 0.5f, 2.0f, 0.85f, 0.95f, 1.0f, true);
        }
    }
    else if (t < 46.0f) {
        // SHOT 2: DESCENT FROM SCREEN RIGHT TOWARD MARS ON THE RIGHT (39.0s - 46.0s)
        // Ranger is on the right side of the screen, oriented in a front 3/4 view
        // (nose pointed toward Mars and downward, roof tilted up toward camera),
        // descending from right to left & receding deep into distance (-Z).
        float u = (t - 39.0f) / 7.0f;
        float smoothU = smoothstep(0.0f, 1.0f, u);

        rangerPosX = lerpf( 0.45f, -0.65f, smoothU);
        rangerPosY = lerpf( 0.65f,  0.10f, smoothU);
        rangerPosZ = lerpf( 2.20f,  0.80f, smoothU);

        // Direction: moving toward -X, downward -Y, and away -Z
        rangerDirX = -0.78f;
        rangerDirY = -0.38f;
        rangerDirZ = -0.49f;
        normalize3(rangerDirX, rangerDirY, rangerDirZ);

        // Front 3/4 view: bank so top roof facets face the camera from above
        rangerPitchExtra = -8.0f;
        rangerRollAngle  = -35.0f;

        rcsActive = 0.0f;
        mainBurnActive = (t >= 39.2f) ? 1.0f : 0.0f;
        heatshieldThermal = 0.0f;

        // Exhaust trail
        if (mainBurnActive > 0.0f) {
            float exX = rangerPosX - rangerDirX * 0.25f;
            float exY = rangerPosY - rangerDirY * 0.25f;
            float exZ = rangerPosZ - rangerDirZ * 0.25f;
            for (int k = 0; k < 2; ++k) {
                spawnParticle(exX + randRange(-0.015f, 0.015f),
                              exY + randRange(-0.015f, 0.015f),
                              exZ + randRange(-0.015f, 0.015f),
                              -rangerDirX * 1.3f + randRange(-0.05f, 0.05f),
                              -rangerDirY * 1.3f + randRange(-0.05f, 0.05f),
                              -rangerDirZ * 1.3f + randRange(-0.05f, 0.05f),
                              0.38f, randRange(2.0f, 3.5f),
                              0.35f, 0.75f, 1.0f, true);
            }
        }
    }
    else {
        // SHOT 3: RECEDING DEEP INTO MARS HORIZON -> SHRINKING TO PINPOINT DOT (46.0s - 56.0s)
        float u = (t - 46.0f) / 10.0f;

        rangerPosX = lerpf(-0.65f, -1.85f, u);
        rangerPosY = lerpf( 0.10f, -0.45f, u);
        rangerPosZ = lerpf( 0.80f, -1.50f, u);

        rangerDirX = -0.72f;
        rangerDirY = -0.30f;
        rangerDirZ = -0.62f;
        normalize3(rangerDirX, rangerDirY, rangerDirZ);

        rangerPitchExtra = -8.0f;
        rangerRollAngle  = -30.0f;

        rcsActive = 0.0f;
        mainBurnActive = 1.0f;
        heatshieldThermal = smoothstep(48.0f, 54.0f, t) * 0.45f;

        if (rand() % 2 == 0) {
            float exX = rangerPosX - rangerDirX * 0.20f;
            float exY = rangerPosY - rangerDirY * 0.20f;
            float exZ = rangerPosZ - rangerDirZ * 0.20f;
            spawnParticle(exX, exY, exZ,
                          -rangerDirX * 1.1f, -rangerDirY * 1.1f, -rangerDirZ * 1.1f,
                          0.30f, 2.0f, 0.4f, 0.8f, 1.0f, true);
        }
    }

    // Update active particles life
    for (size_t i = 0; i < particles.size(); ) {
        particles[i].x += particles[i].vx * dt;
        particles[i].y += particles[i].vy * dt;
        particles[i].z += particles[i].vz * dt;
        particles[i].life -= dt;
        if (particles[i].life <= 0.0f) {
            particles[i] = particles.back();
            particles.pop_back();
        } else {
            ++i;
        }
    }
}

void computeCameraScene2(float& eyeX, float& eyeY, float& eyeZ,
                        float& targetX, float& targetY, float& targetZ,
                        float& upX, float& upY, float& upZ) {
    upX = 0.0f; upY = 1.0f; upZ = 0.0f;
    float t = seqTime;

    if (t < 39.0f) {
        // SHOT 1 (35.0s - 39.0s): Match Scene 1 hero position
        // Looking at the Endurance as Ranger drops out behind it
        float u = (t - 35.0f) / 4.0f;
        eyeX = lerpf(0.0f, 0.05f, u);
        eyeY = lerpf(-0.60f, -0.55f, u);
        eyeZ = lerpf(2.92f, 2.96f, u);

        targetX = 0.0f;
        targetY = -0.60f;
        targetZ = 2.45f;
    }
    else if (t < 46.0f) {
        // SHOT 2 (39.0s - 46.0s): Mars placed on right half of screen
        // Camera positioned to the left & slightly above, viewing Mars on right
        float u = (t - 39.0f) / 7.0f;
        eyeX = lerpf(-1.85f, -1.75f, u);
        eyeY = lerpf( 1.15f,  1.05f, u);
        eyeZ = lerpf( 4.10f,  3.90f, u);

        targetX = lerpf(-0.25f, -0.45f, u);
        targetY =  0.05f;
        targetZ =  0.80f;
    }
    else {
        // SHOT 3 (46.0s - 56.0s): Receding Over Mars Surface toward distant curve
        float u = (t - 46.0f) / 10.0f;
        eyeX = lerpf(-1.75f, -1.65f, u);
        eyeY = lerpf( 1.05f,  0.95f, u);
        eyeZ = lerpf( 3.90f,  3.80f, u);

        targetX = lerpf(-0.45f, -1.15f, u);
        targetY = lerpf( 0.05f, -0.20f, u);
        targetZ = lerpf( 0.80f, -0.50f, u);
    }
}
