#include "Scene2.h"
#include "SceneDirector.h"
#include "Common.h"
#include "Spacecraft.h"
#include <cmath>
#include <cstdio>

// ============================================================================
// SCENE 2: LANDER UNDOCKING, DE-ORBIT BURN & HYPERSONIC ENTRY
// ============================================================================
// Phase 1 (35.0s - 41.0s): Undocking from Endurance, RCS separation puff, pitch
// Phase 2 (41.0s - 47.0s): De-orbit retro burn with blue/white ion flare
// Phase 3 (47.0s - 53.0s): Interior Cockpit HUD cut (CRT avionics & telemetry)
// Phase 4 (53.0s - 60.0s): Exterior hypersonic entry, glowing heatshield & plasma
// ============================================================================

void updateScene2(float dt) {
    rangerDetached = true;

    // Base orbital position of Endurance
    endPosX = 0.0f;
    endPosY = -0.60f;
    endPosZ = 2.45f;
    endTanX = 0.0f; endTanY = 0.0f; endTanZ = 1.0f;

    float t = seqTime;

    if (t < 41.0f) {
        // PHASE 1: UNDOCKING & SEPARATION (35.0s - 41.0s)
        float u = (t - 35.0f) / 6.0f;
        float smoothU = smoothstep(0.0f, 1.0f, u);

        // Ranger separates downward and slightly forward
        rangerPosX = 0.0f;
        rangerPosY = lerpf(-0.60f, -0.92f, smoothU);
        rangerPosZ = lerpf(2.45f, 2.58f, smoothU);

        rangerDirX = 0.0f;
        rangerDirY = lerpf(0.0f, -0.42f, smoothU);
        rangerDirZ = lerpf(1.0f, 0.90f, smoothU);
        normalize3(rangerDirX, rangerDirY, rangerDirZ);

        rangerPitchExtra = lerpf(0.0f, -22.0f, smoothU);
        rangerRollAngle  = lerpf(0.0f, 8.0f, smoothU);

        // Cold-gas RCS puffs during initial separation
        rcsActive = (t >= 35.5f && t <= 39.0f) ? 1.0f : 0.0f;
        mainBurnActive = 0.0f;
        heatshieldThermal = 0.0f;

        if (rcsActive > 0.0f && (rand() % 3 == 0)) {
            // Emit subtle cold-gas RCS particles
            float side = (rand() % 2 == 0) ? -1.0f : 1.0f;
            spawnParticle(rangerPosX + side * 0.15f, rangerPosY + 0.02f, rangerPosZ - 0.1f,
                          side * 0.4f, 0.1f, -0.2f, 0.6f, 2.2f, 0.85f, 0.95f, 1.0f, true);
        }
    }
    else if (t < 47.0f) {
        // PHASE 2: DE-ORBIT RETRO BURN (41.0s - 47.0s)
        float u = (t - 41.0f) / 6.0f;
        float smoothU = smoothstep(0.0f, 1.0f, u);

        rangerPosX = lerpf(0.0f, 0.08f, smoothU);
        rangerPosY = lerpf(-0.92f, -1.35f, smoothU);
        rangerPosZ = lerpf(2.58f, 2.30f, smoothU);

        rangerDirX = 0.05f;
        rangerDirY = lerpf(-0.42f, -0.65f, smoothU);
        rangerDirZ = lerpf(0.90f, 0.75f, smoothU);
        normalize3(rangerDirX, rangerDirY, rangerDirZ);

        rangerPitchExtra = -26.0f;
        rangerRollAngle  = 8.0f + 4.0f * sinf(u * 3.14159f);

        rcsActive = 0.0f;
        mainBurnActive = (t >= 41.5f && t <= 46.5f) ? 1.0f : 0.0f;
        heatshieldThermal = smoothstep(45.0f, 47.0f, t) * 0.25f;

        // Spawn engine exhaust trail particles
        if (mainBurnActive > 0.0f) {
            for (int k = 0; k < 3; ++k) {
                float side = (k % 2 == 0) ? -0.08f : 0.08f;
                float exX = rangerPosX + side;
                float exY = rangerPosY + 0.01f;
                float exZ = rangerPosZ - 0.28f;
                spawnParticle(exX + randRange(-0.01f, 0.01f),
                              exY + randRange(-0.01f, 0.01f),
                              exZ - randRange(0.0f, 0.05f),
                              randRange(-0.05f, 0.05f), -0.2f, -0.8f,
                              0.5f, randRange(2.5f, 4.5f),
                              0.4f, 0.75f, 1.0f, true);
            }
        }
    }
    else if (t < 53.0f) {
        // PHASE 3: COCKPIT HUD CUT (47.0s - 53.0s)
        float u = (t - 47.0f) / 6.0f;

        rangerPosX = 0.08f + u * 0.05f;
        rangerPosY = lerpf(-1.35f, -1.82f, u);
        rangerPosZ = lerpf(2.30f, 2.05f, u);

        rangerDirX = 0.06f;
        rangerDirY = -0.72f;
        rangerDirZ = 0.69f;
        normalize3(rangerDirX, rangerDirY, rangerDirZ);

        rangerPitchExtra = -28.0f;
        rangerRollAngle  = 12.0f + 2.0f * sinf(t * 2.0f);

        rcsActive = 0.0f;
        mainBurnActive = 0.0f;
        heatshieldThermal = lerpf(0.25f, 0.75f, u);
    }
    else {
        // PHASE 4: HYPERSONIC ENTRY & PLASMA FIRE (53.0s - 60.0s)
        float u = (t - 53.0f) / 7.0f;

        rangerPosX = 0.13f + u * 0.06f;
        rangerPosY = lerpf(-1.82f, -2.15f, u);
        rangerPosZ = lerpf(2.05f, 1.88f, u);

        rangerDirX = 0.08f;
        rangerDirY = -0.78f;
        rangerDirZ = 0.62f;
        normalize3(rangerDirX, rangerDirY, rangerDirZ);

        rangerPitchExtra = -30.0f + 1.5f * sinf(t * 18.0f); // entry buffeting
        rangerRollAngle  = 14.0f + 1.2f * cosf(t * 15.0f);

        rcsActive = 0.0f;
        mainBurnActive = 0.0f;
        heatshieldThermal = lerpf(0.75f, 1.0f, smoothstep(53.0f, 57.0f, t));

        // Intense glowing plasma flame embers streaming into wake
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

    if (t < 41.0f) {
        // Phase 1 (35.0s - 41.0s): Docking Hub Separation View
        // Camera looks from slightly above Endurance hub tracking Ranger as it drops away
        float u = (t - 35.0f) / 6.0f;
        eyeX = lerpf(0.0f, -0.25f, u);
        eyeY = lerpf(-0.52f, -0.68f, u);
        eyeZ = lerpf(2.88f, 3.05f, u);

        targetX = rangerPosX * 0.6f;
        targetY = rangerPosY;
        targetZ = rangerPosZ;
    }
    else if (t < 47.0f) {
        // Phase 2 (41.0s - 47.0s): Dynamic Retro Burn 3/4 Orbit Chase
        // Camera moves behind & to the side of the firing engines
        float u = (t - 41.0f) / 6.0f;
        eyeX = lerpf(0.38f, 0.48f, u);
        eyeY = lerpf(-0.80f, -1.15f, u);
        eyeZ = lerpf(2.95f, 2.75f, u);

        targetX = rangerPosX;
        targetY = rangerPosY;
        targetZ = rangerPosZ;
    }
    else if (t < 53.0f) {
        // Phase 3 (47.0s - 53.0s): Cockpit First-Person Perspective
        // Eye placed directly inside Ranger cabin looking forward-down through canopy
        eyeX = rangerPosX;
        eyeY = rangerPosY + 0.04f;
        eyeZ = rangerPosZ + 0.06f;

        targetX = eyeX + rangerDirX * 2.0f;
        targetY = eyeY + rangerDirY * 2.0f;
        targetZ = eyeZ + rangerDirZ * 2.0f;

        // Subtle cockpit head buffeting
        eyeX += sinf(t * 22.0f) * 0.002f;
        eyeY += cosf(t * 19.0f) * 0.002f;
    }
    else {
        // Phase 4 (53.0s - 60.0s): External Hypersonic Plasma Chase with IMAX shake
        float u = (t - 53.0f) / 7.0f;

        // Buffeting camera vibration
        float shake = smoothstep(53.0f, 56.0f, t) * 0.012f;
        float sx = sinf(t * 35.0f) * shake;
        float sy = cosf(t * 42.0f) * shake;

        eyeX = lerpf(-0.45f, -0.55f, u) + sx;
        eyeY = lerpf(-1.75f, -2.05f, u) + sy;
        eyeZ = lerpf(2.45f, 2.25f, u);

        targetX = rangerPosX + sx * 0.5f;
        targetY = rangerPosY + sy * 0.5f;
        targetZ = rangerPosZ;
    }
}

// ============================================================================
// PHASE 3: COCKPIT HUD & CRT AVIONICS TELEMETRY OVERLAY
// ============================================================================
static void drawHUDString(float x, float y, const char* str, float scale = 0.12f) {
    glPushMatrix();
    glTranslatef(x, y, 0.0f);
    glScalef(scale, scale, 1.0f);
    for (const char* p = str; *p; ++p) {
        glutStrokeCharacter(GLUT_STROKE_ROMAN, *p);
    }
    glPopMatrix();
}

void renderCockpitHUD() {
    if (seqTime < 47.0f || seqTime > 53.0f) return;

    float t = seqTime;
    float phaseU = (t - 47.0f) / 6.0f;
    float hudAlpha = 1.0f;
    if (t < 47.6f) hudAlpha = smoothstep(47.0f, 47.6f, t);
    if (t > 52.4f) hudAlpha = 1.0f - smoothstep(52.4f, 53.0f, t);

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0.0, (GLdouble)windowWidth, 0.0, (GLdouble)windowHeight);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    float w = (float)windowWidth;
    float h = (float)windowHeight;
    float cx = w * 0.5f;
    float cy = h * 0.5f;

    // 1. Dark Cockpit Canopy Struts & Framing (Interstellar Ranger Cockpit Silhouette)
    glColor4f(0.06f, 0.07f, 0.09f, 0.94f * hudAlpha);
    // Lower instrument dash
    glBegin(GL_QUADS);
        glVertex2f(0.0f, 0.0f);
        glVertex2f(w, 0.0f);
        glVertex2f(w, h * 0.22f);
        glVertex2f(0.0f, h * 0.22f);
    glEnd();

    // Top ceiling frame
    glBegin(GL_QUADS);
        glVertex2f(0.0f, h * 0.88f);
        glVertex2f(w, h * 0.88f);
        glVertex2f(w, h);
        glVertex2f(0.0f, h);
    glEnd();

    // Center vertical canopy mullion / divider
    glBegin(GL_QUADS);
        glVertex2f(cx - 18.0f, 0.0f);
        glVertex2f(cx + 18.0f, 0.0f);
        glVertex2f(cx + 12.0f, h);
        glVertex2f(cx - 12.0f, h);
    glEnd();

    // Diagonal A-pillar window frames
    glBegin(GL_QUADS);
        glVertex2f(0.0f, 0.0f);
        glVertex2f(w * 0.16f, h * 0.22f);
        glVertex2f(w * 0.10f, h);
        glVertex2f(0.0f, h);

        glVertex2f(w, 0.0f);
        glVertex2f(w * 0.84f, h * 0.22f);
        glVertex2f(w * 0.90f, h);
        glVertex2f(w, h);
    glEnd();

    // 2. Phosphor Green & Amber CRT Vector HUD
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive luminous glow
    glLineWidth(1.6f);

    float phosphorR = 0.22f;
    float phosphorG = 0.98f;
    float phosphorB = 0.45f;
    glColor4f(phosphorR, phosphorG, phosphorB, 0.85f * hudAlpha);

    // Artificial Horizon / Flight Ladder in Center-Left
    float roll = 12.0f * sinf(t * 1.5f);
    glPushMatrix();
    glTranslatef(cx - w * 0.18f, cy, 0.0f);
    glRotatef(roll, 0.0f, 0.0f, 1.0f);
    glBegin(GL_LINES);
        glVertex2f(-60.0f, 0.0f); glVertex2f(-15.0f, 0.0f);
        glVertex2f(15.0f, 0.0f);  glVertex2f(60.0f, 0.0f);
        glVertex2f(-35.0f, 25.0f); glVertex2f(-10.0f, 25.0f);
        glVertex2f(10.0f, 25.0f);  glVertex2f(35.0f, 25.0f);
        glVertex2f(-35.0f, -25.0f); glVertex2f(-10.0f, -25.0f);
        glVertex2f(10.0f, -25.0f);  glVertex2f(35.0f, -25.0f);
    glEnd();
    glPopMatrix();

    // Flight Reticle
    glBegin(GL_LINE_LOOP);
    for (int deg = 0; deg < 360; deg += 15) {
        float rad = deg * 3.14159f / 180.0f;
        glVertex2f(cx + cosf(rad) * 22.0f, cy + sinf(rad) * 22.0f);
    }
    glEnd();

    // 3. Digital Telemetry Text Readouts
    char buf[128];
    float alt = lerpf(128.4f, 62.1f, phaseU);
    float vel = lerpf(7.85f, 6.42f, phaseU);
    float mach = vel * 3.12f;
    float qBar = lerpf(0.12f, 38.6f, smoothstep(0.0f, 1.0f, phaseU));
    float heatTemp = lerpf(420.0f, 1850.0f, smoothstep(0.0f, 1.0f, phaseU));

    float fontScale = (w / 1280.0f) * 0.11f;

    // Left Console Telemetry
    sprintf(buf, "ALT : %5.1f KM", alt);
    drawHUDString(w * 0.18f, h * 0.16f, buf, fontScale);

    sprintf(buf, "VEL : %4.2f KM/S [MACH %4.1f]", vel, mach);
    drawHUDString(w * 0.18f, h * 0.12f, buf, fontScale);

    sprintf(buf, "PITCH: -28.0 DEG  ROLL: +12.4 DEG");
    drawHUDString(w * 0.18f, h * 0.08f, buf, fontScale * 0.9f);

    // Right Console Telemetry (Thermal & Dynamic Pressure)
    if (heatTemp > 1200.0f) {
        // Amber warning for peak entry heat
        glColor4f(1.0f, 0.65f, 0.15f, 0.90f * hudAlpha);
    }
    sprintf(buf, "Q-BAR    : %4.1f KPA [MAX-Q]", qBar);
    drawHUDString(w * 0.58f, h * 0.16f, buf, fontScale);

    sprintf(buf, "HEATSHLD : %4.0f C [ENTRY INTERFACE]", heatTemp);
    drawHUDString(w * 0.58f, h * 0.12f, buf, fontScale);

    sprintf(buf, "TRAJECTORY: MARS CORRIDOR LOCKED");
    drawHUDString(w * 0.58f, h * 0.08f, buf, fontScale * 0.9f);

    // Thermal Bar Graph
    float barX = w * 0.58f;
    float barY = h * 0.05f;
    float barW = 220.0f * (w / 1280.0f);
    float barH = 8.0f;
    float fillW = barW * clampf(heatTemp / 2000.0f, 0.0f, 1.0f);

    glBegin(GL_LINE_LOOP);
        glVertex2f(barX, barY);
        glVertex2f(barX + barW, barY);
        glVertex2f(barX + barW, barY + barH);
        glVertex2f(barX, barY + barH);
    glEnd();
    glBegin(GL_QUADS);
        glVertex2f(barX, barY);
        glVertex2f(barX + fillW, barY);
        glVertex2f(barX + fillW, barY + barH);
        glVertex2f(barX, barY + barH);
    glEnd();

    // 4. Subtle CRT Scanlines
    glLineWidth(1.0f);
    glColor4f(0.15f, 0.85f, 0.40f, 0.06f * hudAlpha);
    glBegin(GL_LINES);
    for (float y = 0.0f; y < h; y += 4.0f) {
        glVertex2f(0.0f, y);
        glVertex2f(w, y);
    }
    glEnd();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}
