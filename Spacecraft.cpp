#include "Spacecraft.h"
#include "Common.h"

std::vector<Particle>   particles;
std::vector<DustStreak> dustStreaks;

float enduranceOrbitT  = 0.0f;
float ringSpinAngle    = 0.0f;
float endPosX = 0.0f, endPosY = 0.0f, endPosZ = 0.0f;
float endTanX = 0.0f, endTanY = 0.0f, endTanZ = 1.0f;

float rangerPosX = 0.0f, rangerPosY = 0.0f, rangerPosZ = 0.0f;
float rangerDirX = 0.0f, rangerDirY = 0.0f, rangerDirZ = 1.0f;
float rangerPitchExtra  = 0.0f;
float rangerRollAngle   = 0.0f;
float heatshieldThermal = 0.0f;
float rcsActive         = 0.0f;
float mainBurnActive    = 0.0f;
bool  rangerDetached    = false;

void spawnParticle(float x, float y, float z,
                   float vx, float vy, float vz,
                   float life, float size,
                   float r, float g, float b, bool additive) {
    Particle p;
    p.x = x; p.y = y; p.z = z;
    p.vx = vx; p.vy = vy; p.vz = vz;
    p.life = life;
    p.maxLife = life;
    p.size = size;
    p.r = r; p.g = g; p.b = b;
    p.additive = additive;
    particles.push_back(p);
}

void renderEnduranceShip() {
    glPushMatrix();
    glTranslatef(endPosX, endPosY, endPosZ);

    // Align Endurance ring plane slightly tilted toward Mars horizon
    float yaw = atan2f(endTanX, endTanZ) * 180.0f / (float)M_PI;
    glRotatef(yaw, 0.0f, 1.0f, 0.0f);
    glRotatef(26.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(14.0f, 0.0f, 0.0f, 1.0f);

    // Small pinpoint dot beacon during Shot 2 & early Shot 3 (0.0s - 22.0s)
    if (seqTime < 22.0f) {
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        float dotAlpha = (seqTime < 16.0f) ? 1.0f : (1.0f - smoothstep(16.0f, 22.0f, seqTime));
        float strobe = 0.85f + 0.15f * sinf(totalTime * 8.0f);
        glPointSize(3.5f);
        glBegin(GL_POINTS);
            glColor4f(1.0f, 0.98f, 0.94f, strobe * dotAlpha);
            glVertex3f(0.0f, 0.0f, 0.0f);
        glEnd();

        glPointSize(6.0f);
        glBegin(GL_POINTS);
            float redBlink = (fmodf(totalTime * 1.5f, 1.0f) < 0.25f) ? 0.95f : 0.25f;
            glColor4f(1.0f, 0.32f, 0.20f, redBlink * dotAlpha);
            glVertex3f(0.0f, 0.0f, 0.0f);
        glEnd();

        glPointSize(1.0f);
        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
    }

    // Dynamic gradual scale: tiny microscopic dot in Shot 2, smoothly expanding during the ultra-slow zoom
    float shipScale = 0.38f;
    if (seqTime < 15.0f) {
        shipScale = 0.02f; // Tiny dot scale
    } else if (seqTime < 28.0f) {
        float t = (seqTime - 15.0f) / 13.0f;
        float e = smoothstep(0.0f, 1.0f, t);
        shipScale = lerpf(0.02f, 0.38f, e); // Ultra-slow continuous enlargement
    }
    glScalef(shipScale, shipScale, shipScale);

    // Spinning Ring Frame
    glPushMatrix();
    glRotatef(ringSpinAngle, 0.0f, 0.0f, 1.0f);

    GLfloat hullWhite[] = { 0.86f, 0.88f, 0.92f, 1.0f };
    GLfloat hullDark[]  = { 0.20f, 0.22f, 0.26f, 1.0f };
    GLfloat hullSpec[]  = { 0.75f, 0.75f, 0.80f, 1.0f };
    GLfloat hullShine[] = { 48.0f };
    GLfloat zeroEmit[]  = { 0.0f, 0.0f, 0.0f, 1.0f };

    glMaterialfv(GL_FRONT, GL_SPECULAR, hullSpec);
    glMaterialfv(GL_FRONT, GL_SHININESS, hullShine);
    glMaterialfv(GL_FRONT, GL_EMISSION, zeroEmit);

    const int NUM_MODULES = 12;
    const float RING_R    = 1.15f;

    // 12 Endurance Ring Modules + Connecting Ring Trusses
    for (int i = 0; i < NUM_MODULES; ++i) {
        float angDeg = i * (360.0f / NUM_MODULES);
        glPushMatrix();
        glRotatef(angDeg, 0.0f, 0.0f, 1.0f);
        glTranslatef(RING_R, 0.0f, 0.0f);

        // Main pressurized module body
        glMaterialfv(GL_FRONT, GL_DIFFUSE, hullWhite);
        drawBox(0.24f, 0.36f, 0.26f);

        // Dark thermal radiator / carbon trim plates
        glMaterialfv(GL_FRONT, GL_DIFFUSE, hullDark);
        glPushMatrix();
        glTranslatef(0.11f, 0.0f, 0.0f);
        drawBox(0.05f, 0.32f, 0.22f);
        glPopMatrix();

        // Connecting tubular perimeter truss to adjacent module
        glPushMatrix();
        glTranslatef(0.0f, 0.30f, 0.0f);
        glRotatef(15.0f, 0.0f, 0.0f, 1.0f);
        drawBox(0.06f, 0.28f, 0.06f);
        glPopMatrix();

        // Cabin Viewport Slits (warm amber/white glow)
        GLfloat windowGlow[] = { 0.95f, 0.88f, 0.65f, 1.0f };
        glMaterialfv(GL_FRONT, GL_EMISSION, windowGlow);
        glPushMatrix();
        glTranslatef(-0.115f, 0.0f, 0.04f);
        drawBox(0.02f, 0.16f, 0.04f);
        glPopMatrix();
        glMaterialfv(GL_FRONT, GL_EMISSION, zeroEmit);

        // Strobe Navigation Beacons (alternating red / green / white)
        if (i % 3 == 0) {
            float blink = (fmodf(totalTime * 2.2f + i * 0.5f, 1.0f) < 0.22f) ? 1.0f : 0.0f;
            if (blink > 0.0f) {
                GLfloat beaconColor[] = { 1.0f, 0.25f, 0.20f, 1.0f };
                glMaterialfv(GL_FRONT, GL_EMISSION, beaconColor);
                glPushMatrix();
                glTranslatef(0.13f, 0.16f, 0.12f);
                glutSolidSphere(0.022f, 8, 6);
                glPopMatrix();
                glMaterialfv(GL_FRONT, GL_EMISSION, zeroEmit);
            }
        }
        glPopMatrix();
    }

    // 4 Radial Spokes to Central Hub
    glMaterialfv(GL_FRONT, GL_DIFFUSE, hullDark);
    for (int s = 0; s < 4; ++s) {
        glPushMatrix();
        glRotatef(s * 90.0f, 0.0f, 0.0f, 1.0f);
        glTranslatef(RING_R * 0.5f, 0.0f, 0.0f);
        drawBox(RING_R * 0.92f, 0.07f, 0.09f);
        glPopMatrix();
    }

    // Central Docking Hub & Spine
    glMaterialfv(GL_FRONT, GL_DIFFUSE, hullWhite);
    glutSolidSphere(0.20f, 20, 16);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, hullDark);
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, -0.22f);
    glutSolidCone(0.14f, 0.35f, 14, 4);
    glPopMatrix();

    glPopMatrix(); // End spinning ring

    // Interstellar beacon: at scale distance, render crisp anti-aliased luminous point & strobe
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glPointSize(3.8f);
    glBegin(GL_POINTS);
        float strobe = 0.75f + 0.25f * sinf(totalTime * 8.0f);
        glColor4f(1.0f, 0.98f, 0.92f, strobe);
        glVertex3f(0.0f, 0.0f, 0.0f);
    glEnd();
    glPointSize(1.0f);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);

    glPopMatrix();
}

void renderRangerLander() {
    glPushMatrix();
    glTranslatef(rangerPosX, rangerPosY, rangerPosZ);

    // Orientation: look along flight direction vector
    float yaw = atan2f(rangerDirX, rangerDirZ) * 180.0f / (float)M_PI;
    glRotatef(yaw, 0.0f, 1.0f, 0.0f);

    float horizLen = sqrtf(rangerDirX * rangerDirX + rangerDirZ * rangerDirZ);
    float pitch    = -atan2f(rangerDirY, horizLen) * 180.0f / (float)M_PI;
    glRotatef(pitch + rangerPitchExtra, 1.0f, 0.0f, 0.0f);
    glRotatef(rangerRollAngle, 0.0f, 0.0f, 1.0f);

    glScalef(0.19f, 0.19f, 0.19f);

    GLfloat zeroEmit[]     = { 0.0f, 0.0f, 0.0f, 1.0f };
    GLfloat rangerWhite[]  = { 0.88f, 0.90f, 0.94f, 1.0f };
    GLfloat rangerDark[]   = { 0.14f, 0.15f, 0.18f, 1.0f };
    GLfloat rangerGlass[]  = { 0.08f, 0.12f, 0.18f, 1.0f };

    // Ranger Faceted Lifting-Body Fuselage
    glMaterialfv(GL_FRONT, GL_DIFFUSE, rangerWhite);
    glBegin(GL_TRIANGLES);
        // Top deck forward slope
        glNormal3f(0.0f, 0.707f, 0.707f);
        glVertex3f(0.0f,   0.14f,  0.88f);
        glVertex3f(-0.35f, 0.02f, -0.22f);
        glVertex3f(0.35f,  0.02f, -0.22f);

        // Aft upper deck
        glNormal3f(0.0f, 1.0f, 0.0f);
        glVertex3f(-0.35f, 0.02f, -0.22f);
        glVertex3f(-0.52f, 0.00f, -0.85f);
        glVertex3f(0.52f,  0.00f, -0.85f);

        glVertex3f(-0.35f, 0.02f, -0.22f);
        glVertex3f(0.52f,  0.00f, -0.85f);
        glVertex3f(0.35f,  0.02f, -0.22f);
    glEnd();

    // Carbon-phenolic thermal belly heatshield
    GLfloat heatEmit[4] = {
        heatshieldThermal * 1.6f,
        heatshieldThermal * 0.72f,
        heatshieldThermal * 0.16f,
        1.0f
    };
    glMaterialfv(GL_FRONT, GL_DIFFUSE, rangerDark);
    glMaterialfv(GL_FRONT, GL_EMISSION, heatEmit);
    glBegin(GL_QUADS);
        glNormal3f(0.0f, -1.0f, 0.0f);
        glVertex3f(0.0f,   -0.12f,  0.88f);
        glVertex3f(-0.54f, -0.06f, -0.85f);
        glVertex3f(0.54f,  -0.06f, -0.85f);
        glVertex3f(0.0f,   -0.12f,  0.88f);
    glEnd();
    glMaterialfv(GL_FRONT, GL_EMISSION, zeroEmit);

    // Dark Cockpit Canopy
    glMaterialfv(GL_FRONT, GL_DIFFUSE, rangerGlass);
    glPushMatrix();
    glTranslatef(0.0f, 0.08f, 0.32f);
    glScalef(0.20f, 0.08f, 0.38f);
    glutSolidCube(1.0);
    glPopMatrix();

    // Twin Main Propulsion Nozzles
    glMaterialfv(GL_FRONT, GL_DIFFUSE, rangerDark);
    for (float side = -1.0f; side <= 1.0f; side += 2.0f) {
        glPushMatrix();
        glTranslatef(side * 0.22f, -0.02f, -0.92f);
        glutSolidCone(0.07f, 0.18f, 12, 3);
        glPopMatrix();
    }

    glPopMatrix();
}

void renderParticles() {
    if (particles.empty()) return;
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);

    for (size_t i = 0; i < particles.size(); ++i) {
        const Particle& p = particles[i];
        float alpha = p.life / p.maxLife;

        if (p.additive) {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        } else {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        glPointSize(p.size);
        glBegin(GL_POINTS);
            glColor4f(p.r, p.g, p.b, alpha * 0.85f);
            glVertex3f(p.x, p.y, p.z);
        glEnd();
    }

    glPointSize(1.0f);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

void renderSpeedDustStreaks(float camX, float camY, float camZ) {
    if (dustStreaks.empty()) return;
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glLineWidth(1.4f);

    glBegin(GL_LINES);
    for (size_t i = 0; i < dustStreaks.size(); ++i) {
        float x0 = dustStreaks[i].x;
        float y0 = dustStreaks[i].y;
        float z0 = dustStreaks[i].z;
        float x1 = x0 + rangerDirX * dustStreaks[i].length;
        float y1 = y0 + rangerDirY * dustStreaks[i].length;
        float z1 = z0 + rangerDirZ * dustStreaks[i].length;

        glColor4f(1.0f, 0.72f, 0.40f, dustStreaks[i].alpha * 0.65f);
        glVertex3f(x0, y0, z0);
        glColor4f(0.85f, 0.92f, 1.0f, 0.0f);
        glVertex3f(x1, y1, z1);
    }
    glEnd();

    glLineWidth(1.0f);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

