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
        // SHOT 2: SIDE PROFILE CRUISE (39.0s - 46.0s)
        // Mars curve on TOP-RIGHT, Ranger on left traveling rightward toward Mars
        // Stays strictly in upper atmosphere (altitude > 2.26, Mars radius is 2.20)
        float u = (t - 39.0f) / 7.0f;
        float smoothU = smoothstep(0.0f, 1.0f, u);

        // Ranger flies from left to right across the screen above Mars surface
        rangerPosX = lerpf(-2.40f, -0.60f, smoothU);
        rangerPosY = lerpf(-0.60f, -0.52f, smoothU);
        rangerPosZ = lerpf( 2.30f,  2.26f, smoothU);

        // Directed horizontally rightward toward Mars
        rangerDirX = 0.998f;
        rangerDirY = 0.04f;
        rangerDirZ = -0.02f;
        normalize3(rangerDirX, rangerDirY, rangerDirZ);

        rangerPitchExtra = -2.0f;
        rangerRollAngle  = 4.0f;

        rcsActive = 0.0f;
        mainBurnActive = (t >= 39.2f) ? 1.0f : 0.0f;
        heatshieldThermal = 0.0f;

        // Twin main propulsion trail particles shooting backwards to the left
        if (mainBurnActive > 0.0f) {
            float exX = rangerPosX - rangerDirX * 0.35f;
            float exY = rangerPosY - rangerDirY * 0.35f;
            float exZ = rangerPosZ - rangerDirZ * 0.35f;
            for (int k = 0; k < 2; ++k) {
                spawnParticle(exX + randRange(-0.02f, 0.02f),
                              exY + randRange(-0.02f, 0.02f),
                              exZ + randRange(-0.02f, 0.02f),
                              -rangerDirX * 1.5f + randRange(-0.08f, 0.08f),
                              -rangerDirY * 1.5f + randRange(-0.04f, 0.04f),
                              -rangerDirZ * 1.5f + randRange(-0.08f, 0.08f),
                              0.4f, randRange(2.5f, 4.0f),
                              0.35f, 0.75f, 1.0f, true);
            }
        }
    }
    else {
        // SHOT 3: RECEDING INTO DISTANCE (46.0s - 56.0s)
        // "It keeps going on -> small dot it becomes"
        // Curves over the Martian horizon into the deep background (altitude ~2.38 > 2.20)
        float u = (t - 46.0f) / 10.0f;

        rangerPosX = lerpf(-0.60f,  0.95f, u);
        rangerPosY = lerpf(-0.52f, -0.42f, u);
        rangerPosZ = lerpf( 2.26f,  2.16f, u);

        rangerDirX = 0.998f;
        rangerDirY = 0.04f;
        rangerDirZ = -0.02f;
        normalize3(rangerDirX, rangerDirY, rangerDirZ);

        rangerPitchExtra = -2.0f;
        rangerRollAngle  = 3.0f;

        rcsActive = 0.0f;
        mainBurnActive = 1.0f; // continuous cruise burn
        heatshieldThermal = smoothstep(49.0f, 54.0f, t) * 0.35f;

        // Faint wake trail
        if (rand() % 2 == 0) {
            float exX = rangerPosX - rangerDirX * 0.35f;
            float exY = rangerPosY;
            float exZ = rangerPosZ - rangerDirZ * 0.35f;
            spawnParticle(exX, exY, exZ,
                          -rangerDirX * 1.2f, 0.0f, -rangerDirZ * 1.2f,
                          0.35f, 2.2f, 0.4f, 0.8f, 1.0f, true);
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
        // SHOT 2 (39.0s - 46.0s): Side Profile Shot (Panel 2)
        // Mars curve on TOP-RIGHT, Ranger on the left flying rightward
        float u = (t - 39.0f) / 7.0f;
        eyeX = lerpf(-0.60f, -0.50f, u);
        eyeY = lerpf(-0.85f, -0.80f, u);
        eyeZ = lerpf( 3.80f,  3.70f, u);

        targetX = lerpf(-1.50f, -1.25f, u);
        targetY = -0.60f;
        targetZ = 1.60f;
    }
    else {
        // SHOT 3 (46.0s - 56.0s): The Great Martian Expanse (Panel 3)
        // Camera holds wide as Ranger shrinks into a tiny dot near the top-right Mars curve
        float u = (t - 46.0f) / 10.0f;
        eyeX = lerpf(-0.50f, -0.40f, u);
        eyeY = lerpf(-0.80f, -0.75f, u);
        eyeZ = lerpf( 3.70f,  3.75f, u);

        targetX = lerpf(-1.25f, -0.90f, u);
        targetY = -0.55f;
        targetZ = 1.55f;
    }
}
