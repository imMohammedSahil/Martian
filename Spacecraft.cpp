#include "Spacecraft.h"
#include "Common.h"
#include "stb_image.h"
#include <cstdio>
#include <cstdint>

std::vector<Particle>   particles;
std::vector<DustStreak> dustStreaks;

float enduranceOrbitT  = 0.0f;
float ringSpinAngle    = 0.0f;
float endPosX = 0.0f, endPosY = 0.0f, endPosZ = 0.0f;
float endTanX = 0.0f, endTanY = 0.0f, endTanZ = 1.0f;

struct EnduVert {
    float x, y, z;
    float nx, ny, nz;
    float u, v;
    float r, g, b;
};

static GLuint enduranceList = 0;
static GLuint enduranceTexID = 0;
static bool enduranceLoaded = false;
static bool enduranceLoadFailed = false;

static GLuint rangerList = 0;
static GLuint rangerTexID = 0;
static bool rangerLoaded = false;
static bool rangerLoadFailed = false;

static void loadRangerModel() {
    if (rangerLoaded || rangerLoadFailed) return;

    // 1. Texture
    const char* texPaths[] = {
        "ranger_diffuse.png",
        "../ranger_diffuse.png",
        "bin/Release/ranger_diffuse.png",
        "bin/Debug/ranger_diffuse.png",
        "C:/Holidays/Martian/ranger_diffuse.png",
        "C:/Holidays/Martian/Ranger/textures/DefaultMaterial_Base_Color.png"
    };
    unsigned char* texData = NULL;
    int tw = 0, th = 0, tch = 0;
    for (int i = 0; i < 6; ++i) {
        texData = stbi_load(texPaths[i], &tw, &th, &tch, 3);
        if (texData) break;
    }

    if (texData) {
        glGenTextures(1, &rangerTexID);
        glBindTexture(GL_TEXTURE_2D, rangerTexID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, tw, th, GL_RGB, GL_UNSIGNED_BYTE, texData);
        stbi_image_free(texData);
    }

    // 2. Binary geometry
    const char* binPaths[] = {
        "ranger.bin",
        "../ranger.bin",
        "bin/Release/ranger.bin",
        "bin/Debug/ranger.bin",
        "C:/Holidays/Martian/ranger.bin"
    };
    FILE* fp = NULL;
    for (int i = 0; i < 5; ++i) {
        fp = fopen(binPaths[i], "rb");
        if (fp) break;
    }

    if (!fp) {
        rangerLoadFailed = true;
        return;
    }

    char magic[4];
    uint32_t numVerts = 0;
    if (fread(magic, 1, 4, fp) != 4 || memcmp(magic, "RNGR", 4) != 0 ||
        fread(&numVerts, sizeof(uint32_t), 1, fp) != 1 || numVerts == 0) {
        fclose(fp);
        rangerLoadFailed = true;
        return;
    }

    std::vector<EnduVert> verts(numVerts);
    size_t readCount = fread(&verts[0], sizeof(EnduVert), numVerts, fp);
    fclose(fp);

    if (readCount != numVerts) {
        rangerLoadFailed = true;
        return;
    }

    rangerList = glGenLists(1);
    glNewList(rangerList, GL_COMPILE);

    if (rangerTexID) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, rangerTexID);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    GLfloat amb[]  = { 0.40f, 0.40f, 0.45f, 1.0f };
    GLfloat spec[] = { 0.82f, 0.82f, 0.88f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, amb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 64.0f);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    glVertexPointer(3, GL_FLOAT, sizeof(EnduVert), &verts[0].x);
    glNormalPointer(GL_FLOAT, sizeof(EnduVert), &verts[0].nx);
    glTexCoordPointer(2, GL_FLOAT, sizeof(EnduVert), &verts[0].u);
    glColorPointer(3, GL_FLOAT, sizeof(EnduVert), &verts[0].r);

    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)numVerts);

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    glDisable(GL_COLOR_MATERIAL);
    if (rangerTexID) {
        glDisable(GL_TEXTURE_2D);
    }

    glEndList();
    rangerLoaded = true;
}

static void loadEnduranceModel() {
    if (enduranceLoaded || enduranceLoadFailed) return;

    // 1. Load authentic master texture
    const char* texPaths[] = {
        "endurance_diffuse.jpg",
        "../endurance_diffuse.jpg",
        "bin/Release/endurance_diffuse.jpg",
        "bin/Debug/endurance_diffuse.jpg",
        "C:/Holidays/Martian/endurance_diffuse.jpg",
        "C:/Holidays/Martian/Endurance from SketchLab/textures/A-MasterColour.jpeg"
    };
    unsigned char* texData = NULL;
    int tw = 0, th = 0, tch = 0;
    for (int i = 0; i < 6; ++i) {
        texData = stbi_load(texPaths[i], &tw, &th, &tch, 3);
        if (texData) break;
    }

    if (texData) {
        glGenTextures(1, &enduranceTexID);
        glBindTexture(GL_TEXTURE_2D, enduranceTexID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, tw, th, GL_RGB, GL_UNSIGNED_BYTE, texData);
        stbi_image_free(texData);
    }

    // 2. Load binary geometry
    const char* binPaths[] = {
        "endurance.bin",
        "../endurance.bin",
        "bin/Release/endurance.bin",
        "bin/Debug/endurance.bin",
        "C:/Holidays/Martian/endurance.bin"
    };
    FILE* fp = NULL;
    for (int i = 0; i < 5; ++i) {
        fp = fopen(binPaths[i], "rb");
        if (fp) break;
    }

    if (!fp) {
        enduranceLoadFailed = true;
        return;
    }

    char magic[4];
    uint32_t numVerts = 0;
    if (fread(magic, 1, 4, fp) != 4 || memcmp(magic, "ENDU", 4) != 0 ||
        fread(&numVerts, sizeof(uint32_t), 1, fp) != 1 || numVerts == 0) {
        fclose(fp);
        enduranceLoadFailed = true;
        return;
    }

    std::vector<EnduVert> verts(numVerts);
    size_t readCount = fread(&verts[0], sizeof(EnduVert), numVerts, fp);
    fclose(fp);

    if (readCount != numVerts) {
        enduranceLoadFailed = true;
        return;
    }

    // Compile into GPU display list
    enduranceList = glGenLists(1);
    glNewList(enduranceList, GL_COMPILE);

    if (enduranceTexID) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, enduranceTexID);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    GLfloat spec[] = { 0.75f, 0.75f, 0.82f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 54.0f);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    glVertexPointer(3, GL_FLOAT, sizeof(EnduVert), &verts[0].x);
    glNormalPointer(GL_FLOAT, sizeof(EnduVert), &verts[0].nx);
    glTexCoordPointer(2, GL_FLOAT, sizeof(EnduVert), &verts[0].u);
    glColorPointer(3, GL_FLOAT, sizeof(EnduVert), &verts[0].r);

    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)numVerts);

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    glDisable(GL_COLOR_MATERIAL);
    if (enduranceTexID) {
        glDisable(GL_TEXTURE_2D);
    }

    glEndList();
    enduranceLoaded = true;
}

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

    // Small pinpoint dot beacon during Shot 2 & early Shot 3 (0.0s - 18.0s)
    if (seqTime < 18.0f) {
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        float dotAlpha = (seqTime < 13.0f) ? 1.0f : (1.0f - smoothstep(13.0f, 18.0f, seqTime));
        float strobe = 0.85f + 0.15f * sinf(totalTime * 8.0f);
        glPointSize(2.4f);
        glBegin(GL_POINTS);
            glColor4f(1.0f, 0.98f, 0.94f, strobe * dotAlpha);
            glVertex3f(0.0f, 0.0f, 0.0f);
        glEnd();

        glPointSize(3.8f);
        glBegin(GL_POINTS);
            float redBlink = (fmodf(totalTime * 1.5f, 1.0f) < 0.25f) ? 0.95f : 0.25f;
            glColor4f(1.0f, 0.32f, 0.20f, redBlink * dotAlpha);
            glVertex3f(0.0f, 0.0f, 0.0f);
        glEnd();

        glPointSize(1.0f);
        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
    }

    // Microscopic scale in Shot 2, smoothly enlarging only during the camera's physical descent in Shot 3
    float shipScale = 0.135f;
    if (seqTime < 13.0f) {
        shipScale = 0.008f; // Microscopic dot in Shot 2
    } else if (seqTime < 22.0f) {
        float t = (seqTime - 13.0f) / 9.0f;
        float e = smoothstep(0.0f, 1.0f, t);
        shipScale = lerpf(0.008f, 0.135f, e); // Matches camera descent into low orbit
    }
    glScalef(shipScale, shipScale, shipScale);

    // Spinning Ring Frame
    glPushMatrix();
    glRotatef(ringSpinAngle, 0.0f, 0.0f, 1.0f);

    if (!enduranceLoaded && !enduranceLoadFailed) {
        loadEnduranceModel();
    }

    if (enduranceLoaded && enduranceList) {
        glCallList(enduranceList);

        // Subtle navigation strobe beacons along the perimeter
        for (int i = 0; i < 12; ++i) {
            if (i % 3 == 0) {
                float blink = (fmodf(totalTime * 2.2f + i * 0.5f, 1.0f) < 0.22f) ? 1.0f : 0.0f;
                if (blink > 0.0f) {
                    float ang = i * (3.14159265f * 2.0f / 12.0f);
                    GLfloat beaconColor[] = { 1.0f, 0.25f, 0.20f, 1.0f };
                    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, beaconColor);
                    glPushMatrix();
                    glTranslatef(cosf(ang) * 0.98f, sinf(ang) * 0.98f, 0.06f);
                    glutSolidSphere(0.018f, 8, 6);
                    glPopMatrix();
                    GLfloat zeroEmit[] = { 0.0f, 0.0f, 0.0f, 1.0f };
                    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zeroEmit);
                }
            }
        }
    } else {
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
    }

    glPopMatrix(); // End spinning ring

    // Shot 1 (35.0s - 39.0s): Small craft naturally emerges out from inside the central hub pipe
    if (seqTime >= 35.0f && seqTime < 39.0f) {
        float u = (seqTime - 35.0f) / 4.0f;
        float smoothU = smoothstep(0.0f, 1.0f, u);
        float emergeAlpha = smoothstep(0.0f, 0.12f, u); // Smooth progressive emergence, zero sudden pop-in

        loadRangerModel();

        glPushMatrix();
        // Starts nestled inside the central pipe bore and smoothly slides out backwards (-Z)
        float detachZ = lerpf(-0.06f, -0.92f, smoothU);
        float dropY   = lerpf(-0.01f, -0.16f, smoothU);
        glTranslatef(0.0f, dropY, detachZ);
        glRotatef(lerpf(0.0f, -14.0f, smoothU), 1.0f, 0.0f, 0.0f);

        // Small sleek proportional scale fitting cleanly out of the central pipe bore
        float hubScale = 0.055f;
        glScalef(hubScale, hubScale, hubScale);

        if (emergeAlpha > 0.01f) {
            if (rangerLoaded && rangerList) {
                glCallList(rangerList);
            } else {
                GLfloat podWhite[] = { 0.92f, 0.94f, 0.96f, 1.0f };
                GLfloat podDark[]  = { 0.16f, 0.18f, 0.22f, 1.0f };
                glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, podWhite);
                drawBox(0.36f, 0.14f, 0.90f);
                glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, podDark);
                glPushMatrix();
                glTranslatef(0.0f, -0.05f, 0.0f);
                drawBox(0.38f, 0.06f, 0.92f);
                glPopMatrix();
            }

            // Subtle RCS separation puff as it clears the pipe
            if (u > 0.15f && u < 0.85f) {
                glDisable(GL_LIGHTING);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);

                glColor4f(0.80f, 0.92f, 1.0f, 0.70f);
                glLineWidth(1.8f);
                glBegin(GL_LINES);
                    glVertex3f(-0.50f, 0.0f, 0.1f); glVertex3f(-0.95f, 0.0f, 0.05f);
                    glVertex3f( 0.50f, 0.0f, 0.1f); glVertex3f( 0.95f, 0.0f, 0.05f);
                glEnd();

                // Faint blue engine pilot spark
                glColor4f(0.35f, 0.75f, 1.0f, 0.80f);
                for (float side = -0.32f; side <= 0.35f; side += 0.64f) {
                    glPushMatrix();
                    glTranslatef(side, 0.0f, -0.95f);
                    glutSolidSphere(0.06f, 8, 6);
                    glPopMatrix();
                }

                glLineWidth(1.0f);
                glDisable(GL_BLEND);
                glEnable(GL_LIGHTING);
            }
        }

        glPopMatrix();
    }

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
    loadRangerModel();

    glPushMatrix();
    glTranslatef(rangerPosX, rangerPosY, rangerPosZ);

    // Dedicated key light for the Ranger so its facets, heatshield and hull pop out vividly
    glEnable(GL_LIGHT2);
    GLfloat rangerKeyPos[4]     = { -1.5f, 3.0f, 2.5f, 0.0f }; // Directional light angled from upper-front
    GLfloat rangerKeyDiffuse[4] = { 1.6f, 1.55f, 1.5f, 1.0f };
    GLfloat rangerKeyAmbient[4] = { 0.45f, 0.45f, 0.50f, 1.0f };
    GLfloat rangerKeySpec[4]    = { 0.8f, 0.8f, 0.8f, 1.0f };
    glLightfv(GL_LIGHT2, GL_POSITION, rangerKeyPos);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, rangerKeyDiffuse);
    glLightfv(GL_LIGHT2, GL_AMBIENT, rangerKeyAmbient);
    glLightfv(GL_LIGHT2, GL_SPECULAR, rangerKeySpec);

    // Orientation: look along flight direction vector
    float yaw = atan2f(rangerDirX, rangerDirZ) * 180.0f / (float)M_PI;
    glRotatef(yaw, 0.0f, 1.0f, 0.0f);

    float horizLen = sqrtf(rangerDirX * rangerDirX + rangerDirZ * rangerDirZ);
    float pitch    = -atan2f(rangerDirY, horizLen) * 180.0f / (float)M_PI;
    glRotatef(pitch + rangerPitchExtra, 1.0f, 0.0f, 0.0f);
    glRotatef(rangerRollAngle, 0.0f, 0.0f, 1.0f);

    // Dynamic scale receding into distance: prominent hero scale in deep space -> pinpoint speck over Mars
    float rScale = 0.22f;
    if (seqTime >= 39.0f && seqTime < 46.0f) {
        float u = (seqTime - 39.0f) / 7.0f;
        rScale = lerpf(0.22f, 0.095f, smoothstep(0.0f, 1.0f, u));
    } else if (seqTime >= 46.0f) {
        float u = (seqTime - 46.0f) / 10.0f;
        rScale = lerpf(0.095f, 0.016f, u);
    }
    glScalef(rScale, rScale, rScale);

    if (rangerLoaded && rangerList) {
        glCallList(rangerList);
    } else {
        GLfloat zeroEmit[]     = { 0.0f, 0.0f, 0.0f, 1.0f };
        GLfloat rangerWhite[]  = { 0.88f, 0.90f, 0.94f, 1.0f };
        GLfloat rangerDark[]   = { 0.14f, 0.15f, 0.18f, 1.0f };
        GLfloat rangerGlass[]  = { 0.08f, 0.12f, 0.18f, 1.0f };

        // Ranger Faceted Lifting-Body Fuselage
        glMaterialfv(GL_FRONT, GL_DIFFUSE, rangerWhite);
        glBegin(GL_TRIANGLES);
            glNormal3f(0.0f, 0.707f, 0.707f);
            glVertex3f(0.0f,   0.14f,  0.88f);
            glVertex3f(-0.35f, 0.02f, -0.22f);
            glVertex3f(0.35f,  0.02f, -0.22f);

            glNormal3f(0.0f, 1.0f, 0.0f);
            glVertex3f(-0.35f, 0.02f, -0.22f);
            glVertex3f(-0.52f, 0.00f, -0.85f);
            glVertex3f(0.52f,  0.00f, -0.85f);

            glVertex3f(-0.35f, 0.02f, -0.22f);
            glVertex3f(0.52f,  0.00f, -0.85f);
            glVertex3f(0.35f,  0.02f, -0.22f);
        glEnd();

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

        glMaterialfv(GL_FRONT, GL_DIFFUSE, rangerGlass);
        glPushMatrix();
        glTranslatef(0.0f, 0.08f, 0.32f);
        glScalef(0.20f, 0.08f, 0.38f);
        glutSolidCube(1.0);
        glPopMatrix();
    }

    // 1. Cold-Gas RCS Thruster Puffs
    if (rcsActive > 0.0f) {
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glLineWidth(2.5f);
        glColor4f(0.85f, 0.95f, 1.0f, 0.75f * rcsActive);
        glBegin(GL_LINES);
            glVertex3f(-0.55f, 0.0f, 0.2f); glVertex3f(-1.1f, 0.0f, 0.1f);
            glVertex3f( 0.55f, 0.0f, 0.2f); glVertex3f( 1.1f, 0.0f, 0.1f);
            glVertex3f(-0.45f, 0.1f, -0.4f); glVertex3f(-0.9f, 0.2f, -0.5f);
            glVertex3f( 0.45f, 0.1f, -0.4f); glVertex3f( 0.9f, 0.2f, -0.5f);
        glEnd();
        glLineWidth(1.0f);
        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
    }

    // 2. Twin Main Propulsion De-Orbit Flame Plumes (cinematic compact shock diamonds & bright core)
    if (mainBurnActive > 0.0f) {
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        float burnFlicker = 0.88f + 0.12f * sinf(totalTime * 45.0f);
        float flameLen = 0.45f * mainBurnActive * burnFlicker;

        for (float side = -0.32f; side <= 0.35f; side += 0.64f) {
            glPushMatrix();
            glTranslatef(side, 0.0f, -0.92f);

            // Core shock diamond
            glColor4f(0.95f, 0.98f, 1.0f, 0.95f);
            glBegin(GL_TRIANGLE_FAN);
                glVertex3f(0.0f, 0.0f, -flameLen * 0.55f);
                for (int deg = 0; deg <= 360; deg += 45) {
                    float rad = deg * 3.14159f / 180.0f;
                    glColor4f(0.40f, 0.85f, 1.0f, 0.0f);
                    glVertex3f(cosf(rad) * 0.07f, sinf(rad) * 0.07f, 0.0f);
                }
            glEnd();

            // Outer plasma plume
            glColor4f(0.25f, 0.65f, 1.0f, 0.70f);
            glBegin(GL_TRIANGLE_FAN);
                glVertex3f(0.0f, 0.0f, -flameLen);
                for (int deg = 0; deg <= 360; deg += 45) {
                    float rad = deg * 3.14159f / 180.0f;
                    glColor4f(0.08f, 0.35f, 0.85f, 0.0f);
                    glVertex3f(cosf(rad) * 0.14f, sinf(rad) * 0.14f, 0.0f);
                }
            glEnd();

            glPopMatrix();
        }

        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
    }

    // 3. Hypersonic Plasma Bow Shock & Belly Incandescence
    if (heatshieldThermal > 0.05f) {
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        float shockFlicker = 0.90f + 0.10f * sinf(totalTime * 32.0f);
        float shockAlpha = heatshieldThermal * 0.75f * shockFlicker;

        glColor4f(1.0f, 0.55f, 0.12f, shockAlpha);
        glPushMatrix();
        glTranslatef(0.0f, -0.05f, 0.85f);
        glScalef(1.25f, 0.75f, 1.0f);
        glBegin(GL_TRIANGLE_FAN);
            glColor4f(1.0f, 0.85f, 0.45f, shockAlpha);
            glVertex3f(0.0f, 0.0f, 0.35f);
            for (int deg = 0; deg <= 360; deg += 30) {
                float rad = deg * 3.14159f / 180.0f;
                glColor4f(0.95f, 0.25f, 0.05f, 0.0f);
                glVertex3f(cosf(rad) * 0.85f, sinf(rad) * 0.45f, -0.75f);
            }
        glEnd();
        glPopMatrix();

        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
    }

    // 4. Distance Beacon Dot: When receding into the distance (Shot 3), ensure crisp visible speck
    if (seqTime >= 46.0f) {
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        float strobe = 0.85f + 0.15f * sinf(totalTime * 12.0f);
        glPointSize(3.5f);
        glBegin(GL_POINTS);
            glColor4f(1.0f, 0.98f, 0.92f, strobe);
            glVertex3f(0.0f, 0.0f, 0.0f);
        glEnd();

        glPointSize(5.0f);
        glBegin(GL_POINTS);
            float redBlink = (fmodf(totalTime * 2.0f, 1.0f) < 0.25f) ? 0.95f : 0.20f;
            glColor4f(1.0f, 0.35f, 0.20f, redBlink);
            glVertex3f(0.0f, 0.0f, 0.0f);
        glEnd();

        glPointSize(1.0f);
        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
    }

    glDisable(GL_LIGHT2);
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

