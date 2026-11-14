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
    else {
        // SHOTS 2 & 3: CONTINUOUS SMOOTH CURVING FLIGHT PATH INTO MARS (39.0s - 56.0s)
        // 17 seconds of single continuous Bezier flight — zero cuts, zero stops, zero jerks.
        float u = (t - 39.0f) / 17.0f; // 0.0 at 39s -> 1.0 at 56s
        u = clampf(u, 0.0f, 1.0f);

        // Cubic Bezier curve control points:
        // Calibrated start for scale 0.50: enters cleanly from off-screen bottom-left
        const float p0x =  -5.20f, p0y = -0.65f, p0z =  2.60f;
        const float p1x =  -3.30f, p1y = -0.10f, p1z =  2.15f;
        const float p2x =  -2.40f, p2y =  0.25f, p2z =  1.65f;
        const float p3x =  -1.55f, p3y = -0.15f, p3z =  1.85f;

        float invU = 1.0f - u;
        float b0 = invU * invU * invU;
        float b1 = 3.0f * invU * invU * u;
        float b2 = 3.0f * invU * u * u;
        float b3 = u * u * u;

        rangerPosX = b0 * p0x + b1 * p1x + b2 * p2x + b3 * p3x;
        rangerPosY = b0 * p0y + b1 * p1y + b2 * p2y + b3 * p3y;
        rangerPosZ = b0 * p0z + b1 * p1z + b2 * p2z + b3 * p3z;

        // Exact analytical tangent vector (derivative dP/du) ensures continuous smooth heading
        float d0 = -3.0f * invU * invU;
        float d1 =  3.0f * invU * (1.0f - 3.0f * u);
        float d2 =  3.0f * u * (2.0f - 3.0f * u);
        float d3 =  3.0f * u * u;

        rangerDirX = d0 * p0x + d1 * p1x + d2 * p2x + d3 * p3x;
        rangerDirY = d0 * p0y + d1 * p1y + d2 * p2y + d3 * p3y;
        rangerDirZ = d0 * p0z + d1 * p1z + d2 * p2z + d3 * p3z;
        normalize3(rangerDirX, rangerDirY, rangerDirZ);

        // Smooth continuous roll & pitch banking into the curve
        rangerPitchExtra = -8.0f;
        rangerRollAngle  = lerpf(28.0f, 18.0f, u);

        rcsActive = 0.0f;
        mainBurnActive = (t >= 39.1f && t < 55.0f) ? 1.0f : 0.0f;
        heatshieldThermal = smoothstep(46.0f, 54.0f, t) * 0.45f;

        // Exhaust trail particles along the continuous curve
        if (mainBurnActive > 0.0f && rangerPosX > -6.0f && (rand() % 2 == 0)) {
            float exX = rangerPosX - rangerDirX * 0.22f;
            float exY = rangerPosY - rangerDirY * 0.22f;
            float exZ = rangerPosZ - rangerDirZ * 0.22f;
            spawnParticle(exX + randRange(-0.015f, 0.015f),
                          exY + randRange(-0.015f, 0.015f),
                          exZ + randRange(-0.015f, 0.015f),
                          -rangerDirX * 1.2f + randRange(-0.05f, 0.05f),
                          -rangerDirY * 1.2f + randRange(-0.05f, 0.05f),
                          -rangerDirZ * 1.2f + randRange(-0.05f, 0.05f),
                          0.35f, randRange(2.0f, 3.5f),
                          0.35f, 0.75f, 1.0f, true);
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
        // SHOT 2 (39.0s - 46.0s): Mars firmly placed on right edge of viewport
        float u = (t - 39.0f) / 7.0f;
        eyeX = lerpf(-3.25f, -3.15f, u);
        eyeY = lerpf( 0.90f,  0.85f, u);
        eyeZ = lerpf( 4.05f,  3.95f, u);

        targetX = lerpf(-3.45f, -3.35f, u);
        targetY = -0.20f;
        targetZ =  0.85f;
    }
    else {
        // SHOT 3 (46.0s - 56.0s): Receding Over Mars Surface on the right
        float u = (t - 46.0f) / 10.0f;
        eyeX = lerpf(-3.15f, -3.05f, u);
        eyeY = lerpf( 0.85f,  0.80f, u);
        eyeZ = lerpf( 3.95f,  4.00f, u);

        targetX = lerpf(-3.35f, -3.15f, u);
        targetY = -0.25f;
        targetZ =  0.80f;
    }
}
