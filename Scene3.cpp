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
static float s3ShipRoll = 0.0f, s3ShipPitchExtra = 0.0f;
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
    float scale;
};
static std::vector<VaporStreak> s3Vapor;

// Volumetric Supersonic Plasma Fire Sheath Particles
struct PlasmaFlame {
    float x, y, z;
    float vx, vy, vz;
    float size;
    float growth;
    float life;
    float maxLife;
    float angle;
    float rotSpeed;
};
static std::vector<PlasmaFlame> s3PlasmaFlames;

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
        for (int i = 0; i < 35; ++i) {
            VaporStreak v;
            v.x = s3ShipX + randRange(-5.0f, 5.0f);
            v.y = s3ShipY + randRange(-3.0f, 4.0f);
            v.z = s3ShipZ + randRange(-8.0f, 12.0f);
            v.length = randRange(2.5f, 5.5f);
            v.speed  = randRange(22.0f, 36.0f);
            v.alpha  = randRange(0.18f, 0.40f);
            v.scale  = randRange(0.35f, 0.85f);
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

    // Update cloud vapor shreds streaming backward
    for (size_t i = 0; i < s3Vapor.size(); ++i) {
        s3Vapor[i].z -= s3Vapor[i].speed * dt;
        s3Vapor[i].x -= s3Vapor[i].speed * 0.40f * dt;
        s3Vapor[i].y += s3Vapor[i].speed * 0.25f * dt;
        if (s3Vapor[i].z < s3ShipZ - 12.0f) {
            s3Vapor[i].z = s3ShipZ + randRange(10.0f, 18.0f);
            s3Vapor[i].x = s3ShipX + randRange(-4.0f, 6.0f);
            s3Vapor[i].y = s3ShipY + randRange(-2.0f, 4.0f);
        }
    }

    // Update volumetric supersonic plasma fire billows
    for (size_t i = 0; i < s3PlasmaFlames.size(); ) {
        s3PlasmaFlames[i].x += s3PlasmaFlames[i].vx * dt;
        s3PlasmaFlames[i].y += s3PlasmaFlames[i].vy * dt;
        s3PlasmaFlames[i].z += s3PlasmaFlames[i].vz * dt;
        s3PlasmaFlames[i].size += s3PlasmaFlames[i].growth * dt;
        s3PlasmaFlames[i].angle += s3PlasmaFlames[i].rotSpeed * dt;
        s3PlasmaFlames[i].life -= dt;
        if (s3PlasmaFlames[i].life <= 0.0f) {
            s3PlasmaFlames[i] = s3PlasmaFlames.back();
            s3PlasmaFlames.pop_back();
        } else {
            ++i;
        }
    }

    // ------------------------------------------------------------------------
    // SHOT 1: SIDE-PROFILE ATMOSPHERIC PIERCE & HEATSHIELD TEAR (44.0s - 50.0s)
    // ------------------------------------------------------------------------
    if (t < 50.0f) {
        float u = (t - 44.0f) / 6.0f; // Extended 6.0s cinematic hero shot

        // Plunging steep downward descent into upper Martian atmosphere
        s3ShipX = lerpf(-18.0f, -9.0f, u);
        s3ShipY = lerpf( 16.5f,  7.5f, u * u);
        s3ShipZ = lerpf(-20.0f, -9.0f, u);

        // Vector plunging downward along the diagonal
        s3ShipDirX =  0.65f;
        s3ShipDirY = -0.55f; // Strong downward plunge vector
        s3ShipDirZ =  0.75f;
        normalize3(s3ShipDirX, s3ShipDirY, s3ShipDirZ);

        // Angle of attack: nose pitched DOWNWARD into the dive (+8.0 degrees)
        s3ShipPitchExtra = 8.0f;
        s3ShipRoll  = lerpf(24.0f, 10.0f, u);
        s3ShipScale = 0.50f;
        s3HeatGlow  = lerpf(0.50f, 1.0f, u); // High friction thermal glow

        // 3D Ship Coordinate Basis for authentic aerothermodynamic particle anchoring
        float dirX = s3ShipDirX, dirY = s3ShipDirY, dirZ = s3ShipDirZ;
        float wupX = 0.0f, wupY = 1.0f, wupZ = 0.0f;
        float rightX = dirY * wupZ - dirZ * wupY;
        float rightY = dirZ * wupX - dirX * wupZ;
        float rightZ = dirX * wupY - dirY * wupX;
        normalize3(rightX, rightY, rightZ);
        float upX = rightY * dirZ - rightZ * dirY;
        float upY = rightZ * dirX - rightX * dirZ;
        float upZ = rightX * dirY - rightY * dirX;

        // 1. Spallation embers and supersonic ablation sparks peeling into slipstream
        for (int k = 0; k < 22; ++k) {
            float lLateral = randRange(-0.40f, 0.40f);
            float lAlong   = randRange(-0.40f, 0.95f);
            float lHeight  = randRange(-0.10f, -0.02f); // Underbelly and leading edges

            float spawnX = s3ShipX + (rightX * lLateral + upX * lHeight + dirX * lAlong) * s3ShipScale;
            float spawnY = s3ShipY + (rightY * lLateral + upY * lHeight + dirY * lAlong) * s3ShipScale;
            float spawnZ = s3ShipZ + (rightZ * lLateral + upZ * lHeight + dirZ * lAlong) * s3ShipScale;

            float speed = randRange(20.0f, 38.0f);
            spawnParticle(spawnX, spawnY, spawnZ,
                          -dirX * speed + rightX * randRange(-1.2f, 1.2f) + upX * randRange(-0.8f, 0.8f),
                          -dirY * speed + rightY * randRange(-1.2f, 1.2f) + upY * randRange(-0.8f, 0.8f),
                          -dirZ * speed + rightZ * randRange(-1.2f, 1.2f) + upZ * randRange(-0.8f, 0.8f),
                          randRange(0.06f, 0.16f), randRange(1.8f, 3.2f),
                          1.0f, randRange(0.85f, 1.0f), randRange(0.30f, 0.60f), true);
        }

        // 2. Volumetric Supersonic Plasma Fire Tail (sleek, multi-layered trailing wake)
        for (int f = 0; f < 14; ++f) {
            float lLateral = randRange(-0.55f, 0.55f);
            float lAlong   = randRange(0.10f, 0.98f); // Originates at nose apex & along chines
            float lHeight  = -0.06f + randRange(-0.04f, 0.02f);

            float spawnX = s3ShipX + (rightX * lLateral + upX * lHeight + dirX * lAlong) * s3ShipScale;
            float spawnY = s3ShipY + (rightY * lLateral + upY * lHeight + dirY * lAlong) * s3ShipScale;
            float spawnZ = s3ShipZ + (rightZ * lLateral + upZ * lHeight + dirZ * lAlong) * s3ShipScale;

            float fireSpd = randRange(22.0f, 38.0f);
            PlasmaFlame flame;
            flame.x = spawnX;
            flame.y = spawnY;
            flame.z = spawnZ;
            flame.vx = -dirX * fireSpd + rightX * randRange(-0.6f, 0.6f) - upX * randRange(0.2f, 0.8f);
            flame.vy = -dirY * fireSpd + rightY * randRange(-0.6f, 0.6f) - upY * randRange(0.2f, 0.8f);
            flame.vz = -dirZ * fireSpd + rightZ * randRange(-0.6f, 0.6f) - upZ * randRange(0.2f, 0.8f);
            flame.size = randRange(0.10f, 0.20f);
            flame.growth = randRange(1.8f, 3.2f); // Smooth aerodynamic expansion into trailing plasma plume
            flame.maxLife = randRange(0.18f, 0.32f);
            flame.life = flame.maxLife;
            flame.angle = randRange(0.0f, 6.28f);
            flame.rotSpeed = randRange(-4.0f, 4.0f);
            s3PlasmaFlames.push_back(flame);
        }
    }
    // ------------------------------------------------------------------------
    // SHOT 2: HYPERSONIC OVER-THE-SHOULDER DIVE (50.0s - 54.0s)
    // ------------------------------------------------------------------------
    else if (t < 54.0f) {
        float u = (t - 50.0f) / 4.0f;
        float ease = u * u;

        s3ShipX = lerpf(-9.0f, -2.0f, u);
        s3ShipY = lerpf( 7.5f,  3.2f, ease);
        s3ShipZ = lerpf(-9.0f, -1.5f, u);

        s3ShipDirX =  0.60f;
        s3ShipDirY = -0.42f;
        s3ShipDirZ =  0.75f;
        normalize3(s3ShipDirX, s3ShipDirY, s3ShipDirZ);

        s3ShipPitchExtra = lerpf(8.0f, 4.0f, u);
        s3ShipRoll  = lerpf(10.0f, 4.0f, u);
        s3ShipScale = 0.50f;
        s3HeatGlow  = lerpf(1.0f, 0.25f, u);

        // Residual plasma billows fading out
        for (int f = 0; f < 4; ++f) {
            float fireSpd = randRange(12.0f, 20.0f);
            PlasmaFlame flame;
            flame.x = s3ShipX + randRange(-0.3f, 0.3f);
            flame.y = s3ShipY - 0.08f;
            flame.z = s3ShipZ + randRange(-0.2f, 0.2f);
            flame.vx = -s3ShipDirX * fireSpd;
            flame.vy = -s3ShipDirY * fireSpd;
            flame.vz = -s3ShipDirZ * fireSpd;
            flame.size = randRange(0.25f, 0.45f);
            flame.growth = 4.0f;
            flame.maxLife = randRange(0.18f, 0.30f);
            flame.life = flame.maxLife;
            flame.angle = randRange(0.0f, 6.28f);
            flame.rotSpeed = randRange(-3.0f, 3.0f);
            s3PlasmaFlames.push_back(flame);
        }
    }
    // ------------------------------------------------------------------------
    // SHOT 3: WIDE CANYON REVEAL & MOUNTAIN PASS BANK (54.0s - 58.5s)
    // ------------------------------------------------------------------------
    else if (t < 58.5f) {
        float u = (t - 54.0f) / 4.5f;
        float smoothU = smoothstep(0.0f, 1.0f, u);

        s3ShipX = lerpf(-2.0f, 3.2f, smoothU);
        s3ShipY = lerpf( 3.2f, 1.2f, smoothU);
        s3ShipZ = lerpf(-1.5f, 4.5f, smoothU);

        s3ShipDirX =  0.50f;
        s3ShipDirY = -0.25f;
        s3ShipDirZ =  0.80f;
        normalize3(s3ShipDirX, s3ShipDirY, s3ShipDirZ);

        s3ShipPitchExtra = lerpf(4.0f, 0.0f, smoothU);
        s3ShipRoll  = lerpf(4.0f, -6.0f, smoothU);
        s3ShipScale = 0.50f;
        s3HeatGlow  = 0.0f; // Cooled
    }
    // ------------------------------------------------------------------------
    // SHOT 4: SUPERSONIC PARACHUTE DEPLOYMENT (58.5s - 63.0s)
    // ------------------------------------------------------------------------
    else if (t < 63.0f) {
        float u = (t - 58.5f) / 4.5f;
        float smoothU = smoothstep(0.0f, 1.0f, u);

        s3ChuteDeploy = clampf((t - 58.5f) * 2.8f, 0.0f, 1.0f);

        s3ShipX = lerpf( 3.2f, 5.0f, smoothU);
        s3ShipY = lerpf( 1.2f, -0.6f, smoothU);
        s3ShipZ = lerpf( 4.5f, 7.5f, smoothU);

        s3ShipDirX = lerpf(0.50f, 0.15f, smoothU);
        s3ShipDirY = lerpf(-0.25f, -0.10f, smoothU);
        s3ShipDirZ = lerpf(0.80f, 0.98f, smoothU);
        normalize3(s3ShipDirX, s3ShipDirY, s3ShipDirZ);

        s3ShipPitchExtra = lerpf(0.0f, 2.0f, smoothU);
        s3ShipRoll  = lerpf(-6.0f, 0.0f, smoothU);
        s3ShipScale = 0.50f;
        s3HeatGlow  = 0.0f;

        s3ChuteX = s3ShipX - s3ShipDirX * 1.5f;
        s3ChuteY = s3ShipY + 2.5f + 0.08f * sinf(t * 7.0f);
        s3ChuteZ = s3ShipZ - s3ShipDirZ * 1.5f;
    }
    // ------------------------------------------------------------------------
    // SHOT 5: RETRO-THRUSTERS & TOUCHDOWN DUST KICK (63.0s - 69.0s)
    // ------------------------------------------------------------------------
    else {
        float u = (t - 63.0f) / 5.5f;
        u = clampf(u, 0.0f, 1.0f);
        float landEase = smoothstep(0.0f, 1.0f, u);

        s3ChuteCut = true;
        s3ChuteX += 1.6f * dt;
        s3ChuteY += 0.6f * dt;
        s3ChuteZ -= 2.0f * dt;

        s3ShipX = lerpf( 5.0f,  5.6f, landEase);
        s3ShipY = lerpf(-0.6f, -2.10f, landEase);
        s3ShipZ = lerpf( 7.5f,  9.0f, landEase);

        s3ShipPitchExtra = 0.0f;
        s3ShipRoll  = 0.0f;
        s3ShipDirX = 0.08f; s3ShipDirY = -0.04f; s3ShipDirZ = 0.99f;
        normalize3(s3ShipDirX, s3ShipDirY, s3ShipDirZ);
        s3HeatGlow = 0.0f;

        if (t >= 63.0f && t <= 66.0f) {
            for (int k = 0; k < 2; ++k) {
                float ox = randRange(-0.3f, 0.3f);
                float oz = randRange(-0.3f, 0.3f);
                spawnParticle(s3ShipX + ox, s3ShipY - 0.2f, s3ShipZ + oz,
                              randRange(-0.2f, 0.2f), -5.0f, randRange(-0.2f, 0.2f),
                              0.15f, 1.0f, 0.2f, 0.85f, 1.0f, true);
            }
        }

        if (t >= 65.5f && t <= 68.5f) {
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
    // SHOT 1: SIDE-PROFILE HERO SHOT (44.0s - 50.0s)
    // ------------------------------------------------------------------------
    // Cinematic side flank: Ranger plunges downward from upper-right toward lower-left.
    // Framed with open Martian salmon sky and vapor shreds tearing beside it.
    if (t < 50.0f) {
        float u = (t - 44.0f) / 6.0f; // Extended 6.0s hero view

        // Position camera to the side-above the diving ship
        eyeX = s3ShipX + 2.4f;
        eyeY = s3ShipY + 0.65f;
        eyeZ = s3ShipZ - 0.40f;

        targetX = s3ShipX - 0.35f;
        targetY = s3ShipY - 0.60f; // Look downward along the plunge trajectory
        targetZ = s3ShipZ + 0.55f;

        // Aerodynamic buffeting shudder
        float freq = t * 62.0f;
        float amp  = (1.0f - u * 0.2f) * 0.022f;
        eyeX += sinf(freq * 1.05f) * amp;
        eyeY += cosf(freq * 0.95f) * amp;
        eyeZ += sinf(freq * 1.25f) * amp;
    }
    // ------------------------------------------------------------------------
    // SHOT 2: HYPERSONIC CHASE DIVE (50.0s - 54.0s)
    // ------------------------------------------------------------------------
    else if (t < 54.0f) {
        float u = (t - 50.0f) / 4.0f;
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
    // SHOT 3: WIDE CANYON REVEAL (54.0s - 58.5s)
    // ------------------------------------------------------------------------
    else if (t < 58.5f) {
        float u = (t - 54.0f) / 4.5f;
        float smoothU = smoothstep(0.0f, 1.0f, u);

        eyeX = lerpf(-10.0f, -2.5f, smoothU);
        eyeY = lerpf(  7.5f,  4.5f, smoothU);
        eyeZ = lerpf( -2.0f,  6.5f, smoothU);

        targetX = s3ShipX;
        targetY = s3ShipY;
        targetZ = s3ShipZ;
    }
    // ------------------------------------------------------------------------
    // SHOT 4: LOW-ANGLE PARACHUTE TRACK (58.5s - 63.0s)
    // ------------------------------------------------------------------------
    else if (t < 63.0f) {
        float u = (t - 58.5f) / 4.5f;
        float smoothU = smoothstep(0.0f, 1.0f, u);

        eyeX = lerpf(s3ShipX + 2.8f, s3ShipX + 2.2f, smoothU);
        eyeY = lerpf(s3ShipY - 0.8f, s3ShipY - 0.5f, smoothU);
        eyeZ = lerpf(s3ShipZ - 3.2f, s3ShipZ - 2.8f, smoothU);

        targetX = s3ShipX;
        targetY = s3ShipY + 1.2f;
        targetZ = s3ShipZ;

        if (t >= 58.6f && t <= 59.1f) {
            float snap = sinf((t - 58.6f) * 40.0f) * 0.028f;
            eyeY += snap;
        }
    }
    // ------------------------------------------------------------------------
    // SHOT 5: GROUND-LEVEL TOUCHDOWN (63.0s - 69.0s)
    // ------------------------------------------------------------------------
    else {
        float u = (t - 63.0f) / 5.5f;
        float smoothU = smoothstep(0.0f, 1.0f, u);

        eyeX = lerpf( 2.4f,  2.8f, smoothU);
        eyeY = -1.85f;
        eyeZ = lerpf( 5.5f,  6.0f, smoothU);

        targetX = lerpf( 5.0f,  5.6f, smoothU);
        targetY = -2.05f;
        targetZ = lerpf( 7.5f,  9.0f, smoothU);

        if (t >= 65.5f && t <= 66.1f) {
            float impact = sinf((t - 65.5f) * 28.0f) * 0.020f * (1.0f - (t - 65.5f) / 0.6f);
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
    if (seqTime > 51.5f || s3Vapor.empty() || !smokeTexID) return;

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive luminous vapor
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, smokeTexID);
    glDepthMask(GL_FALSE);

    // Billowing high-speed cloud vapor shreds tearing diagonally past
    for (size_t i = 0; i < s3Vapor.size(); ++i) {
        const VaporStreak& v = s3Vapor[i];
        glColor4f(0.92f, 0.82f, 0.75f, v.alpha * 0.65f);

        glPushMatrix();
        glTranslatef(v.x, v.y, v.z);
        glScalef(v.scale * 1.8f, v.scale * 0.45f, v.length);
        glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, 0.0f, -1.0f);
            glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, 0.0f, -1.0f);
            glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f, 0.0f,  1.0f);
            glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f, 0.0f,  1.0f);
        glEnd();
        glPopMatrix();
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

static void renderHeatshieldGlow() {
    if (s3HeatGlow <= 0.01f) return;

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Pure radiant additive incandescent plasma
    glDepthMask(GL_FALSE);

    float t = seqTime;
    float glow = s3HeatGlow;
    // Aerodynamic turbulent thermal flicker (high-frequency plasma combustion instability)
    float flicker = glow * (0.92f + 0.08f * sinf(t * 73.0f) * cosf(t * 49.0f));

    // ========================================================================
    // 1. CERAMIC TILE THERMAL PROTECTION SYSTEM (TPS) UNDERBELLY INCANDESCENCE
    // ========================================================================
    // Direct emission from the white-hot and radiant cherry-red ceramic tiles
    if (smokeTexID) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, smokeTexID);
    }

    glBegin(GL_QUADS);
        // Forward nose tiles (stagnation heating: 2200K blinding white-yellow)
        glColor4f(1.0f, 0.98f, 0.88f, flicker * 0.90f);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.18f, -0.075f, 0.95f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f( 0.18f, -0.075f, 0.95f);
        glColor4f(1.0f, 0.82f, 0.35f, flicker * 0.80f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f( 0.34f, -0.085f, 0.45f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.34f, -0.085f, 0.45f);

        // Mid-fuselage belly tiles (radiant solar orange: 1800K)
        glColor4f(1.0f, 0.82f, 0.35f, flicker * 0.80f);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.34f, -0.085f, 0.45f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f( 0.34f, -0.085f, 0.45f);
        glColor4f(1.0f, 0.48f, 0.10f, flicker * 0.65f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f( 0.44f, -0.095f, -0.20f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.44f, -0.095f, -0.20f);

        // Aft belly & engine heatshield base (cooling cherry-red: 1300K)
        glColor4f(1.0f, 0.48f, 0.10f, flicker * 0.65f);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.44f, -0.095f, -0.20f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f( 0.44f, -0.095f, -0.20f);
        glColor4f(0.75f, 0.18f, 0.03f, flicker * 0.38f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f( 0.48f, -0.098f, -0.95f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.48f, -0.098f, -0.95f);

        // Port & Starboard wing underbelly heat wash
        glColor4f(1.0f, 0.65f, 0.18f, flicker * 0.55f);
        glTexCoord2f(0.0f, 0.0f); glVertex3f( 0.34f, -0.085f, 0.45f);
        glColor4f(0.90f, 0.30f, 0.05f, flicker * 0.35f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f( 0.82f, -0.080f, -0.40f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f( 0.44f, -0.095f, -0.20f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f( 0.34f, -0.085f, 0.10f);

        glColor4f(1.0f, 0.65f, 0.18f, flicker * 0.55f);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.34f, -0.085f, 0.45f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.34f, -0.085f, 0.10f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-0.44f, -0.095f, -0.20f);
        glColor4f(0.90f, 0.30f, 0.05f, flicker * 0.35f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-0.82f, -0.080f, -0.40f);
    glEnd();

    // ========================================================================
    // 2. FLOWING TEXTURED AERODYNAMIC BOW-SHOCK VEIL (Supersonic Wave Front)
    // ========================================================================
    // A detached 3D hyperbolic shock envelope with hypersonic fluid texture streaming
    // violently backward over the vehicle profile at Mach 25.
    const int SLICES = 32;
    const int RINGS  = 20;

    for (int pass = 0; pass < 2; ++pass) {
        float passScale = (pass == 0) ? 1.0f : 0.85f; // Outer bow shock wave, then tight inner sheath
        float zApex     = (pass == 0) ? 1.06f : 1.02f;
        float vFlowSpd  = (pass == 0) ? 24.0f : 36.0f;

        for (int r = 0; r < RINGS; ++r) {
            float frac0 = (float)r / (float)RINGS;
            float frac1 = (float)(r + 1) / (float)RINGS;

            float z0 = zApex - 2.40f * powf(frac0, 1.30f);
            float z1 = zApex - 2.40f * powf(frac1, 1.30f);

            float dist0 = zApex - z0;
            float dist1 = zApex - z1;

            // Lifting body aerodynamic profile: tight elliptical cross section
            float w0 = (0.24f + 0.46f * powf(dist0, 0.65f)) * passScale;
            float h0 = (0.10f + 0.18f * powf(dist0, 0.65f)) * passScale;
            float y0 = -0.05f - 0.035f * dist0;

            float w1 = (0.24f + 0.46f * powf(dist1, 0.65f)) * passScale;
            float h1 = (0.10f + 0.18f * powf(dist1, 0.65f)) * passScale;
            float y1 = -0.05f - 0.035f * dist1;

            auto getPlasmaColor = [&](float f, float& cr, float& cg, float& cb, float& ca) {
                if (f < 0.10f) {
                    // Stagnation apex: Pure nuclear white-hot
                    float s = f / 0.10f;
                    cr = 1.0f;
                    cg = lerpf(1.0f, 0.94f, s);
                    cb = lerpf(0.98f, 0.60f, s);
                    ca = flicker * lerpf(0.85f, 0.60f, s);
                } else if (f < 0.45f) {
                    // Primary compression zone: incandescent solar gold & radiant amber
                    float s = (f - 0.10f) / 0.35f;
                    cr = 1.0f;
                    cg = lerpf(0.94f, 0.45f, s);
                    cb = lerpf(0.60f, 0.05f, s);
                    ca = flicker * lerpf(0.60f, 0.35f, s);
                } else if (f < 0.80f) {
                    // Shock expansion flanks: intense plasma vermilion
                    float s = (f - 0.45f) / 0.35f;
                    cr = 1.0f;
                    cg = lerpf(0.45f, 0.15f, s);
                    cb = 0.02f;
                    ca = flicker * lerpf(0.35f, 0.12f, s);
                } else {
                    // Dissipating wake envelope: deep fiery crimson fading cleanly to 0
                    float s = (f - 0.80f) / 0.20f;
                    cr = lerpf(1.0f, 0.55f, s);
                    cg = lerpf(0.15f, 0.02f, s);
                    cb = 0.01f;
                    ca = flicker * (1.0f - s) * 0.12f;
                }
                if (pass == 1) {
                    ca *= 1.2f;
                }
            };

            float cr0, cg0, cb0, ca0;
            float cr1, cg1, cb1, ca1;
            getPlasmaColor(frac0, cr0, cg0, cb0, ca0);
            getPlasmaColor(frac1, cr1, cg1, cb1, ca1);

            float v0 = frac0 * 3.0f - t * vFlowSpd;
            float v1 = frac1 * 3.0f - t * vFlowSpd;

            glBegin(GL_TRIANGLE_STRIP);
            for (int s = 0; s <= SLICES; ++s) {
                float phi = (float)s / (float)SLICES * 2.0f * (float)M_PI;
                float cosP = cosf(phi);
                float sinP = sinf(phi);
                float uCoord = (float)s / (float)SLICES;

                // High-frequency boundary flutter
                float flutter0 = sinf(t * 72.0f + phi * 4.0f + frac0 * 18.0f) * 0.012f;
                float flutter1 = sinf(t * 72.0f + phi * 4.0f + frac1 * 18.0f) * 0.012f;

                glColor4f(cr0, cg0, cb0, ca0);
                glTexCoord2f(uCoord, v0);
                glVertex3f(cosP * (w0 + flutter0), y0 + sinP * (h0 + flutter0), z0);

                glColor4f(cr1, cg1, cb1, ca1);
                glTexCoord2f(uCoord, v1);
                glVertex3f(cosP * (w1 + flutter1), y1 + sinP * (h1 + flutter1), z1);
            }
            glEnd();
        }
    }

    // ========================================================================
    // 3. STAGNATION POINT NOSE BLOOM & ANAMORPHIC GLARE
    // ========================================================================
    // Concentrated radiant bloom right at the blunt nose cap
    glPushMatrix();
    glTranslatef(0.0f, -0.05f, 1.03f);

    // Core White Glint (tight to the nose)
    glColor4f(1.0f, 1.0f, 0.98f, flicker * 0.88f);
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.16f, -0.10f, 0.0f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f( 0.16f, -0.10f, 0.0f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f( 0.16f,  0.10f, 0.0f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.16f,  0.10f, 0.0f);
    glEnd();

    // Radiant Gold Shock Corona
    glColor4f(1.0f, 0.78f, 0.25f, flicker * 0.45f);
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.35f, -0.22f, -0.02f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f( 0.35f, -0.22f, -0.02f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f( 0.35f,  0.22f, -0.02f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.35f,  0.22f, -0.02f);
    glEnd();

    // Subtle Anamorphic Optical Glare Streak (proportional to nose width)
    glBegin(GL_QUADS);
        glColor4f(1.0f, 0.95f, 0.80f, flicker * 0.65f);
        glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.65f, -0.020f, 0.01f);
        glTexCoord2f(1.0f, 0.0f); glVertex3f( 0.65f, -0.020f, 0.01f);
        glTexCoord2f(1.0f, 1.0f); glVertex3f( 0.65f,  0.020f, 0.01f);
        glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.65f,  0.020f, 0.01f);
    glEnd();

    glPopMatrix();

    // ========================================================================
    // 4. CHINE & WING-EDGE SUPERSONIC IONIZATION RIBBONS
    // ========================================================================
    // Textured camera-facing quad ribbons peeling off delta chines and wingtips
    for (int side = -1; side <= 1; side += 2) {
        float sx = (float)side * 0.38f;
        for (int ribbon = 0; ribbon < 6; ++ribbon) {
            float frac = (float)ribbon / 5.0f;
            float zStart = 0.75f - frac * 0.85f;
            float zEnd   = zStart - 2.10f;
            float lineY  = -0.065f - frac * 0.025f;
            float wave0  = sinf(t * 52.0f + frac * 3.5f) * 0.022f;
            float wave1  = sinf(t * 52.0f + frac * 3.5f + 2.5f) * 0.045f;
            float lateralStart = sx + (float)side * frac * 0.38f;
            float lateralEnd   = lateralStart * 1.55f;

            float vRibbon = frac * 2.0f - t * 26.0f;

            glBegin(GL_QUADS);
                glColor4f(1.0f, 0.95f, 0.65f, flicker * 0.85f);
                glTexCoord2f(0.0f, vRibbon);
                glVertex3f(lateralStart + wave0, lineY - 0.025f, zStart);

                glColor4f(1.0f, 0.95f, 0.65f, flicker * 0.85f);
                glTexCoord2f(1.0f, vRibbon);
                glVertex3f(lateralStart + wave0, lineY + 0.025f, zStart);

                glColor4f(0.85f, 0.20f, 0.02f, 0.0f);
                glTexCoord2f(1.0f, vRibbon + 2.5f);
                glVertex3f(lateralEnd + wave1, lineY - 0.06f + 0.035f, zEnd);

                glColor4f(0.85f, 0.20f, 0.02f, 0.0f);
                glTexCoord2f(0.0f, vRibbon + 2.5f);
                glVertex3f(lateralEnd + wave1, lineY - 0.06f - 0.035f, zEnd);
            glEnd();
        }
    }

    if (smokeTexID) {
        glDisable(GL_TEXTURE_2D);
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

static void renderVolumetricPlasmaFlames() {
    if (s3PlasmaFlames.empty() || !smokeTexID) return;

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Pure radiant additive flame
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, smokeTexID);
    glDepthMask(GL_FALSE);

    // Extract billboard right and up from ModelView matrix
    GLfloat m[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, m);
    float rx = m[0], ry = m[4], rz = m[8]; // Camera Right
    float ux = m[1], uy = m[5], uz = m[9]; // Camera Up

    for (size_t i = 0; i < s3PlasmaFlames.size(); ++i) {
        const PlasmaFlame& p = s3PlasmaFlames[i];
        float normLife = p.life / p.maxLife; // 1.0 (newborn) -> 0.0 (death)

        float r, g, b, a;
        if (normLife > 0.85f) {
            // White-hot stagnation flame core
            float sub = (normLife - 0.85f) / 0.15f;
            r = 1.0f;
            g = lerpf(0.88f, 0.96f, sub);
            b = lerpf(0.35f, 0.80f, sub);
            a = lerpf(0.40f, 0.58f, sub);
        } else if (normLife > 0.35f) {
            // Incandescent solar plasma gold & deep atmospheric orange
            float sub = (normLife - 0.35f) / 0.50f;
            r = 1.0f;
            g = lerpf(0.38f, 0.88f, sub);
            b = lerpf(0.03f, 0.35f, sub);
            a = lerpf(0.24f, 0.40f, sub);
        } else {
            // Cooling trailing crimson wake
            float sub = normLife / 0.35f;
            r = lerpf(0.75f, 1.0f, sub);
            g = lerpf(0.10f, 0.38f, sub);
            b = 0.02f;
            a = sub * 0.24f;
        }

        glColor4f(r, g, b, a * s3HeatGlow);

        float s = p.size;
        float cosA = cosf(p.angle) * s;
        float sinA = sinf(p.angle) * s;

        float dx0 =  rx * cosA - ux * sinA;
        float dy0 =  ry * cosA - uy * sinA;
        float dz0 =  rz * cosA - uz * sinA;

        float dx1 =  rx * sinA + ux * cosA;
        float dy1 =  ry * sinA + uy * cosA;
        float dz1 =  rz * sinA + uz * cosA;

        glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex3f(p.x - dx0 - dx1, p.y - dy0 - dy1, p.z - dz0 - dz1);
            glTexCoord2f(1.0f, 0.0f); glVertex3f(p.x + dx0 - dx1, p.y + dy0 - dy1, p.z + dz0 - dz1);
            glTexCoord2f(1.0f, 1.0f); glVertex3f(p.x + dx0 + dx1, p.y + dy0 + dy1, p.z + dz0 + dz1);
            glTexCoord2f(0.0f, 1.0f); glVertex3f(p.x - dx0 + dx1, p.y - dy0 + dy1, p.z - dz0 + dz1);
        glEnd();
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_TEXTURE_2D);
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

    // 3. Official NASA Gale Crater 3D Topography Mesh (revealed in Shot 2 with over-the-shoulder cut at 50.0s)
    if (seqTime >= 50.0f && craterLoaded && craterDisplayList != 0) {
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

    // Orientation: Compute true descent plunge pitch
    float yaw = atan2f(s3ShipDirX, s3ShipDirZ) * 180.0f / (float)M_PI;
    float horizLen = sqrtf(s3ShipDirX * s3ShipDirX + s3ShipDirZ * s3ShipDirZ);
    float flightPitch = -atan2f(s3ShipDirY, horizLen) * 180.0f / (float)M_PI;

    glRotatef(yaw, 0.0f, 1.0f, 0.0f);
    // Positive pitch angles the nose DOWNWARD into the descent
    glRotatef(flightPitch + s3ShipPitchExtra, 1.0f, 0.0f, 0.0f);
    glRotatef(s3ShipRoll, 0.0f, 0.0f, 1.0f);

    // Key & Fill lighting for crisp ship illumination
    glEnable(GL_LIGHT2);
    GLfloat shipKeyPos[]  = { 0.0f, 4.0f, 2.0f, 0.0f };
    GLfloat shipKeyDiff[] = { 2.4f, 2.3f, 2.2f, 1.0f };
    GLfloat shipKeyAmb[]  = { 0.9f, 0.9f, 0.95f, 1.0f };
    glLightfv(GL_LIGHT2, GL_POSITION, shipKeyPos);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, shipKeyDiff);
    glLightfv(GL_LIGHT2, GL_AMBIENT, shipKeyAmb);

    // Dynamic underbelly heatshield incandescence illumination
    if (s3HeatGlow > 0.05f) {
        glEnable(GL_LIGHT3);
        GLfloat heatPos[]  = { 0.0f, -1.2f, 0.3f, 1.0f }; // Directly below the forward heatshield tiles
        GLfloat heatDiff[] = { 3.8f * s3HeatGlow, 1.8f * s3HeatGlow, 0.25f * s3HeatGlow, 1.0f };
        GLfloat heatAmb[]  = { 0.50f * s3HeatGlow, 0.20f * s3HeatGlow, 0.02f * s3HeatGlow, 1.0f };
        glLightfv(GL_LIGHT3, GL_POSITION, heatPos);
        glLightfv(GL_LIGHT3, GL_DIFFUSE, heatDiff);
        glLightfv(GL_LIGHT3, GL_AMBIENT, heatAmb);
    } else {
        glDisable(GL_LIGHT3);
    }

    renderRangerMesh();

    // Aerodynamic bow-shock & underbelly thermal glow
    renderHeatshieldGlow();

    glDisable(GL_LIGHT2);
    glDisable(GL_LIGHT3);
    glPopMatrix();

    // 6. Volumetric Supersonic Plasma Fire Sheath & Trail (in world space)
    renderVolumetricPlasmaFlames();

    // 7. Volumetric Touchdown Dust Clouds
    renderTouchdownDustParticles();
}
