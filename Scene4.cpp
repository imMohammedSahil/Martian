#include "Scene4.h"
#include "SceneDirector.h"
#include "Common.h"
#include "stb_image.h"
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <vector>
#include <algorithm>

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

// ============================================================================
// STRUCTURES & STATIC STATE
// ============================================================================
struct AstroVertex {
    float x, y, z;
    float nx, ny, nz;
    float u, v;
    float r, g, b;
};

static GLuint astroDisplayLists[4] = { 0, 0, 0, 0 };
static GLuint astroTexID = 0;
static bool   astroLoaded = false;
static bool   astroLoadFailed = false;

static GLuint sandTexID = 0;
static GLuint cabinTexID = 0;

static GLuint duneDisplayList = 0;
static GLuint cabinDisplayList = 0;
static GLuint skyDomeDisplayList = 0;

static bool   scene4Initialized = false;

// Floating Cabin Dust Motes
struct CabinMote {
    float x, y, z;
    float vx, vy, vz;
    float size;
    float alpha;
    float phase;
};
static std::vector<CabinMote> cabinMotes;

// ============================================================================
// INTERACTIVE LIGHTING KEYFRAME SYSTEM
// ============================================================================
struct LightKeyframe {
    float time;
    float sunBrightness;
    float sunHeight;
    float sunAngleX;
    float cabinAmbient;
    float lightShaft;
    float doorBloom;
    float skyWarmth;
};

static std::vector<LightKeyframe> g_keyframes;
static LightKeyframe g_curLight = { 61.0f, 2.40f, 14.0f, 1.50f, 0.15f, 0.28f, 0.45f, 1.00f };
static bool g_showLightingHUD = true;
static char g_hudStatusMsg[128] = "HUD Active: [Space] Pause, [< >] Scrub, Hotkeys to tweak";
static float g_hudMsgTimer = 4.0f;

void reloadScene4Lighting() {
    FILE* fp = fopen("scene4_lighting.txt", "r");
    if (!fp) return;
    g_keyframes.clear();
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        LightKeyframe kf;
        if (sscanf(line, "keyframe %f %f %f %f %f %f %f %f",
                   &kf.time, &kf.sunBrightness, &kf.sunHeight, &kf.sunAngleX,
                   &kf.cabinAmbient, &kf.lightShaft, &kf.doorBloom, &kf.skyWarmth) == 8) {
            g_keyframes.push_back(kf);
        } else {
            char key[64];
            float val = 0.0f;
            if (sscanf(line, "%63[^= ] = %f", key, &val) == 2 || sscanf(line, "%63s %f", key, &val) == 2) {
                if (strcmp(key, "sun_brightness") == 0) g_curLight.sunBrightness = val;
                else if (strcmp(key, "sun_height") == 0) g_curLight.sunHeight = val;
                else if (strcmp(key, "sun_angle_x") == 0) g_curLight.sunAngleX = val;
                else if (strcmp(key, "cabin_ambient") == 0) g_curLight.cabinAmbient = val;
                else if (strcmp(key, "light_shaft") == 0) g_curLight.lightShaft = val;
                else if (strcmp(key, "door_bloom") == 0) g_curLight.doorBloom = val;
                else if (strcmp(key, "sky_warmth") == 0) g_curLight.skyWarmth = val;
            }
        }
    }
    fclose(fp);

    std::sort(g_keyframes.begin(), g_keyframes.end(), [](const LightKeyframe& a, const LightKeyframe& b) {
        return a.time < b.time;
    });
}

static void saveLightingKeyframes() {
    FILE* fp = fopen("scene4_lighting.txt", "w");
    if (!fp) return;
    fprintf(fp, "# Scene 4 Interactive Lighting & Grading Keyframes\n");
    fprintf(fp, "# Format: keyframe <time_sec> <sun_brightness> <sun_height> <sun_angle_x> <cabin_ambient> <light_shaft> <door_bloom> <sky_warmth>\n");
    for (size_t i = 0; i < g_keyframes.size(); ++i) {
        const LightKeyframe& k = g_keyframes[i];
        fprintf(fp, "keyframe %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f\n",
                k.time, k.sunBrightness, k.sunHeight, k.sunAngleX, k.cabinAmbient, k.lightShaft, k.doorBloom, k.skyWarmth);
    }
    fclose(fp);
    snprintf(g_hudStatusMsg, sizeof(g_hudStatusMsg), "SAVED %d keyframes to scene4_lighting.txt!", (int)g_keyframes.size());
    g_hudMsgTimer = 3.5f;
}

static LightKeyframe evalLighting(float t) {
    if (g_keyframes.empty()) return g_curLight;
    if (t <= g_keyframes.front().time) return g_keyframes.front();
    if (t >= g_keyframes.back().time) return g_keyframes.back();
    for (size_t i = 0; i + 1 < g_keyframes.size(); ++i) {
        if (t >= g_keyframes[i].time && t <= g_keyframes[i+1].time) {
            float span = g_keyframes[i+1].time - g_keyframes[i].time;
            float f = (span > 0.001f) ? ((t - g_keyframes[i].time) / span) : 0.0f;
            LightKeyframe out;
            out.time = t;
            out.sunBrightness = lerpf(g_keyframes[i].sunBrightness, g_keyframes[i+1].sunBrightness, f);
            out.sunHeight     = lerpf(g_keyframes[i].sunHeight,     g_keyframes[i+1].sunHeight,     f);
            out.sunAngleX     = lerpf(g_keyframes[i].sunAngleX,     g_keyframes[i+1].sunAngleX,     f);
            out.cabinAmbient  = lerpf(g_keyframes[i].cabinAmbient,  g_keyframes[i+1].cabinAmbient,  f);
            out.lightShaft    = lerpf(g_keyframes[i].lightShaft,    g_keyframes[i+1].lightShaft,    f);
            out.doorBloom     = lerpf(g_keyframes[i].doorBloom,     g_keyframes[i+1].doorBloom,     f);
            out.skyWarmth     = lerpf(g_keyframes[i].skyWarmth,     g_keyframes[i+1].skyWarmth,     f);
            return out;
        }
    }
    return g_curLight;
}

float getScene4BloomStrength() {
    return evalLighting(seqTime).doorBloom;
}

static void setOrUpdateKeyframe(float t, const LightKeyframe& val) {
    for (size_t i = 0; i < g_keyframes.size(); ++i) {
        if (fabsf(g_keyframes[i].time - t) < 0.15f) {
            g_keyframes[i] = val;
            g_keyframes[i].time = t;
            return;
        }
    }
    LightKeyframe k = val;
    k.time = t;
    g_keyframes.push_back(k);
    std::sort(g_keyframes.begin(), g_keyframes.end(), [](const LightKeyframe& a, const LightKeyframe& b) {
        return a.time < b.time;
    });
}

// ============================================================================
// ASSET LOADERS
// ============================================================================
static GLuint loadTextureFromFile(const char* filename, bool repeat = true) {
    int w = 0, h = 0, ch = 0;
    unsigned char* data = stbi_load(filename, &w, &h, &ch, 3);
    if (!data) return 0;

    GLuint texID = 0;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, w, h, GL_RGB, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
    return texID;
}

static GLuint loadAstroMesh(const char* binFile) {
    FILE* fp = fopen(binFile, "rb");
    if (!fp) fp = fopen("astronaut.bin", "rb");
    if (!fp) return 0;

    char magic[4];
    uint32_t numVerts = 0;
    if (fread(magic, 1, 4, fp) != 4 || memcmp(magic, "ASTR", 4) != 0 ||
        fread(&numVerts, sizeof(uint32_t), 1, fp) != 1 || numVerts == 0) {
        fclose(fp);
        return 0;
    }

    std::vector<AstroVertex> verts(numVerts);
    size_t readCount = fread(&verts[0], sizeof(AstroVertex), numVerts, fp);
    fclose(fp);

    if (readCount != numVerts) return 0;

    GLuint dList = glGenLists(1);
    glNewList(dList, GL_COMPILE);

    glBegin(GL_TRIANGLES);
    for (size_t i = 0; i < verts.size(); ++i) {
        glNormal3f(verts[i].nx, verts[i].ny, verts[i].nz);
        glVertex3f(verts[i].x, verts[i].y, verts[i].z);
    }
    glEnd();

    glEndList();
    return dList;
}

static void loadAstroAssets() {
    if (astroLoaded || astroLoadFailed) return;

    astroTexID = loadTextureFromFile("astronaut_diffuse.png", true);

    const char* poseFiles[4] = {
        "astronaut_pose1.bin",
        "astronaut_pose2.bin",
        "astronaut_pose3.bin",
        "astronaut_pose4.bin"
    };

    for (int i = 0; i < 4; ++i) {
        astroDisplayLists[i] = loadAstroMesh(poseFiles[i]);
    }

    if (!astroDisplayLists[0]) {
        astroLoadFailed = true;
        return;
    }

    astroLoaded = true;
}

// ============================================================================
// HIGH-POLYGON PROCEDURAL MARTIAN DUNES & MOUNTAINS
// ============================================================================
static float evaluateDuneHeight(float x, float z) {
    if (z < 5.25f) return -0.74f;

    float blend = smoothstep(5.25f, 13.0f, z);

    // Multi-octave transverse and barchan sand dune waves
    float d1 = sinf(x * 0.048f + z * 0.034f) * 3.4f;
    float d2 = cosf(x * 0.095f - z * 0.058f) * 1.9f;
    float slip = powf(fabsf(sinf(z * 0.028f + x * 0.010f)), 1.8f) * 4.4f; // Sharp wind-blown crests
    float ripple1 = sinf(x * 0.28f + z * 0.20f) * 0.38f;
    float ripple2 = cosf(x * 0.60f - z * 0.45f) * 0.16f; // Fine micro sand grain ripples

    // Distant mountain ramp-up
    float distMtn = 0.0f;
    if (z > 90.0f) {
        float fz = (z - 90.0f) / 160.0f;
        distMtn = (sinf(x * 0.018f) * 24.0f + cosf(x * 0.032f) * 15.0f + 25.0f) * fz;
    }

    float duneH = d1 + d2 + slip + ripple1 + ripple2 + distMtn - 0.2f;
    return -0.74f * (1.0f - blend) + duneH * blend;
}

static void buildDuneMesh() {
    duneDisplayList = glGenLists(1);
    glNewList(duneDisplayList, GL_COMPILE);

    if (sandTexID) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, sandTexID);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }

    GLfloat matDiff[] = { 1.85f, 1.05f, 0.65f, 1.0f }; // Radiant sunlit Martian red sand
    GLfloat matAmb[]  = { 0.55f, 0.32f, 0.22f, 1.0f };
    GLfloat matSpec[] = { 0.45f, 0.30f, 0.20f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, matDiff);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, matAmb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 24.0f);

    const int GRID_X = 160;
    const int GRID_Z = 160;
    const float MIN_X = -180.0f, MAX_X = 180.0f;
    const float MIN_Z = 5.25f,   MAX_Z = 320.0f; // STRICTLY starts at ramp tip: ZERO sand inside ship!
    const float DX = (MAX_X - MIN_X) / (float)GRID_X;
    const float DZ = (MAX_Z - MIN_Z) / (float)GRID_Z;

    for (int iz = 0; iz < GRID_Z; ++iz) {
        float z0 = MIN_Z + iz * DZ;
        float z1 = z0 + DZ;

        glBegin(GL_TRIANGLE_STRIP);
        for (int ix = 0; ix <= GRID_X; ++ix) {
            float x = MIN_X + ix * DX;

            float y0 = evaluateDuneHeight(x, z0);
            float y1 = evaluateDuneHeight(x, z1);

            // Compute analytic normal at (x, z0)
            float eps = 0.4f;
            float hL0 = evaluateDuneHeight(x - eps, z0);
            float hR0 = evaluateDuneHeight(x + eps, z0);
            float hD0 = evaluateDuneHeight(x, z0 - eps);
            float hU0 = evaluateDuneHeight(x, z0 + eps);
            float nx0 = (hL0 - hR0) / (2.0f * eps);
            float nz0 = (hD0 - hU0) / (2.0f * eps);
            float ny0 = 1.0f;
            normalize3(nx0, ny0, nz0);

            // Compute analytic normal at (x, z1)
            float hL1 = evaluateDuneHeight(x - eps, z1);
            float hR1 = evaluateDuneHeight(x + eps, z1);
            float hD1 = evaluateDuneHeight(x, z1 - eps);
            float hU1 = evaluateDuneHeight(x, z1 + eps);
            float nx1 = (hL1 - hR1) / (2.0f * eps);
            float nz1 = (hD1 - hU1) / (2.0f * eps);
            float ny1 = 1.0f;
            normalize3(nx1, ny1, nz1);

            float u0 = x * 0.085f;
            float v0 = z0 * 0.085f;
            float u1 = x * 0.085f;
            float v1 = z1 * 0.085f;

            glNormal3f(nx0, ny0, nz0);
            glTexCoord2f(u0, v0);
            glVertex3f(x, y0, z0);

            glNormal3f(nx1, ny1, nz1);
            glTexCoord2f(u1, v1);
            glVertex3f(x, y1, z1);
        }
        glEnd();
    }

    if (sandTexID) {
        glDisable(GL_TEXTURE_2D);
    }

    glEndList();
}

// ============================================================================
// MARTIAN SKY DOME & SOLAR HORIZON DISC
// ============================================================================
static void buildSkyDome() {
    skyDomeDisplayList = glGenLists(1);
    glNewList(skyDomeDisplayList, GL_COMPILE);

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    const int SLICES = 56;
    const int STACKS = 28;
    const float RADIUS = 280.0f;

    for (int j = 0; j < STACKS; ++j) {
        float phi0 = (float)j / (float)STACKS * ((float)M_PI * 0.5f);
        float phi1 = (float)(j + 1) / (float)STACKS * ((float)M_PI * 0.5f);

        float t0 = (float)j / (float)STACKS;
        float t1 = (float)(j + 1) / (float)STACKS;

        // Radiant golden-white sun-bleached horizon transitioning to rich warm Martian amber sky
        // High-key values produce the blinding daylight contrast seen in Denis Villeneuve's Dune
        float r0 = 1.95f * (1.0f - t0 * 0.42f);
        float g0 = 1.55f * (1.0f - t0 * 0.56f);
        float b0 = 1.20f * (1.0f - t0 * 0.70f);

        float r1 = 1.95f * (1.0f - t1 * 0.42f);
        float g1 = 1.55f * (1.0f - t1 * 0.56f);
        float b1 = 1.20f * (1.0f - t1 * 0.70f);

        glBegin(GL_QUAD_STRIP);
        for (int i = 0; i <= SLICES; ++i) {
            float theta = (float)i / (float)SLICES * ((float)M_PI * 2.0f);

            float x0 = RADIUS * cosf(phi0) * sinf(theta);
            float y0 = RADIUS * sinf(phi0) - 10.0f;
            float z0 = RADIUS * cosf(phi0) * cosf(theta);

            float x1 = RADIUS * cosf(phi1) * sinf(theta);
            float y1 = RADIUS * sinf(phi1) - 10.0f;
            float z1 = RADIUS * cosf(phi1) * cosf(theta);

            glColor3f(r0, g0, b0);
            glVertex3f(x0, y0, z0);

            glColor3f(r1, g1, b1);
            glVertex3f(x1, y1, z1);
        }
        glEnd();
    }

    // Solar Disk & Horizon Glare (Blazing exterior sun centered through the hatch opening)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // Blinding core sun disc
    glPushMatrix();
    glTranslatef(0.0f, 6.2f, 150.0f);
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(1.0f, 0.98f, 0.92f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    for (int k = 0; k <= 32; ++k) {
        float a = (float)k / 32.0f * ((float)M_PI * 2.0f);
        glColor4f(1.0f, 0.88f, 0.65f, 0.0f);
        glVertex3f(cosf(a) * 14.0f, sinf(a) * 14.0f, 0.0f);
    }
    glEnd();

    // Horizontal anamorphic solar glare line
    glBegin(GL_QUADS);
    glColor4f(1.0f, 0.92f, 0.75f, 0.45f);
    glVertex3f(-120.0f, -0.6f, 0.0f);
    glVertex3f( 120.0f, -0.6f, 0.0f);
    glColor4f(1.0f, 0.92f, 0.75f, 0.0f);
    glVertex3f( 120.0f,  0.6f, 0.0f);
    glVertex3f(-120.0f,  0.6f, 0.0f);
    glEnd();

    glPopMatrix();
    glDisable(GL_BLEND);

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    glEndList();
}

// ============================================================================
// CABIN INTERIOR ARCHITECTURE & LOWERING RAMP
// ============================================================================
static void buildCabinInterior() {
    cabinDisplayList = glGenLists(1);
    glNewList(cabinDisplayList, GL_COMPILE);

    if (cabinTexID) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, cabinTexID);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }

    // Heavy dark industrial metal material
    GLfloat matDiff[] = { 0.42f, 0.40f, 0.40f, 1.0f };
    GLfloat matAmb[]  = { 0.12f, 0.11f, 0.11f, 1.0f };
    GLfloat matSpec[] = { 0.35f, 0.35f, 0.35f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, matDiff);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, matAmb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 32.0f);

    // 1. Deck Floor (-Z extends inside cabin from Z=0 to Z=-16, X in [-4.5, 4.5])
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-4.5f, 0.0f, 0.0f);
        glTexCoord2f(4.0f, 0.0f); glVertex3f( 4.5f, 0.0f, 0.0f);
        glTexCoord2f(4.0f, 8.0f); glVertex3f( 4.5f, 0.0f, -16.0f);
        glTexCoord2f(0.0f, 8.0f); glVertex3f(-4.5f, 0.0f, -16.0f);
    glEnd();

    // 2. Ceiling Bulkhead (-Z from 0 to -16, Y = 3.65m)
    glNormal3f(0.0f, -1.0f, 0.0f);
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-4.5f, 3.65f, -16.0f);
        glTexCoord2f(3.0f, 0.0f); glVertex3f( 4.5f, 3.65f, -16.0f);
        glTexCoord2f(3.0f, 6.0f); glVertex3f( 4.5f, 3.65f, 0.0f);
        glTexCoord2f(0.0f, 6.0f); glVertex3f(-4.5f, 3.65f, 0.0f);
    glEnd();

    // 3. Left Bulkhead Wall (X = -4.5m)
    glNormal3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-4.5f, 0.0f, -16.0f);
        glTexCoord2f(6.0f, 0.0f); glVertex3f(-4.5f, 0.0f, 0.0f);
        glTexCoord2f(6.0f, 2.0f); glVertex3f(-4.5f, 3.65f, 0.0f);
        glTexCoord2f(0.0f, 2.0f); glVertex3f(-4.5f, 3.65f, -16.0f);
    glEnd();

    // 4. Right Bulkhead Wall (X = +4.5m)
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(4.5f, 0.0f, 0.0f);
        glTexCoord2f(6.0f, 0.0f); glVertex3f(4.5f, 0.0f, -16.0f);
        glTexCoord2f(6.0f, 2.0f); glVertex3f(4.5f, 3.65f, -16.0f);
        glTexCoord2f(0.0f, 2.0f); glVertex3f(4.5f, 3.65f, 0.0f);
    glEnd();

    // 5. Heavy Vertical Stanchions / Structural Ribs along walls
    for (float rz = -2.0f; rz >= -14.0f; rz -= 3.0f) {
        // Left rib
        glNormal3f(1.0f, 0.0f, 0.0f);
        glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex3f(-4.15f, 0.0f, rz - 0.25f);
            glTexCoord2f(0.5f, 0.0f); glVertex3f(-4.15f, 0.0f, rz + 0.25f);
            glTexCoord2f(0.5f, 2.0f); glVertex3f(-4.15f, 3.65f, rz + 0.25f);
            glTexCoord2f(0.0f, 2.0f); glVertex3f(-4.15f, 3.65f, rz - 0.25f);
        glEnd();
        // Right rib
        glNormal3f(-1.0f, 0.0f, 0.0f);
        glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex3f(4.15f, 0.0f, rz + 0.25f);
            glTexCoord2f(0.5f, 0.0f); glVertex3f(4.15f, 0.0f, rz - 0.25f);
            glTexCoord2f(0.5f, 2.0f); glVertex3f(4.15f, 3.65f, rz - 0.25f);
            glTexCoord2f(0.0f, 2.0f); glVertex3f(4.15f, 3.65f, rz + 0.25f);
        glEnd();
    }

    // 6. Top Overhead Lintel / Frame Beam right above the ramp opening (Z = 0)
    // Exactly matches the top framing in the Denis Villeneuve Dune reference!
    glNormal3f(0.0f, 0.0f, -1.0f);
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-4.5f, 2.95f, -0.05f);
        glTexCoord2f(3.0f, 0.0f); glVertex3f( 4.5f, 2.95f, -0.05f);
        glTexCoord2f(3.0f, 0.6f); glVertex3f( 4.5f, 3.65f, -0.05f);
        glTexCoord2f(0.0f, 0.6f); glVertex3f(-4.5f, 3.65f, -0.05f);
    glEnd();

    // 7. Side Door Jambs (creating wide wall-to-wall opening width 8.2m, from X = -4.1 to +4.1)
    glNormal3f(0.0f, 0.0f, -1.0f);
    glBegin(GL_QUADS);
        // Left jamb
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-4.5f, 0.0f, -0.05f);
        glTexCoord2f(0.4f, 0.0f); glVertex3f(-4.1f, 0.0f, -0.05f);
        glTexCoord2f(0.4f, 2.5f); glVertex3f(-4.1f, 2.95f, -0.05f);
        glTexCoord2f(0.0f, 2.5f); glVertex3f(-4.5f, 2.95f, -0.05f);
        // Right jamb
        glTexCoord2f(0.0f, 0.0f); glVertex3f( 4.1f, 0.0f, -0.05f);
        glTexCoord2f(0.4f, 0.0f); glVertex3f( 4.5f, 0.0f, -0.05f);
        glTexCoord2f(0.4f, 2.5f); glVertex3f( 4.5f, 2.95f, -0.05f);
        glTexCoord2f(0.0f, 2.5f); glVertex3f( 4.1f, 2.95f, -0.05f);
    glEnd();

    if (cabinTexID) {
        glDisable(GL_TEXTURE_2D);
    }

    // 8. Yellow Hazard Caution Stripe along the ramp threshold (Z = 0)
    glDisable(GL_LIGHTING);
    glBegin(GL_QUADS);
    glColor3f(0.85f, 0.70f, 0.15f);
    glVertex3f(-4.1f, 0.005f, -0.35f);
    glVertex3f( 4.1f, 0.005f, -0.35f);
    glVertex3f( 4.1f, 0.005f, -0.05f);
    glVertex3f(-4.1f, 0.005f, -0.05f);
    glEnd();
    glEnable(GL_LIGHTING);

    glEndList();
}

static void renderLoweringRamp(float openProg) {
    // openProg: 0.0 = fully closed (upright at 0 deg), 1.0 = fully opened onto ground (98 deg = gentle -8 deg ramp)
    float rotAngle = openProg * 98.0f; // from 0 deg (sealed upright) down to 98 deg (descending ramp onto sand)

    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 0.0f); // Pivot hinge at floor threshold (0, 0, 0)
    glRotatef(rotAngle, 1.0f, 0.0f, 0.0f);

    if (cabinTexID) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, cabinTexID);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }

    GLfloat matDiff[] = { 0.48f, 0.46f, 0.46f, 1.0f };
    GLfloat matAmb[]  = { 0.15f, 0.14f, 0.14f, 1.0f };
    GLfloat matSpec[] = { 0.45f, 0.45f, 0.45f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, matDiff);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, matAmb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpec);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 32.0f);

    const float RW = 4.10f;  // Half-width = 4.1m (total wall-to-wall width = 8.2m)
    const float RL = 5.20f;  // Length of ramp = 5.2m
    const float THICK = 0.22f; // Thickness = 22cm armored plate

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    // 1. Walking surface (faces -Z into cabin when upright, faces upward when lowered)
    glNormal3f(0.0f, 0.0f, -1.0f);
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-RW, 0.0f, 0.0f);
        glTexCoord2f(3.0f, 0.0f); glVertex3f( RW, 0.0f, 0.0f);
        glTexCoord2f(3.0f, 4.0f); glVertex3f( RW,  RL,  0.0f);
        glTexCoord2f(0.0f, 4.0f); glVertex3f(-RW,  RL,  0.0f);
    glEnd();

    // 2. Exterior armored back (faces +Z into exterior when upright, faces ground when lowered)
    glNormal3f(0.0f, 0.0f, 1.0f);
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-RW,  RL,  THICK);
        glTexCoord2f(3.0f, 0.0f); glVertex3f( RW,  RL,  THICK);
        glTexCoord2f(3.0f, 4.0f); glVertex3f( RW, 0.0f, THICK);
        glTexCoord2f(0.0f, 4.0f); glVertex3f(-RW, 0.0f, THICK);
    glEnd();

    // 3. Left edge
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
        glVertex3f(-RW, 0.0f, 0.0f);
        glVertex3f(-RW,  RL,  0.0f);
        glVertex3f(-RW,  RL,  THICK);
        glVertex3f(-RW, 0.0f, THICK);
    glEnd();

    // 4. Right edge
    glNormal3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
        glVertex3f( RW, 0.0f, THICK);
        glVertex3f( RW,  RL,  THICK);
        glVertex3f( RW,  RL,  0.0f);
        glVertex3f( RW, 0.0f, 0.0f);
    glEnd();

    // 5. Far top tip (touches ground when lowered)
    glNormal3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_QUADS);
        glVertex3f(-RW, RL, 0.0f);
        glVertex3f( RW, RL, 0.0f);
        glVertex3f( RW, RL, THICK);
        glVertex3f(-RW, RL, THICK);
    glEnd();

    if (cabinTexID) {
        glDisable(GL_TEXTURE_2D);
    }

    // Heavy reinforcement longitudinal ribs across the walking ramp surface
    glDisable(GL_LIGHTING);
    glColor3f(0.20f, 0.19f, 0.19f);
    for (float rx = -3.2f; rx <= 3.2f; rx += 1.6f) {
        glBegin(GL_QUADS);
            glVertex3f(rx - 0.04f, 0.05f, -0.015f);
            glVertex3f(rx + 0.04f, 0.05f, -0.015f);
            glVertex3f(rx + 0.04f, RL - 0.05f, -0.015f);
            glVertex3f(rx - 0.04f, RL - 0.05f, -0.015f);
        glEnd();
    }
    glEnable(GL_LIGHTING);

    glPopMatrix();
}

// ============================================================================
// INITIALIZATION
// ============================================================================
static void initScene4() {
    if (scene4Initialized) return;

    reloadScene4Lighting();
    loadAstroAssets();
    sandTexID = loadTextureFromFile("mars_dune_sand_diffuse.jpg", true);
    cabinTexID = loadTextureFromFile("cabin_metal_diffuse.jpg", true);

    buildSkyDome();
    buildDuneMesh();
    buildCabinInterior();

    // Initialize floating dust motes inside cabin
    cabinMotes.resize(90);
    for (size_t i = 0; i < cabinMotes.size(); ++i) {
        cabinMotes[i].x = randRange(-3.0f, 3.0f);
        cabinMotes[i].y = randRange(0.2f, 2.8f);
        cabinMotes[i].z = randRange(-6.5f, 0.5f);
        cabinMotes[i].vx = randRange(-0.015f, 0.015f);
        cabinMotes[i].vy = randRange(-0.008f, 0.012f);
        cabinMotes[i].vz = randRange(-0.010f, 0.015f);
        cabinMotes[i].size = randRange(1.8f, 3.4f);
        cabinMotes[i].alpha = randRange(0.25f, 0.70f);
        cabinMotes[i].phase = randRange(0.0f, 6.28f);
    }

    scene4Initialized = true;
}

// ============================================================================
// UPDATE LOGIC
// ============================================================================
void updateScene4(float dt) {
    initScene4();

    if (g_hudMsgTimer > 0.0f) {
        g_hudMsgTimer -= dt;
    }

    // Animate floating dust motes in cabin
    for (size_t i = 0; i < cabinMotes.size(); ++i) {
        cabinMotes[i].x += cabinMotes[i].vx * dt;
        cabinMotes[i].y += cabinMotes[i].vy * dt;
        cabinMotes[i].z += cabinMotes[i].vz * dt;
        cabinMotes[i].phase += dt * 1.5f;

        if (cabinMotes[i].y < 0.1f) cabinMotes[i].y = 2.8f;
        if (cabinMotes[i].y > 2.9f) cabinMotes[i].y = 0.2f;
        if (cabinMotes[i].z < -7.0f) cabinMotes[i].z = 0.5f;
        if (cabinMotes[i].z > 1.0f) cabinMotes[i].z = -6.5f;
    }
}

// ============================================================================
// CAMERA CHOREOGRAPHY (SINGLE CONTINUOUS SHOT)
// ============================================================================
void computeCameraScene4(float& eyeX, float& eyeY, float& eyeZ,
                         float& targetX, float& targetY, float& targetZ,
                         float& upX, float& upY, float& upZ) {
    upX = 0.0f; upY = 1.0f; upZ = 0.0f;

    float t = seqTime;

    // The single continuous Dune shot runs 61.0s to 73.0s (12.0s duration)
    // Camera is positioned inside the dark cabin behind the 4 crew members,
    // looking past them through the opened hatch onto the Martian dunes.
    float prog = clampf((t - 61.0f) / 12.0f, 0.0f, 1.0f);
    float smoothProg = prog * prog * (3.0f - 2.0f * prog);

    // Very slow, majestic cinematic push-in dolly forward
    eyeX = 0.0f;
    eyeY = 1.52f + smoothProg * 0.08f; // eye level ~1.55m
    eyeZ = -7.40f + smoothProg * 1.50f; // slowly creeps from -7.4m to -5.9m

    // Subtle natural human breathing / camera operator sway
    eyeX += sinf(t * 0.75f) * 0.008f;
    eyeY += cosf(t * 0.60f) * 0.005f;

    // Look directly past the crew toward the distant Martian horizon
    targetX = 0.0f;
    targetY = 1.35f + smoothProg * 0.08f;
    targetZ = 35.0f;
}

// ============================================================================
// ASTRONAUT CREW RENDERING
// ============================================================================
static void renderAstronautCrew(float openProg) {
    if (!astroDisplayLists[0]) return;

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);

    struct CrewMember {
        float x, z;
        float scale;
        float rotY;
        float tiltX;
    };

    CrewMember crew[4] = {
        { -1.50f, -3.45f, 0.84f, -4.0f,  0.5f }, // Member 1 (Scout: hand on hip, head turned)
        { -0.50f, -3.10f, 0.92f, -1.0f, -0.8f }, // Member 2 (Commander: parade rest, tall upright)
        {  0.50f, -3.10f, 0.79f,  2.0f,  1.2f }, // Member 3 (Scientist: gazing up, sensor in hand, shorter)
        {  1.50f, -3.45f, 0.85f,  5.0f, -0.4f }  // Member 4 (Engineer: high-gain comms antenna mast)
    };

    for (int i = 0; i < 4; ++i) {
        glPushMatrix();
        glTranslatef(crew[i].x, 0.0f, crew[i].z);
        glRotatef(crew[i].rotY, 0.0f, 1.0f, 0.0f);
        glRotatef(crew[i].tiltX, 1.0f, 0.0f, 0.0f);
        glScalef(crew[i].scale, crew[i].scale, crew[i].scale);

        // 1. Primary Solid Silhouette Pass (Deep Dark Interior Shadow)
        glColor3f(0.042f, 0.038f, 0.035f);
        GLuint dl = astroDisplayLists[i] ? astroDisplayLists[i] : astroDisplayLists[0];
        glCallList(dl);

        glPopMatrix();
    }

    glEnable(GL_LIGHTING);
}

// ============================================================================
// MAIN SCENE 4 RENDERER
// ============================================================================
void renderScene4() {
    glDisable(GL_LIGHT1); // Disable planetary bounce light so interior cabin stays properly dark
    float t = seqTime;

    // Ramp animation timing:
    // 61.0s – 63.2s: Closed (hold in dark interior)
    // 63.2s – 66.8s: Opening smoothly (hydraulic release & lowering)
    // 66.8s – 73.0s: Fully lowered onto red sand
    float openProg = 0.0f;
    if (t >= 63.2f) {
        openProg = clampf((t - 63.2f) / 3.6f, 0.0f, 1.0f);
        openProg = openProg * openProg * (3.0f - 2.0f * openProg); // Smooth cubic ease
    }

    // Configure Lighting for Scene 4:
    // Blinding exterior Martian sun coming from outside the ramp (+Z) shining back into cabin (-Z)
    LightKeyframe curLight = evalLighting(t);

    GLfloat sunPos[4]  = { curLight.sunAngleX, curLight.sunHeight, 55.0f, 0.0f };
    float sB = curLight.sunBrightness / 2.40f;
    float sW = curLight.skyWarmth;
    GLfloat sunDiff[4] = { 2.40f * sB * sW, 1.75f * sB, 1.30f * sB * (2.0f - sW), 1.0f };
    GLfloat sunAmb[4]  = { curLight.cabinAmbient, curLight.cabinAmbient * 0.67f, curLight.cabinAmbient * 0.53f, 1.0f };
    GLfloat sunSpec[4] = { 1.80f * sB, 1.45f * sB, 1.10f * sB, 1.0f };

    glLightfv(GL_LIGHT0, GL_POSITION, sunPos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  sunDiff);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  sunAmb);
    glLightfv(GL_LIGHT0, GL_SPECULAR, sunSpec);

    // 1. Martian Sky Dome
    if (skyDomeDisplayList) {
        glCallList(skyDomeDisplayList);
    }

    // 2. High-Polygon Procedural Sand Dunes & Distant Mountain Ridges
    if (duneDisplayList) {
        glCallList(duneDisplayList);
    }

    // 3. Spacecraft Cabin Interior Architecture (Bulkheads, Deck & Ceiling Lintel)
    if (cabinDisplayList) {
        glCallList(cabinDisplayList);
    }

    // 4. Heavy Lowering Cargo Ramp
    renderLoweringRamp(openProg);

    // 5. Volumetric Light Shaft spilling onto the cabin deck from the lowering door
    if (openProg > 0.01f && curLight.lightShaft > 0.005f) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive sunbeam
        glDisable(GL_LIGHTING);
        glDepthMask(GL_FALSE);

        float intensity = curLight.lightShaft * clampf(openProg * 1.4f, 0.0f, 1.0f);

        // A. Soft Volumetric Light Shafts in the air:
        // Firmly anchored at the TOP LINTEL (Y = 2.95m, Z = 0.0m) at all times
        const int numShaftLayers = 6;
        for (int layer = 0; layer < numShaftLayers; ++layer) {
            float layerFrac = (float)layer / (float)(numShaftLayers - 1);
            float topY = 2.95f; 
            float targetZ = lerpf(-3.8f, -6.2f, layerFrac);
            float layerAlpha = intensity * 0.16f * (1.0f - layerFrac * 0.35f);

            glBegin(GL_TRIANGLE_FAN);
                // Center apex along top lintel
                glColor4f(1.0f, 0.90f, 0.72f, layerAlpha);
                glVertex3f(0.0f, topY, 0.0f);

                // Outer left boundary fading to zero
                glColor4f(1.0f, 0.88f, 0.65f, 0.0f);
                glVertex3f(-4.0f, topY, 0.0f);
                glVertex3f(-4.3f, 0.02f, targetZ);

                // Floor interior illumination
                glColor4f(1.0f, 0.90f, 0.70f, layerAlpha * 0.75f);
                glVertex3f(-2.0f, 0.02f, targetZ);
                glVertex3f( 0.0f, 0.02f, targetZ);
                glVertex3f( 2.0f, 0.02f, targetZ);

                // Outer right boundary fading to zero
                glColor4f(1.0f, 0.88f, 0.65f, 0.0f);
                glVertex3f( 4.3f, 0.02f, targetZ);
                glVertex3f( 4.0f, topY, 0.0f);
            glEnd();
        }

        // B. Floor light pool: soft trapezoid with lateral falloff
        const int floorSlices = 5;
        for (int i = 0; i < floorSlices; ++i) {
            float f0 = (float)i / (float)floorSlices;
            float f1 = (float)(i + 1) / (float)floorSlices;
            float z0 = -f0 * 5.8f;
            float z1 = -f1 * 5.8f;
            float w0 = lerpf(3.6f, 4.2f, f0);
            float w1 = lerpf(3.6f, 4.2f, f1);
            float a0 = intensity * 0.28f * (1.0f - f0 * 0.80f);
            float a1 = intensity * 0.28f * (1.0f - f1 * 0.80f);

            glBegin(GL_QUAD_STRIP);
                glColor4f(1.0f, 0.88f, 0.65f, 0.0f);
                glVertex3f(-w0, 0.02f, z0);
                glColor4f(1.0f, 0.88f, 0.65f, 0.0f);
                glVertex3f(-w1, 0.02f, z1);

                glColor4f(1.0f, 0.88f, 0.65f, a0 * 0.7f);
                glVertex3f(-w0 * 0.6f, 0.02f, z0);
                glColor4f(1.0f, 0.88f, 0.65f, a1 * 0.7f);
                glVertex3f(-w1 * 0.6f, 0.02f, z1);

                glColor4f(1.0f, 0.90f, 0.70f, a0);
                glVertex3f(0.0f, 0.02f, z0);
                glColor4f(1.0f, 0.90f, 0.70f, a1);
                glVertex3f(0.0f, 0.02f, z1);

                glColor4f(1.0f, 0.88f, 0.65f, a0 * 0.7f);
                glVertex3f(w0 * 0.6f, 0.02f, z0);
                glColor4f(1.0f, 0.88f, 0.65f, a1 * 0.7f);
                glVertex3f(w1 * 0.6f, 0.02f, z1);

                glColor4f(1.0f, 0.88f, 0.65f, 0.0f);
                glVertex3f(w0, 0.02f, z0);
                glColor4f(1.0f, 0.88f, 0.65f, 0.0f);
                glVertex3f(w1, 0.02f, z1);
            glEnd();
        }

        glDepthMask(GL_TRUE);
        glEnable(GL_LIGHTING);
        glDisable(GL_BLEND);
    }

    // 6. The 4 Astronauts in Monumental Silhouette (The Dune Shot)
    renderAstronautCrew(openProg);

    // 7. Floating Dust Motes swirling in the sunlit cabin
    if (openProg > 0.05f) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDisable(GL_LIGHTING);
        glDepthMask(GL_FALSE);

        for (size_t i = 0; i < cabinMotes.size(); ++i) {
            float pulse = 0.65f + 0.35f * sinf(cabinMotes[i].phase);
            float a = cabinMotes[i].alpha * openProg * pulse * 0.65f;
            glColor4f(1.0f, 0.88f, 0.65f, a);

            glPointSize(cabinMotes[i].size);
            glBegin(GL_POINTS);
                glVertex3f(cabinMotes[i].x, cabinMotes[i].y, cabinMotes[i].z);
            glEnd();
        }

        glDepthMask(GL_TRUE);
        glEnable(GL_LIGHTING);
        glDisable(GL_BLEND);
    }

    glEnable(GL_LIGHT1);
}

// ============================================================================
// ON-SCREEN INTERACTIVE LIGHTING HUD & KEYBOARD GRADING
// ============================================================================
static void drawHUDString(float x, float y, const char* str) {
    glRasterPos2f(x, y);
    while (*str) {
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, *str++);
    }
}

void renderScene4HUD(int windowWidth, int windowHeight) {
    if (!g_showLightingHUD || seqTime < 60.5f) return;

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, windowWidth, 0, windowHeight);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    float panelW = 480.0f;
    float panelH = 180.0f;
    float padX = 25.0f;
    float padY = 25.0f;

    // Semi-transparent background
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.04f, 0.05f, 0.08f, 0.88f);
    glBegin(GL_QUADS);
        glVertex2f(padX, padY);
        glVertex2f(padX + panelW, padY);
        glVertex2f(padX + panelW, padY + panelH);
        glVertex2f(padX, padY + panelH);
    glEnd();

    // Golden border
    glColor4f(0.95f, 0.50f, 0.15f, 0.95f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(padX, padY);
        glVertex2f(padX + panelW, padY);
        glVertex2f(padX + panelW, padY + panelH);
        glVertex2f(padX, padY + panelH);
    glEnd();

    LightKeyframe cur = evalLighting(seqTime);

    char buf[128];
    float textX = padX + 14.0f;
    float textY = padY + panelH - 22.0f;

    glColor3f(1.0f, 0.65f, 0.20f);
    snprintf(buf, sizeof(buf), "MARTIAN LIGHTING GRADING HUD [%s]", isPaused ? "PAUSED" : "PLAYING");
    drawHUDString(textX, textY, buf);

    textY -= 18.0f;
    glColor3f(0.85f, 0.85f, 0.85f);
    snprintf(buf, sizeof(buf), "Time: %.2fs / 73.00s | Keyframes Defined: %d", seqTime, (int)g_keyframes.size());
    drawHUDString(textX, textY, buf);

    textY -= 20.0f;
    glColor3f(0.35f, 0.85f, 1.0f);
    snprintf(buf, sizeof(buf), "[U / J] Sun Brightness:  %.2f", cur.sunBrightness);
    drawHUDString(textX, textY, buf);

    glColor3f(0.35f, 0.85f, 1.0f);
    snprintf(buf, sizeof(buf), "[I / K] Cabin Ambient:   %.2f", cur.cabinAmbient);
    drawHUDString(textX + 230.0f, textY, buf);

    textY -= 16.0f;
    glColor3f(0.35f, 0.85f, 1.0f);
    snprintf(buf, sizeof(buf), "[O / 9] Sun Elevation:  %.1f", cur.sunHeight);
    drawHUDString(textX, textY, buf);

    glColor3f(0.35f, 0.85f, 1.0f);
    snprintf(buf, sizeof(buf), "[P / ;] Door Bloom:      %.2f", cur.doorBloom);
    drawHUDString(textX + 230.0f, textY, buf);

    textY -= 16.0f;
    glColor3f(0.35f, 0.85f, 1.0f);
    snprintf(buf, sizeof(buf), "[Y / H] Light Shaft:     %.2f", cur.lightShaft);
    drawHUDString(textX, textY, buf);

    glColor3f(0.35f, 0.85f, 1.0f);
    snprintf(buf, sizeof(buf), "[T / G] Sky Warmth:      %.2f", cur.skyWarmth);
    drawHUDString(textX + 230.0f, textY, buf);

    textY -= 20.0f;
    glColor3f(0.70f, 0.70f, 0.70f);
    snprintf(buf, sizeof(buf), "[< / >] Scrub 0.1s | [[ / ]] 1-Frame | [Space] Pause");
    drawHUDString(textX, textY, buf);

    textY -= 16.0f;
    glColor3f(0.30f, 1.0f, 0.40f);
    snprintf(buf, sizeof(buf), "[Enter / S] Save Keyframe | [Del] Delete | [L] Toggle HUD");
    drawHUDString(textX, textY, buf);

    if (g_hudMsgTimer > 0.0f) {
        glColor3f(1.0f, 0.90f, 0.30f);
        drawHUDString(padX, padY - 16.0f, g_hudStatusMsg);
    }

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

bool handleScene4Keyboard(unsigned char key) {
    if (seqTime < 60.5f) return false;

    float t = seqTime;
    LightKeyframe cur = evalLighting(t);
    bool changed = false;

    if (key == 'l' || key == 'L') {
        g_showLightingHUD = !g_showLightingHUD;
        return true;
    } else if (key == 'u' || key == 'U') {
        cur.sunBrightness += (key == 'U') ? 0.20f : 0.05f;
        changed = true;
    } else if (key == 'j' || key == 'J') {
        cur.sunBrightness = std::max(0.10f, cur.sunBrightness - ((key == 'J') ? 0.20f : 0.05f));
        changed = true;
    } else if (key == 'i' || key == 'I') {
        cur.cabinAmbient = std::min(0.50f, cur.cabinAmbient + 0.01f);
        changed = true;
    } else if (key == 'k' || key == 'K') {
        cur.cabinAmbient = std::max(0.01f, cur.cabinAmbient - 0.01f);
        changed = true;
    } else if (key == 'o' || key == 'O') {
        cur.sunHeight += 1.0f;
        changed = true;
    } else if (key == '9' || key == '(') {
        cur.sunHeight = std::max(2.0f, cur.sunHeight - 1.0f);
        changed = true;
    } else if (key == 'p' || key == 'P') {
        cur.doorBloom = std::min(1.50f, cur.doorBloom + 0.05f);
        changed = true;
    } else if (key == ';' || key == ':') {
        cur.doorBloom = std::max(0.00f, cur.doorBloom - 0.05f);
        changed = true;
    } else if (key == 'y' || key == 'Y') {
        cur.lightShaft = std::min(1.00f, cur.lightShaft + 0.03f);
        changed = true;
    } else if (key == 'h' || key == 'H') {
        cur.lightShaft = std::max(0.00f, cur.lightShaft - 0.03f);
        changed = true;
    } else if (key == 't' || key == 'T') {
        cur.skyWarmth = std::min(2.00f, cur.skyWarmth + 0.05f);
        changed = true;
    } else if (key == 'g' || key == 'G') {
        cur.skyWarmth = std::max(0.30f, cur.skyWarmth - 0.05f);
        changed = true;
    } else if (key == 13 || key == 's' || key == 'S') { // Enter or S
        setOrUpdateKeyframe(t, cur);
        saveLightingKeyframes();
        return true;
    } else if (key == 8 || key == 127) { // Backspace or Del
        for (size_t i = 0; i < g_keyframes.size(); ++i) {
            if (fabsf(g_keyframes[i].time - t) < 0.25f) {
                g_keyframes.erase(g_keyframes.begin() + i);
                saveLightingKeyframes();
                snprintf(g_hudStatusMsg, sizeof(g_hudStatusMsg), "Deleted keyframe near %.2fs", t);
                g_hudMsgTimer = 3.0f;
                return true;
            }
        }
    } else if (key == '[' || key == '{') { // Fine frame step back (1 frame = 0.016s)
        seqTime = clampf(seqTime - 0.01667f, 61.0f, SEQUENCE_DURATION - 0.1f);
        return true;
    } else if (key == ']' || key == '}') { // Fine frame step forward (1 frame = 0.016s)
        seqTime = clampf(seqTime + 0.01667f, 61.0f, SEQUENCE_DURATION - 0.1f);
        return true;
    }

    if (changed) {
        setOrUpdateKeyframe(t, cur);
        snprintf(g_hudStatusMsg, sizeof(g_hudStatusMsg), "T=%.2fs | Sun=%.2f Amb=%.2f Bloom=%.2f [S to Save]",
                 t, cur.sunBrightness, cur.cabinAmbient, cur.doorBloom);
        g_hudMsgTimer = 3.0f;
        return true;
    }
    return false;
}

bool handleScene4Special(int key) {
    if (seqTime < 60.5f) return false;
    if (key == GLUT_KEY_LEFT) {
        seqTime = clampf(seqTime - 0.10f, 61.0f, SEQUENCE_DURATION - 0.1f);
        return true;
    } else if (key == GLUT_KEY_RIGHT) {
        seqTime = clampf(seqTime + 0.10f, 61.0f, SEQUENCE_DURATION - 0.1f);
        return true;
    }
    return false;
}
