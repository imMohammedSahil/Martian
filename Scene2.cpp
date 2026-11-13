#include "Scene2.h"
#include "SceneDirector.h"
#include "Common.h"
#include "Spacecraft.h"
#include <cmath>

// ============================================================================
// SCENE 2: LANDER UNDOCKING, DE-ORBIT BURN & HYPERSONIC ENTRY (EXTERIOR)
// ============================================================================
// Phase 1 (35.0s - 42.0s): Undocking from Endurance, cold-gas RCS puffs, pitch
// Phase 2 (42.0s - 48.0s): De-orbit retro burn with dual electric-blue ion plumes
// Phase 3 (48.0s - 55.0s): Hypersonic entry, incandescent heatshield & plasma cone
// ============================================================================

void updateScene2(float dt) {
    rangerDetached = true;

    // Base orbital position of Endurance
    endPosX = 0.0f;
    endPosY = -0.60f;
    endPosZ = 2.45f;
    endTanX = 0.0f; endTanY = 0.0f; endTanZ = 1.0f;

    float t = seqTime;

    if (t < 42.0f) {
        // PHASE 1: UNDOCKING & SEPARATION (35.0s - 42.0s)
        float u = (t - 35.0f) / 7.0f;
        float smoothU = smoothstep(0.0f, 1.0f, u);

        rangerPosX = 0.0f;
        rangerPosY = lerpf(-0.60f, -0.95f, smoothU);
        rangerPosZ = lerpf(2.45f, 2.60f, smoothU);

        rangerDirX = 0.0f;
        rangerDirY = lerpf(0.0f, -0.42f, smoothU);
        rangerDirZ = lerpf(1.0f, 0.90f, smoothU);
        normalize3(rangerDirX, rangerDirY, rangerDirZ);

        rangerPitchExtra = lerpf(0.0f, -24.0f, smoothU);
        rangerRollAngle  = lerpf(0.0f, 8.0f, smoothU);

        rcsActive = (t >= 35.5f && t <= 39.5f) ? 1.0f : 0.0f;
        mainBurnActive = 0.0f;
        heatshieldThermal = 0.0f;

        if (rcsActive > 0.0f && (rand() % 3 == 0)) {
            float side = (rand() % 2 == 0) ? -1.0f : 1.0f;
            spawnParticle(rangerPosX + side * 0.15f, rangerPosY + 0.02f, rangerPosZ - 0.1f,
                          side * 0.4f, 0.1f, -0.2f, 0.6f, 2.2f, 0.85f, 0.95f, 1.0f, true);
        }
    }
    else if (t < 48.0f) {
        // PHASE 2: DE-ORBIT RETRO BURN (42.0s - 48.0s)
        float u = (t - 42.0f) / 6.0f;
        float smoothU = smoothstep(0.0f, 1.0f, u);

        rangerPosX = lerpf(0.0f, 0.08f, smoothU);
        rangerPosY = lerpf(-0.95f, -1.45f, smoothU);
        rangerPosZ = lerpf(2.60f, 2.30f, smoothU);

        rangerDirX = 0.05f;
        rangerDirY = lerpf(-0.42f, -0.68f, smoothU);
        rangerDirZ = lerpf(0.90f, 0.73f, smoothU);
        normalize3(rangerDirX, rangerDirY, rangerDirZ);

        rangerPitchExtra = -26.0f;
        rangerRollAngle  = 8.0f + 4.0f * sinf(u * 3.14159f);

        rcsActive = 0.0f;
        mainBurnActive = (t >= 42.4f && t <= 47.6f) ? 1.0f : 0.0f;
        heatshieldThermal = smoothstep(46.0f, 48.0f, t) * 0.25f;

        if (mainBurnActive > 0.0f) {
            for (int k = 0; k < 3; ++k) {
                float side = (k % 2 == 0) ? -0.08f : 0.08f;
                spawnParticle(rangerPosX + side + randRange(-0.01f, 0.01f),
                              rangerPosY + 0.01f + randRange(-0.01f, 0.01f),
                              rangerPosZ - 0.28f - randRange(0.0f, 0.05f),
                              randRange(-0.05f, 0.05f), -0.2f, -0.8f,
                              0.5f, randRange(2.5f, 4.5f),
                              0.4f, 0.75f, 1.0f, true);
            }
        }
    }
    else {
        // PHASE 3: HYPERSONIC ENTRY & PLASMA FIRE (48.0s - 55.0s)
        float u = (t - 48.0f) / 7.0f;

        rangerPosX = 0.08f + u * 0.08f;
        rangerPosY = lerpf(-1.45f, -2.15f, u);
        rangerPosZ = lerpf(2.30f, 1.95f, u);

        rangerDirX = 0.08f;
        rangerDirY = -0.76f;
        rangerDirZ = 0.65f;
        normalize3(rangerDirX, rangerDirY, rangerDirZ);

        rangerPitchExtra = -28.0f + 1.5f * sinf(t * 18.0f); // aerodynamic buffeting
        rangerRollAngle  = 12.0f + 1.2f * cosf(t * 15.0f);

        rcsActive = 0.0f;
        mainBurnActive = 0.0f;
        heatshieldThermal = lerpf(0.25f, 1.0f, smoothstep(48.0f, 52.0f, t));

        // Flaming embers streaming into wake
        for (int k = 0; k < 5; ++k) {
            spawnParticle(rangerPosX + randRange(-0.15f, 0.15f),
                          rangerPosY - 0.06f + randRange(-0.04f, 0.02f),
                          rangerPosZ + randRange(-0.1f, 0.2f),
                          randRange(-0.1f, 0.1f), 0.3f, -1.2f,
                          0.45f, randRange(3.0f, 6.0f),
                          1.0f, randRange(0.35f, 0.75f), 0.15f, true);
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

    if (t < 42.0f) {
        // Phase 1 (35.0s - 42.0s): Docking Hub Separation View
        float u = (t - 35.0f) / 7.0f;
        eyeX = lerpf(0.0f, -0.28f, u);
        eyeY = lerpf(-0.52f, -0.72f, u);
        eyeZ = lerpf(2.88f, 3.08f, u);

        targetX = rangerPosX * 0.6f;
        targetY = rangerPosY;
        targetZ = rangerPosZ;
    }
    else if (t < 48.0f) {
        // Phase 2 (42.0s - 48.0s): Dynamic Retro Burn 3/4 Orbit Chase
        float u = (t - 42.0f) / 6.0f;
        eyeX = lerpf(0.38f, 0.52f, u);
        eyeY = lerpf(-0.80f, -1.22f, u);
        eyeZ = lerpf(2.95f, 2.70f, u);

        targetX = rangerPosX;
        targetY = rangerPosY;
        targetZ = rangerPosZ;
    }
    else {
        // Phase 3 (48.0s - 55.0s): External Hypersonic Plasma Chase with IMAX shake
        float u = (t - 48.0f) / 7.0f;

        float shake = smoothstep(48.0f, 51.0f, t) * 0.012f;
        float sx = sinf(t * 35.0f) * shake;
        float sy = cosf(t * 42.0f) * shake;

        eyeX = lerpf(-0.45f, -0.58f, u) + sx;
        eyeY = lerpf(-1.40f, -2.05f, u) + sy;
        eyeZ = lerpf(2.65f, 2.30f, u);

        targetX = rangerPosX + sx * 0.5f;
        targetY = rangerPosY + sy * 0.5f;
        targetZ = rangerPosZ;
    }
}
