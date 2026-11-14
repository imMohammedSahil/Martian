#include "Scene3.h"
#include "SceneDirector.h"
#include "Common.h"
#include "Spacecraft.h"
#include "stb_image.h"
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <vector>

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

// ============================================================================
// STRUCTURES & STATIC STATE
// ============================================================================
struct TerrainVertex {
    float x, y, z;
    float nx, ny, nz;
    float u, v;
    float r, g, b;
};

static GLuint craterDisplayList = 0;
static bool   craterLoaded = false;
static bool   craterLoadFailed = false;

static GLuint parachuteDisplayList = 0;
static GLuint parachuteTexID = 0;
static bool   parachuteLoaded = false;
static bool   parachuteLoadFailed = false;

static GLuint smokeTexID = 0;
static GLuint sparkTexID = 0;

static bool   soundTriggered = false;

// Dynamic Scene 3 Lander State
static float s3ShipX = 0.0f, s3ShipY = 0.0f, s3ShipZ = 0.0f;
static float s3ShipDirX = 0.0f, s3ShipDirY = -0.5f, s3ShipDirZ = 1.0f;
static float s3ShipRoll = 0.0f, s3ShipPitch = 0.0f;
static float s3ShipScale = 0.50f;
static float s3HeatGlow = 0.0f; // 0.0 to 1.0 heatshield incandescence

static float s3ChuteDeploy = 0.0f; // 0.0 = packed, 1.0 = fully inflated
static float s3ChuteX = 0.0f, s3ChuteY = 0.0f, s3ChuteZ = 0.0f;
static bool  s3ChuteCut = false;

// Touchdown Dust Ring Particles
struct TouchdownDust {
    float x, y, z;
    float vx, vy, vz;
    float scale;
    float alpha;
    float maxLife;
    float life;
};
static std::vector<TouchdownDust> s3Dust;

// Atmospheric Cloud Vapor Shred Streaks
struct VaporStreak {
    float x, y, z;
    float length;
    float speed;
    float alpha;
    float width;
};
static std::vector<VaporStreak> s3Vapor;

// ============================================================================
// ASSET LOADERS
// ============================================================================
static GLuint loadTextureHelper(const char* filename, bool repeat) {
    const char* paths[] = {
        filename,
        "../",
        "bin/Release/",
        "bin/Debug/"
    };
    unsigned char* data = NULL;
    int w = 0, h = 0, ch = 0;
    char fullpath[256];
    for (int i = 0; i < 4; ++i) {
        if (i == 0) snprintf(fullpath, sizeof(fullpath), "%s", filename);
        else snprintf(fullpath, sizeof(fullpath), "%s%s", paths[i], filename);
        data = stbi_load(fullpath, &w, &h, &ch, 4);
        if (data) break;
    }
    if (!data) return 0;

    GLuint texID = 0;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
    return texID;
}

static void loadGaleCraterMesh() {
    if (craterLoaded || craterLoadFailed) return;

    const char* paths[] = {
        "gale_crater.bin",
        "../gale_crater.bin",
        "bin/Release/gale_crater.bin",
        "bin/Debug/gale_crater.bin",
        "C:/Holidays/Martian/gale_crater.bin"
    };
    FILE* fp = NULL;
    for (int i = 0; i < 5; ++i) {
        fp = fopen(paths[i], "rb");
        if (fp) break;
    }
    if (!fp) {
        craterLoadFailed = true;
        return;
    }

    char magic[4];
    uint32_t numVerts = 0;
    if (fread(magic, 1, 4, fp) != 4 || memcmp(magic, "CRTR", 4) != 0 ||
        fread(&numVerts, sizeof(uint32_t), 1, fp) != 1 || numVerts == 0) {
        fclose(fp);
        craterLoadFailed = true;
        return;
    }

    std::vector<TerrainVertex> verts(numVerts);
    size_t readCount = fread(&verts[0], sizeof(TerrainVertex), numVerts, fp);
    fclose(fp);

    if (readCount != numVerts) {
        craterLoadFailed = true;
        return;
    }

    craterDisplayList = glGenLists(1);
    glNewList(craterDisplayList, GL_COMPILE);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);

    glVertexPointer(3, GL_FLOAT, sizeof(TerrainVertex), &verts[0].x);
    glNormalPointer(GL_FLOAT, sizeof(TerrainVertex), &verts[0].nx);
    glColorPointer(3, GL_FLOAT, sizeof(TerrainVertex), &verts[0].r);

    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)numVerts);

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    glEndList();
    craterLoaded = true;
}

static void loadParachuteMesh() {
    if (parachuteLoaded || parachuteLoadFailed) return;

    parachuteTexID = loadTextureHelper("parachute_diffuse.png", false);

    const char* paths[] = {
        "parachute.bin",
        "../parachute.bin",
        "bin/Release/parachute.bin",
        "bin/Debug/parachute.bin",
        "C:/Holidays/Martian/parachute.bin"
    };
    FILE* fp = NULL;
    for (int i = 0; i < 5; ++i) {
        fp = fopen(paths[i], "rb");
        if (fp) break;
    }
    if (!fp) {
        parachuteLoadFailed = true;
        return;
    }

    char magic[4];
    uint32_t numVerts = 0;
    if (fread(magic, 1, 4, fp) != 4 || memcmp(magic, "CHUT", 4) != 0 ||
        fread(&numVerts, sizeof(uint32_t), 1, fp) != 1 || numVerts == 0) {
        fclose(fp);
        parachuteLoadFailed = true;
        return;
    }

    std::vector<TerrainVertex> verts(numVerts);
    size_t readCount = fread(&verts[0], sizeof(TerrainVertex), numVerts, fp);
    fclose(fp);

    if (readCount != numVerts) {
        parachuteLoadFailed = true;
        return;
    }

    parachuteDisplayList = glGenLists(1);
    glNewList(parachuteDisplayList, GL_COMPILE);

    if (parachuteTexID) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, parachuteTexID);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }

    GLfloat pDiff[] = { 1.40f, 1.35f, 1.30f, 1.0f };
    GLfloat pAmb[]  = { 0.85f, 0.80f, 0.75f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, pDiff);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, pAmb);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glVertexPointer(3, GL_FLOAT, sizeof(TerrainVertex), &verts[0].x);
    glNormalPointer(GL_FLOAT, sizeof(TerrainVertex), &verts[0].nx);
    glTexCoordPointer(2, GL_FLOAT, sizeof(TerrainVertex), &verts[0].u);

    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)numVerts);

    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    if (parachuteTexID) {
        glDisable(GL_TEXTURE_2D);
    }

    glEndList();
    parachuteLoaded = true;
}

void initScene3() {
    loadGaleCraterMesh();
    loadParachuteMesh();
    if (!smokeTexID) smokeTexID = loadTextureHelper("smoke1.png", false);
    if (!sparkTexID) sparkTexID = loadTextureHelper("spark1.png", false);

    if (s3Vapor.empty()) {
        for (int i = 0; i < 45; ++i) {
            VaporStreak v;
            v.x = randRange(-8.0f, 8.0f);
            v.y = randRange(-3.0f, 6.0f);
            v.z = randRange(-12.0f, 12.0f);
            v.length = randRange(4.0f, 10.0f);
            v.speed  = randRange(28.0f, 48.0f);
            v.alpha  = randRange(0.25f, 0.65f);
            v.width  = randRange(1.8f, 3.8f);
            s3Vapor.push_back(v);
        }
    }
}

// ============================================================================
// SCENE 3 UPDATE (PHYSICS & PHASES)
// ============================================================================
void updateScene3(float dt) {
    initScene3();

    float t = seqTime;

    // Trigger authentic NASA Martian wind sound when hitting the atmosphere
    if (!soundTriggered && t >= 44.0f) {
        soundTriggered = true;
        PlaySoundA("mars_wind_nasa.wav", NULL, SND_FILENAME | SND_ASYNC | SND_NOSTOP);
    }

    // Update tearing atmospheric cloud streaks
    for (size_t i = 0; i < s3Vapor.size(); ++i) {
        s3Vapor[i].z -= s3Vapor[i].speed * dt;
        s3Vapor[i].x -= s3Vapor[i].speed * 0.45f * dt;
        if (s3Vapor[i].z < s3ShipZ - 14.0f) {
            s3Vapor[i].z = s3ShipZ + randRange(12.0f, 22.0f);
            s3Vapor[i].x = s3ShipX + randRange(-6.0f, 8.0f);
            s3Vapor[i].y = s3ShipY + randRange(-2.5f, 5.0f);
        }
    }

    // ------------------------------------------------------------------------
    // SHOT 1: SIDE-PROFILE ATMOSPHERIC PIERCE & HEATSHIELD TEAR (44.0s - 47.5s)
    // ------------------------------------------------------------------------
    if (t < 47.5f) {
        float u = (t - 44.0f) / 3.5f;
        s3ShipX = lerpf(-18.0f, -10.0f, u);
        s3ShipY = lerpf( 15.0f,   8.5f, u * u);
        s3ShipZ = lerpf(-20.0f, -10.0f, u);

        s3ShipDirX =  0.65f;
        s3ShipDirY = -0.45f;
        s3ShipDirZ =  0.75f;
        normalize3(s3ShipDirX, s3ShipDirY, s3ShipDirZ);

        s3ShipPitch = -18.0f;
        s3ShipRoll  = lerpf(25.0f, 15.0f, u);
        s3ShipScale = 0.50f;
        s3HeatGlow  = lerpf(0.60f, 1.0f, u); // Intense white-hot underbelly glow

        // Streaming thermal sparks & plasma whip
        for (int k = 0; k < 6; ++k) {
            float ox = randRange(-0.25f, 0.25f);
            float oy = randRange(-0.25f, 0.05f); // from heatshield underside
            float oz = randRange(-0.25f, 0.25f);
            spawnParticle(s3ShipX + ox, s3ShipY + oy, s3ShipZ + oz,
                          -s3ShipDirX * 14.0f + randRange(-1.5f, 1.5f),
                          -s3ShipDirY * 14.0f + randRange(-1.0f, 1.0f),
                          -s3ShipDirZ * 14.0f + randRange(-1.5f, 1.5f),
                          randRange(0.18f, 0.40f), randRange(1.8f, 3.2f),
                          1.0f, randRange(0.55f, 0.95f), 0.20f, true);
        }
    }
    // ------------------------------------------------------------------------
    // SHOT 2: HYPERSONIC OVER-THE-SHOULDER DIVE (47.5s - 51.5s)
    // ------------------------------------------------------------------------
    else if (t < 51.5f) {
        float u = (t - 47.5f) / 4.0f;
        float ease = u * u;

        s3ShipX = lerpf(-10.0f, -2.5f, u);
        s3ShipY = lerpf(  8.5f,  3.8f, ease);
        s3ShipZ = lerpf(-10.0f, -2.0f, u);

        s3ShipDirX =  0.60f;
        s3ShipDirY = -0.38f;
        s3ShipDirZ =  0.75f;
        normalize3(s3ShipDirX, s3ShipDirY, s3ShipDirZ);

        s3ShipPitch = lerpf(-18.0f, -12.0f, u);
        s3ShipRoll  = lerpf( 15.0f,   5.0f, u);
        s3ShipScale = 0.50f;
        s3HeatGlow  = lerpf( 1.0f,   0.35f, u); // Cooling down as speed drops

        // Trailing plasma wisps
        for (int k = 0; k < 3; ++k) {
            spawnParticle(s3ShipX, s3ShipY, s3ShipZ,
                          -s3ShipDirX * 8.0f, -s3ShipDirY * 8.0f, -s3ShipDirZ * 8.0f,
                          0.22f, 1.6f, 0.95f, 0.45f, 0.15f, true);
        }
    }
    // ------------------------------------------------------------------------
    // SHOT 3: WIDE CANYON REVEAL & MOUNTAIN PASS BANK (51.5s - 56.0s)
    // ------------------------------------------------------------------------
    else if (t < 56.0f) {
        float u = (t - 51.5f) / 4.5f;
        float smoothU = smoothstep(0.0f, 1.0f, u);

        s3ShipX = lerpf(-2.5f, 3.2f, smoothU);
        s3ShipY = lerpf( 3.8f, 1.2f, smoothU);
        s3ShipZ = lerpf(-2.0f, 4.5f, smoothU);

        s3ShipDirX =  0.50f;
        s3ShipDirY = -0.22f;
        s3ShipDirZ =  0.80f;
        normalize3(s3ShipDirX, s3ShipDirY, s3ShipDirZ);

        s3ShipPitch = lerpf(-12.0f, -4.0f, smoothU);
        s3ShipRoll  = lerpf(  5.0f, -6.0f, smoothU);
        s3ShipScale = 0.50f;
        s3HeatGlow  = 0.0f; // Heatshield fully cooled
    }
    // ------------------------------------------------------------------------
    // SHOT 4: SUPERSONIC PARACHUTE DEPLOYMENT (56.0s - 60.5s)
    // ------------------------------------------------------------------------
    else if (t < 60.5f) {
        float u = (t - 56.0f) / 4.5f;
        float smoothU = smoothstep(0.0f, 1.0f, u);

        s3ChuteDeploy = clampf((t - 56.0f) * 2.8f, 0.0f, 1.0f); // Unfurls in 0.35s

        // Rapid deceleration
        s3ShipX = lerpf( 3.2f, 5.0f, smoothU);
        s3ShipY = lerpf( 1.2f, -0.6f, smoothU);
        s3ShipZ = lerpf( 4.5f, 7.5f, smoothU);

        s3ShipDirX = lerpf(0.50f, 0.15f, smoothU);
        s3ShipDirY = lerpf(-0.22f, -0.10f, smoothU);
        s3ShipDirZ = lerpf(0.80f, 0.98f, smoothU);
        normalize3(s3ShipDirX, s3ShipDirY, s3ShipDirZ);

        s3ShipPitch = lerpf(-4.0f, 3.0f, smoothU);
        s3ShipRoll  = lerpf(-6.0f, 0.0f, smoothU);
        s3ShipScale = 0.50f;
        s3HeatGlow  = 0.0f;

        // Parachute canopy trails gracefully above craft
        s3ChuteX = s3ShipX - s3ShipDirX * 1.5f;
        s3ChuteY = s3ShipY + 2.5f + 0.08f * sinf(t * 7.0f);
        s3ChuteZ = s3ShipZ - s3ShipDirZ * 1.5f;
    }
    // ------------------------------------------------------------------------
    // SHOT 5: RETRO-THRUSTERS & TOUCHDOWN DUST KICK (60.5s - 66.0s)
    // ------------------------------------------------------------------------
    else {
        float u = (t - 60.5f) / 5.5f;
        u = clampf(u, 0.0f, 1.0f);
        float landEase = smoothstep(0.0f, 1.0f, u);

        s3ChuteCut = true;
        // Parachute drifts away on Martian breeze
        s3ChuteX += 1.6f * dt;
        s3ChuteY += 0.6f * dt;
        s3ChuteZ -= 2.0f * dt;

        // Lander settle down onto crater plateau (target landing Y = -2.10f)
        s3ShipX = lerpf( 5.0f,  5.6f, landEase);
        s3ShipY = lerpf(-0.6f, -2.10f, landEase);
        s3ShipZ = lerpf( 7.5f,  9.0f, landEase);

        s3ShipPitch = lerpf(3.0f, 0.0f, landEase);
        s3ShipRoll  = 0.0f;
        s3ShipDirX = 0.08f; s3ShipDirY = -0.04f; s3ShipDirZ = 0.99f;
        normalize3(s3ShipDirX, s3ShipDirY, s3ShipDirZ);
        s3HeatGlow = 0.0f;

        // Ventral retro thrusters pulsing down before touchdown (60.5s - 63.5s)
        if (t >= 60.5f && t <= 63.5f) {
            for (int k = 0; k < 2; ++k) {
                float ox = randRange(-0.3f, 0.3f);
                float oz = randRange(-0.3f, 0.3f);
                spawnParticle(s3ShipX + ox, s3ShipY - 0.2f, s3ShipZ + oz,
                              randRange(-0.2f, 0.2f), -5.0f, randRange(-0.2f, 0.2f),
                              0.15f, 1.0f, 0.2f, 0.85f, 1.0f, true);
            }
        }

        // Touchdown point contact at 63.2s: radial red dust plumes
        if (t >= 63.0f && t <= 65.5f) {
            for (int d = 0; d < 3; ++d) {
                float ang = randRange(0.0f, 2.0f * (float)M_PI);
                float spd = randRange(1.5f, 3.8f);
                TouchdownDust p;
                p.x = s3ShipX + cosf(ang) * 0.3f;
                p.y = -2.10f;
                p.z = s3ShipZ + sinf(ang) * 0.3f;
                p.vx = cosf(ang) * spd;
                p.vy = randRange(0.3f, 1.4f);
                p.vz = sinf(ang) * spd;
                p.scale = randRange(0.4f, 0.8f);
                p.alpha = 0.85f;
                p.maxLife = randRange(1.8f, 2.6f);
                p.life = p.maxLife;
                s3Dust.push_back(p);
            }
        }
    }

    // Update touchdown dust particles
    for (size_t i = 0; i < s3Dust.size(); ) {
        s3Dust[i].x += s3Dust[i].vx * dt;
        s3Dust[i].y += s3Dust[i].vy * dt;
        s3Dust[i].z += s3Dust[i].vz * dt;
        s3Dust[i].vx *= 0.94f;
        s3Dust[i].vz *= 0.94f;
        s3Dust[i].vy -= 0.5f * dt;
        s3Dust[i].scale += 0.7f * dt;
        s3Dust[i].life -= dt;
        s3Dust[i].alpha = (s3Dust[i].life / s3Dust[i].maxLife) * 0.75f;
        if (s3Dust[i].life <= 0.0f) {
            s3Dust[i] = s3Dust.back();
            s3Dust.pop_back();
        } else {
            ++i;
        }
    }
}

// ============================================================================
// CAMERA CHOREOGRAPHY
// ============================================================================
void computeCameraScene3(float& eyeX, float& eyeY, float& eyeZ,
                        float& targetX, float& targetY, float& targetZ,
                        float& upX, float& upY, float& upZ) {
    upX = 0.0f; upY = 1.0f; upZ = 0.0f;
    float t = seqTime;

    // ------------------------------------------------------------------------
    // SHOT 1: SIDE-PROFILE HERO SHOT (44.0s - 47.5s)
    // ------------------------------------------------------------------------
    // Camera rides alongside the Ranger's port flank:
    // Ranger prominently framed on the left, open Martian sky & clouds tearing past on the right.
    if (t < 47.5f) {
        float u = (t - 44.0f) / 3.5f;

        // Position camera to the side-front of the ship
        eyeX = s3ShipX + 1.85f;
        eyeY = s3ShipY + 0.35f;
        eyeZ = s3ShipZ - 0.70f;

        targetX = s3ShipX - 0.15f;
        targetY = s3ShipY;
        targetZ = s3ShipZ + 0.45f;

        // Violent hypersonic aerodynamic shudder & cabin vibration
        float freq = t * 62.0f;
        float amp  = (1.0f - u * 0.2f) * 0.024f;
        eyeX += sinf(freq * 1.05f) * amp;
        eyeY += cosf(freq * 0.95f) * amp;
        eyeZ += sinf(freq * 1.25f) * amp;
    }
    // ------------------------------------------------------------------------
    // SHOT 2: HYPERSONIC CHASE DIVE (47.5s - 51.5s)
    // ------------------------------------------------------------------------
    else if (t < 51.5f) {
        float u = (t - 47.5f) / 4.0f;
        eyeX = s3ShipX - s3ShipDirX * 3.4f + 1.1f;
        eyeY = s3ShipY - s3ShipDirY * 3.4f + 0.7f;
        eyeZ = s3ShipZ - s3ShipDirZ * 3.4f + 1.4f;

        targetX = s3ShipX + s3ShipDirX * 1.5f;
        targetY = s3ShipY + s3ShipDirY * 1.5f;
        targetZ = s3ShipZ + s3ShipDirZ * 1.5f;

        float shake = (1.0f - u * 0.5f) * 0.020f;
        eyeX += sinf(t * 42.0f) * shake;
        eyeY += cosf(t * 38.0f) * shake;
    }
    // ------------------------------------------------------------------------
    // SHOT 3: WIDE CANYON REVEAL (51.5s - 56.0s)
    // ------------------------------------------------------------------------
    else if (t < 56.0f) {
        float u = (t - 51.5f) / 4.5f;
        float smoothU = smoothstep(0.0f, 1.0f, u);

        eyeX = lerpf(-10.0f, -2.5f, smoothU);
        eyeY = lerpf(  7.5f,  4.5f, smoothU);
        eyeZ = lerpf( -2.0f,  6.5f, smoothU);

        targetX = s3ShipX;
        targetY = s3ShipY;
        targetZ = s3ShipZ;
    }
    // ------------------------------------------------------------------------
    // SHOT 4: LOW-ANGLE PARACHUTE TRACK (56.0s - 60.5s)
    // ------------------------------------------------------------------------
    else if (t < 60.5f) {
        float u = (t - 56.0f) / 4.5f;
        float smoothU = smoothstep(0.0f, 1.0f, u);

        eyeX = lerpf(s3ShipX + 2.8f, s3ShipX + 2.2f, smoothU);
        eyeY = lerpf(s3ShipY - 0.8f, s3ShipY - 0.5f, smoothU);
        eyeZ = lerpf(s3ShipZ - 3.2f, s3ShipZ - 2.8f, smoothU);

        targetX = s3ShipX;
        targetY = s3ShipY + 1.2f;
        targetZ = s3ShipZ;

        // Snapping jolt upon parachute opening at 56.1s - 56.6s
        if (t >= 56.1f && t <= 56.6f) {
            float snap = sinf((t - 56.1f) * 40.0f) * 0.028f;
            eyeY += snap;
        }
    }
    // ------------------------------------------------------------------------
    // SHOT 5: GROUND-LEVEL TOUCHDOWN (60.5s - 66.0s)
    // ------------------------------------------------------------------------
    else {
        float u = (t - 60.5f) / 5.5f;
        float smoothU = smoothstep(0.0f, 1.0f, u);

        eyeX = lerpf( 2.4f,  2.8f, smoothU);
        eyeY = -1.85f;
        eyeZ = lerpf( 5.5f,  6.0f, smoothU);

        targetX = lerpf( 5.0f,  5.6f, smoothU);
        targetY = -2.05f;
        targetZ = lerpf( 7.5f,  9.0f, smoothU);

        // Touchdown impact shudder at 63.0s - 63.6s
        if (t >= 63.0f && t <= 63.6f) {
            float impact = sinf((t - 63.0f) * 28.0f) * 0.020f * (1.0f - (t - 63.0f) / 0.6f);
            eyeY += impact;
        }
    }
}

// ============================================================================
// SCENE 3 RENDERING
// ============================================================================
static void renderAtmosphericSkybox() {
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glBegin(GL_QUADS);
        // Horizon: rich Martian salmon/butterscotch
        glColor3f(0.72f, 0.38f, 0.22f);
        glVertex2f(0.0f, 0.0f);
        glVertex2f(1.0f, 0.0f);
        // Zenith: deep cosmic rust/indigo
        glColor3f(0.20f, 0.08f, 0.12f);
        glVertex2f(1.0f, 1.0f);
        glVertex2f(0.0f, 1.0f);
    glEnd();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

static void renderAtmosphericVaporStreaks() {
    if (seqTime > 51.5f || s3Vapor.empty()) return;

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Radiant vapor additive glow
    glDepthMask(GL_FALSE);

    glLineWidth(2.4f);
    glBegin(GL_LINES);
    for (size_t i = 0; i < s3Vapor.size(); ++i) {
        const VaporStreak& v = s3Vapor[i];
        float x0 = v.x;
        float y0 = v.y;
        float z0 = v.z;
        float x1 = x0 + s3ShipDirX * v.length;
        float y1 = y0 + s3ShipDirY * v.length;
        float z1 = z0 + s3ShipDirZ * v.length;

        // Whitish-cyan high altitude condensation shreds
        glColor4f(0.92f, 0.88f, 0.82f, v.alpha * 0.70f);
        glVertex3f(x0, y0, z0);
        glColor4f(0.75f, 0.45f, 0.30f, 0.0f);
        glVertex3f(x1, y1, z1);
    }
    glEnd();

    glLineWidth(1.0f);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

static void renderHeatshieldGlow() {
    if (s3HeatGlow <= 0.01f) return;

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);

    // Multi-layered incandescent plasma halo wrapped around the nosecone & underside
    glPushMatrix();
    glTranslatef(0.0f, -0.05f, 0.45f); // Nosecone & forward underside

    for (int layer = 0; layer < 3; ++layer) {
        float s = 0.32f + (float)layer * 0.12f;
        float a = s3HeatGlow * (0.65f - (float)layer * 0.18f);

        // Core white-hot, fading into fiery orange outer envelope
        if (layer == 0) {
            glColor4f(1.0f, 0.95f, 0.75f, a);
        } else if (layer == 1) {
            glColor4f(1.0f, 0.55f, 0.15f, a);
        } else {
            glColor4f(0.90f, 0.25f, 0.05f, a);
        }

        glPushMatrix();
        glScalef(s * 1.8f, s * 0.6f, s * 2.2f);
        glutSolidSphere(1.0, 14, 10);
        glPopMatrix();
    }

    glPopMatrix();
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

static void renderTouchdownDustParticles() {
    if (s3Dust.empty() || !smokeTexID) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, smokeTexID);
    glDisable(GL_LIGHTING);
    glDepthMask(GL_FALSE);

    for (size_t i = 0; i < s3Dust.size(); ++i) {
        const TouchdownDust& p = s3Dust[i];
        glColor4f(0.72f, 0.32f, 0.18f, p.alpha);

        glPushMatrix();
        glTranslatef(p.x, p.y, p.z);
        glScalef(p.scale, p.scale, p.scale);
        glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f, 0.0f);
            glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f, 0.0f);
            glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f, 0.0f);
            glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f, 0.0f);
        glEnd();
        glPopMatrix();
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

static void renderParachuteWithLines() {
    if (!parachuteLoaded || parachuteDisplayList == 0 || s3ChuteDeploy <= 0.01f) return;

    glPushMatrix();
    glTranslatef(s3ChuteX, s3ChuteY, s3ChuteZ);
    float pScale = 0.40f * s3ChuteDeploy;
    glScalef(pScale, pScale, pScale);
    glCallList(parachuteDisplayList);
    glPopMatrix();

    // Shroud lines connecting parachute skirt to ship hardpoints (only before cut)
    if (!s3ChuteCut && s3ChuteDeploy > 0.5f) {
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glLineWidth(1.6f);
        glColor4f(0.92f, 0.90f, 0.88f, 0.75f);

        float canopyBottomY = s3ChuteY - 0.2f;
        glBegin(GL_LINES);
            glVertex3f(s3ChuteX - 0.7f, canopyBottomY, s3ChuteZ - 0.7f);
            glVertex3f(s3ShipX - 0.2f, s3ShipY + 0.2f, s3ShipZ - 0.2f);

            glVertex3f(s3ChuteX + 0.7f, canopyBottomY, s3ChuteZ - 0.7f);
            glVertex3f(s3ShipX + 0.2f, s3ShipY + 0.2f, s3ShipZ - 0.2f);

            glVertex3f(s3ChuteX + 0.7f, canopyBottomY, s3ChuteZ + 0.7f);
            glVertex3f(s3ShipX + 0.2f, s3ShipY + 0.2f, s3ShipZ + 0.2f);

            glVertex3f(s3ChuteX - 0.7f, canopyBottomY, s3ChuteZ + 0.7f);
            glVertex3f(s3ShipX - 0.2f, s3ShipY + 0.2f, s3ShipZ + 0.2f);
        glEnd();

        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
    }
}

void renderScene3() {
    // 1. Martian Atmospheric Gradient Sky
    renderAtmosphericSkybox();

    // 2. High-Altitude Atmospheric Cloud & Vapor Shreds
    renderAtmosphericVaporStreaks();

    // 3. Official NASA Gale Crater 3D Topography Mesh
    if (craterLoaded && craterDisplayList != 0) {
        glPushMatrix();
        glTranslatef(0.0f, -4.5f, 0.0f);

        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

        GLfloat matSpec[] = { 0.15f, 0.12f, 0.10f, 1.0f };
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpec);
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 8.0f);

        glCallList(craterDisplayList);
        glDisable(GL_COLOR_MATERIAL);
        glPopMatrix();
    }

    // 4. Parachute Canopy & Suspension Lines
    renderParachuteWithLines();

    // 5. Star Sparrow / Ranger Lander Craft with Heatshield Thermal Glow
    glPushMatrix();
    glTranslatef(s3ShipX, s3ShipY, s3ShipZ);
    glScalef(s3ShipScale, s3ShipScale, s3ShipScale);

    // Orientation
    float yaw = atan2f(s3ShipDirX, s3ShipDirZ) * 180.0f / (float)M_PI;
    glRotatef(yaw, 0.0f, 1.0f, 0.0f);
    glRotatef(s3ShipPitch, 1.0f, 0.0f, 0.0f);
    glRotatef(s3ShipRoll, 0.0f, 0.0f, 1.0f);

    // Key & Fill lighting for crisp ship illumination
    glEnable(GL_LIGHT2);
    GLfloat shipKeyPos[]  = { 0.0f, 4.0f, 2.0f, 0.0f };
    GLfloat shipKeyDiff[] = { 2.4f, 2.3f, 2.2f, 1.0f };
    GLfloat shipKeyAmb[]  = { 0.9f, 0.9f, 0.95f, 1.0f };
    glLightfv(GL_LIGHT2, GL_POSITION, shipKeyPos);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, shipKeyDiff);
    glLightfv(GL_LIGHT2, GL_AMBIENT, shipKeyAmb);

    renderRangerMesh();

    // Incandescent heatshield plasma envelope during entry
    renderHeatshieldGlow();

    glDisable(GL_LIGHT2);
    glPopMatrix();

    // 6. Volumetric Touchdown Dust Clouds
    renderTouchdownDustParticles();
}
