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
        // SHOT 2: DESCENT FROM RIGHT TO LEFT INTO DEPTH (39.0s - 46.0s)
        // Starts on right side of screen, angled with roof tilted toward camera,
        // and travels left and deep into Mars (-Z depth).
        float u = (t - 39.0f) / 7.0f;
        float smoothU = smoothstep(0.0f, 1.0f, u);

        rangerPosX = lerpf( 2.20f, -0.60f, smoothU);
        rangerPosY = lerpf( 0.45f, -0.15f, smoothU);
        rangerPosZ = lerpf( 2.50f,  0.50f, smoothU); // travels deep into Mars atmosphere

        // Direction: travelling left (-X), slightly down (-Y), and away into depth (-Z)
        rangerDirX = -0.75f;
        rangerDirY = -0.16f;
        rangerDirZ = -0.64f;
        normalize3(rangerDirX, rangerDirY, rangerDirZ);

        // Roof tilted toward user screen: roll around flight axis + pitch
        rangerPitchExtra = 12.0f;
        rangerRollAngle  = 55.0f;

        rcsActive = 0.0f;
        mainBurnActive = (t >= 39.2f) ? 1.0f : 0.0f;
        heatshieldThermal = 0.0f;

        // Exhaust trail shooting backward (+X, +Y, +Z)
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

        rangerPosX = lerpf(-0.60f, -1.95f, u);
        rangerPosY = lerpf(-0.15f, -0.48f, u);
        rangerPosZ = lerpf( 0.50f, -1.80f, u); // deep distance into Mars horizon

        rangerDirX = -0.68f;
        rangerDirY = -0.16f;
        rangerDirZ = -0.71f;
        normalize3(rangerDirX, rangerDirY, rangerDirZ);

        rangerPitchExtra = 12.0f;
        rangerRollAngle  = 50.0f;

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
        // SHOT 2 (39.0s - 46.0s): Camera framing Ranger entering from right, Mars horizon
        float u = (t - 39.0f) / 7.0f;
        eyeX = lerpf( 0.20f,  0.30f, u);
        eyeY = lerpf( 0.95f,  0.90f, u);
        eyeZ = lerpf( 4.40f,  4.20f, u);

        targetX = lerpf( 0.30f, -0.20f, u);
        targetY = -0.10f;
        targetZ =  0.60f;
    }
    else {
        // SHOT 3 (46.0s - 56.0s): Wide camera view watching Ranger recede into distant Mars curve
        float u = (t - 46.0f) / 10.0f;
        eyeX = lerpf( 0.30f,  0.35f, u);
        eyeY = lerpf( 0.90f,  0.85f, u);
        eyeZ = lerpf( 4.20f,  4.10f, u);

        targetX = lerpf(-0.20f, -1.20f, u);
        targetY = lerpf(-0.10f, -0.30f, u);
        targetZ = lerpf( 0.60f, -0.60f, u);
    }
}
