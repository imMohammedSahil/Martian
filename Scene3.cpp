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
static GLuint horizonDiskDisplayList = 0;
static GLuint craterTexID = 0;
static bool   craterLoaded = false;
static bool   craterLoadFailed = false;

static GLuint cloudTexID = 0;
static GLuint skyTexID = 0;
static GLuint skyDomeDisplayList = 0;

// Procedural 3D Volumetric Cloud Deck
struct VolumetricCloud {
    float x, y, z;
    float speed;
    float scale;
    float rot;
    float rotSpeed;
    float alpha;
    float tint;
};
static std::vector<VolumetricCloud> s3VolumetricClouds;

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

// Supersonic Reentry Ablation Sparks (Horizontal streaks matching reference)
struct ReentrySpark {
    float x, y, z;
    float vx, vy, vz;
    float len;
    float life;
    float maxLife;
    float r, g, b;
    float size;
};
static std::vector<ReentrySpark> s3Sparks;

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

    if (!craterTexID) {
        craterTexID = loadTextureHelper("mars_surface_4k.jpg", true);
        if (!craterTexID) craterTexID = loadTextureHelper("mars_surface.jpg", true);
    }

    craterDisplayList = glGenLists(1);
    glNewList(craterDisplayList, GL_COMPILE);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glVertexPointer(3, GL_FLOAT, sizeof(TerrainVertex), &verts[0].x);
    glNormalPointer(GL_FLOAT, sizeof(TerrainVertex), &verts[0].nx);
    glColorPointer(3, GL_FLOAT, sizeof(TerrainVertex), &verts[0].r);
    glTexCoordPointer(2, GL_FLOAT, sizeof(TerrainVertex), &verts[0].u);

    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)numVerts);

    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
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
    if (!skyTexID) skyTexID = loadTextureHelper("mars_sky_accurate.jpg", false);
    if (!smokeTexID) smokeTexID = loadTextureHelper("smoke1.png", false);
    if (!sparkTexID) sparkTexID = loadTextureHelper("spark1.png", false);
    if (!cloudTexID) cloudTexID = loadTextureHelper("cloud_volume.png", false);
    if (!cloudTexID) cloudTexID = loadTextureHelper("cloud.png", false);
    if (!cloudTexID) cloudTexID = smokeTexID;

    if (s3Vapor.empty()) {
        for (int i = 0; i < 180; ++i) {
            VaporStreak v;
            v.x = s3ShipX + randRange(-18.0f, 18.0f);
            v.y = s3ShipY + randRange(-10.0f, 12.0f);
            v.z = s3ShipZ + randRange(-22.0f, 25.0f);
            v.length = randRange(6.0f, 16.0f);
            v.speed  = randRange(24.0f, 48.0f);
            v.alpha  = randRange(0.25f, 0.65f);
            v.scale  = randRange(1.2f, 3.8f);
            s3Vapor.push_back(v);
        }
    }

    if (s3VolumetricClouds.empty()) {
        for (int i = 0; i < 360; ++i) {
            VolumetricCloud c;
            c.x = randRange(-22.0f, 12.0f);
            c.y = randRange(3.0f, 13.0f);
            c.z = randRange(-22.0f, 12.0f);
            c.speed = randRange(24.0f, 44.0f);
            c.scale = randRange(3.2f, 8.5f);
            c.rot = randRange(0.0f, 6.28f);
            c.rotSpeed = randRange(-0.25f, 0.25f);
            c.alpha = randRange(0.45f, 0.90f);
            c.tint = randRange(0.0f, 1.0f);
            s3VolumetricClouds.push_back(c);
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
        if (s3Vapor[i].z < s3ShipZ - 20.0f) {
            s3Vapor[i].z = s3ShipZ + randRange(15.0f, 26.0f);
            s3Vapor[i].x = s3ShipX + randRange(-12.0f, 16.0f);
            s3Vapor[i].y = s3ShipY + randRange(-6.0f, 10.0f);
        }
    }

    // Update supersonic ablation streak sparks
    for (size_t i = 0; i < s3Sparks.size(); ) {
        s3Sparks[i].x += s3Sparks[i].vx * dt;
        s3Sparks[i].y += s3Sparks[i].vy * dt;
        s3Sparks[i].z += s3Sparks[i].vz * dt;
        s3Sparks[i].life -= dt;
        if (s3Sparks[i].life <= 0.0f) {
            s3Sparks[i] = s3Sparks.back();
            s3Sparks.pop_back();
        } else {
            ++i;
        }
    }

    // Update 3D Volumetric Cloud Deck streaming past Ranger in Shot 2
    if (t >= 50.0f && t < 63.5f) {
        for (size_t i = 0; i < s3VolumetricClouds.size(); ++i) {
            VolumetricCloud& c = s3VolumetricClouds[i];
            c.x -= s3ShipDirX * c.speed * dt;
            c.y -= s3ShipDirY * c.speed * dt;
            c.z -= s3ShipDirZ * c.speed * dt;
            c.rot += c.rotSpeed * dt;

            // Recycle ahead along flight dive vector
            float relX = c.x - s3ShipX;
            float relY = c.y - s3ShipY;
            float relZ = c.z - s3ShipZ;
            float dot = relX * s3ShipDirX + relY * s3ShipDirY + relZ * s3ShipDirZ;
            if (dot < -16.0f) {
                float distAhead = randRange(14.0f, 28.0f);
                c.x = s3ShipX + s3ShipDirX * distAhead + randRange(-16.0f, 16.0f);
                c.y = randRange(3.0f, 12.0f);
                c.z = s3ShipZ + s3ShipDirZ * distAhead + randRange(-16.0f, 16.0f);
                c.scale = randRange(3.2f, 8.5f);
            }
        }
    }

    // ------------------------------------------------------------------------
    // SHOT 1: SIDE-PROFILE ATMOSPHERIC PIERCE & HEATSHIELD TEAR (44.0s - 50.5s)
    // ------------------------------------------------------------------------
    if (t < 50.5f) {
        float u = (t - 44.0f) / 6.5f; // 6.5s cinematic hero shot (trimmed last 1s hold)

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
        s3HeatGlow  = lerpf(0.85f, 1.0f, u); // High brilliant incandescence even in initial space entry

        float dirX = s3ShipDirX, dirY = s3ShipDirY, dirZ = s3ShipDirZ;
        float wupX = 0.0f, wupY = 1.0f, wupZ = 0.0f;
        float rightX = dirY * wupZ - dirZ * wupY;
        float rightY = dirZ * wupX - dirX * wupZ;
        float rightZ = dirX * wupY - dirY * wupX;
        normalize3(rightX, rightY, rightZ);
        float upX = rightY * dirZ - rightZ * dirY;
        float upY = rightZ * dirX - rightX * dirZ;
        float upZ = rightX * dirY - rightY * dirX;

        // Pre-warm sparks for instant steady-state wake stream
        if (s3Sparks.empty()) {
            for (int i = 0; i < 45; ++i) {
                float lLateral = randRange(-0.40f, 0.40f);
                float lAlong   = randRange(-1.10f, 0.10f);
                float lHeight  = randRange(-0.12f, 0.02f);

                float speed = randRange(14.0f, 28.0f);
                float maxLife = randRange(0.25f, 0.45f);
                float age = randRange(0.01f, maxLife * 0.95f);

                ReentrySpark sp;
                sp.vx = -dirX * speed + rightX * randRange(-0.8f, 0.8f) + upX * randRange(-0.5f, 0.5f);
                sp.vy = -dirY * speed + rightY * randRange(-0.8f, 0.8f) + upY * randRange(-0.5f, 0.5f);
                sp.vz = -dirZ * speed + rightZ * randRange(-0.8f, 0.8f) + upZ * randRange(-0.5f, 0.5f);

                float spawnX = s3ShipX + (rightX * lLateral + upX * lHeight + dirX * lAlong) * s3ShipScale;
                float spawnY = s3ShipY + (rightY * lLateral + upY * lHeight + dirY * lAlong) * s3ShipScale;
                float spawnZ = s3ShipZ + (rightZ * lLateral + upZ * lHeight + dirZ * lAlong) * s3ShipScale;

                sp.x = spawnX + sp.vx * age;
                sp.y = spawnY + sp.vy * age;
                sp.z = spawnZ + sp.vz * age;
                sp.len = randRange(0.8f, 2.0f);
                sp.maxLife = maxLife;
                sp.life = maxLife - age;
                sp.size = randRange(2.5f, 4.5f);
                sp.r = 1.0f;
                sp.g = randRange(0.75f, 0.98f);
                sp.b = randRange(0.20f, 0.55f);
                s3Sparks.push_back(sp);
            }
        }

        // Supersonic spallation embers & ablation sparks peeling into slipstream
        for (int k = 0; k < 6; ++k) {
            float lLateral = randRange(-0.40f, 0.40f);
            float lAlong   = randRange(-1.10f, 0.10f);
            float lHeight  = randRange(-0.12f, 0.02f);

            float spawnX = s3ShipX + (rightX * lLateral + upX * lHeight + dirX * lAlong) * s3ShipScale;
            float spawnY = s3ShipY + (rightY * lLateral + upY * lHeight + dirY * lAlong) * s3ShipScale;
            float spawnZ = s3ShipZ + (rightZ * lLateral + upZ * lHeight + dirZ * lAlong) * s3ShipScale;

            ReentrySpark sp;
            sp.x = spawnX;
            sp.y = spawnY;
            sp.z = spawnZ;
            float speed = randRange(14.0f, 28.0f);
            sp.vx = -dirX * speed + rightX * randRange(-0.8f, 0.8f) + upX * randRange(-0.5f, 0.5f);
            sp.vy = -dirY * speed + rightY * randRange(-0.8f, 0.8f) + upY * randRange(-0.5f, 0.5f);
            sp.vz = -dirZ * speed + rightZ * randRange(-0.8f, 0.8f) + upZ * randRange(-0.5f, 0.5f);
            sp.len = randRange(0.8f, 2.0f);
            sp.maxLife = randRange(0.25f, 0.45f);
            sp.life = sp.maxLife;
            sp.size = randRange(2.5f, 4.5f);
            sp.r = 1.0f;
            sp.g = randRange(0.75f, 0.98f);
            sp.b = randRange(0.20f, 0.55f);
            s3Sparks.push_back(sp);
        }
    }
    // ------------------------------------------------------------------------
    // SHOT 2: HYPERSONIC OVER-THE-SHOULDER DIVE & HIGH-SPEED BREAKAWAY (50.5s - 63.0s)
    // ------------------------------------------------------------------------
    else {
        // Hypersonic downward dive vector
        s3ShipDirX =  0.60f;
        s3ShipDirY = -0.42f;
        s3ShipDirZ =  0.75f;
        normalize3(s3ShipDirX, s3ShipDirY, s3ShipDirZ);

        if (t < 60.5f) {
            float u = (t - 50.5f) / 10.0f;
            float ease = u * u;

            s3ShipX = lerpf(-9.0f, -2.0f, u);
            s3ShipY = lerpf( 7.5f,  3.2f, ease);
            s3ShipZ = lerpf(-9.0f, -1.5f, u);

            s3ShipPitchExtra = lerpf(8.0f, 4.0f, u);
            s3ShipRoll  = lerpf(10.0f, 4.0f, u);
            s3HeatGlow  = lerpf(1.0f, 0.20f, u);
        } else {
            // Bullet Breakaway: Camera halts; Ranger rockets forward and down at hypersonic bullet speed into Gale Crater
            float dtBreak = t - 60.5f;
            float plungeDist = dtBreak * 8.5f + 5.5f * dtBreak * dtBreak;

            s3ShipX = -2.0f + s3ShipDirX * plungeDist;
            s3ShipY =  3.2f + s3ShipDirY * plungeDist;
            s3ShipZ = -1.5f + s3ShipDirZ * plungeDist;

            s3ShipPitchExtra = 3.5f;
            s3ShipRoll  = 3.5f;
            s3HeatGlow  = std::max(0.0f, 0.20f - dtBreak * 0.12f);
        }
        s3ShipScale = 0.50f;

        // Residual sparks fading out
        if (t < 62.4f) {
            for (int f = 0; f < 2; ++f) {
                ReentrySpark sp;
                sp.x = s3ShipX + randRange(-0.3f, 0.3f);
                sp.y = s3ShipY - 0.08f;
                sp.z = s3ShipZ + randRange(-0.2f, 0.2f);
                float spd = randRange(14.0f, 22.0f);
                sp.vx = -s3ShipDirX * spd;
                sp.vy = -s3ShipDirY * spd;
                sp.vz = -s3ShipDirZ * spd;
                sp.len = randRange(0.6f, 1.4f);
                sp.maxLife = randRange(0.15f, 0.25f);
                sp.life = sp.maxLife;
                sp.size = randRange(2.0f, 3.5f);
                sp.r = 1.0f;
                sp.g = 0.65f;
                sp.b = 0.15f;
                s3Sparks.push_back(sp);
            }
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
    // SHOT 1: SIDE-PROFILE HERO SHOT (44.0s - 50.5s)
    // ------------------------------------------------------------------------
    if (t < 50.5f) {
        float u = (t - 44.0f) / 6.5f;

        // Position camera to the side-above the diving ship (side profile)
        eyeX = s3ShipX + 2.4f;
        eyeY = s3ShipY + 0.65f;
        eyeZ = s3ShipZ - 0.40f;

        targetX = s3ShipX - 0.35f;
        targetY = s3ShipY - 0.60f; // Look downward along the plunge trajectory
        targetZ = s3ShipZ + 0.35f;

        // High-frequency buffeting vibration
        float shake = (1.0f - u * 0.3f) * 0.015f;
        eyeX += sinf(t * 60.0f) * shake;
        eyeY += cosf(t * 54.0f) * shake;
        eyeZ += sinf(t * 72.0f) * shake;
    }
    // ------------------------------------------------------------------------
    // SHOT 2: HYPERSONIC CHASE DIVE & OVER-THE-SHOULDER HOLD (50.5s - 63.0s)
    // ------------------------------------------------------------------------
    else {
        float dirX =  0.60f, dirY = -0.42f, dirZ =  0.75f;
        normalize3(dirX, dirY, dirZ);

        if (t < 60.5f) {
            float u = (t - 50.5f) / 10.0f;
            eyeX = s3ShipX - dirX * 3.4f + 1.1f;
            eyeY = s3ShipY - dirY * 3.4f + 0.7f;
            eyeZ = s3ShipZ - dirZ * 3.4f + 1.4f;

            targetX = s3ShipX + dirX * 1.5f;
            targetY = s3ShipY + dirY * 1.5f;
            targetZ = s3ShipZ + dirZ * 1.5f;

            float shake = (1.0f - u * 0.5f) * 0.020f;
            eyeX += sinf(t * 42.0f) * shake;
            eyeY += cosf(t * 38.0f) * shake;
        } else {
            // Camera stops following the Ranger, locking at its over-the-shoulder vantage
            float dtBreak = t - 60.5f;
            float camDrift = clampf(dtBreak * 0.10f, 0.0f, 0.30f);

            const float baseShipX = -2.0f;
            const float baseShipY =  3.2f;
            const float baseShipZ = -1.5f;

            eyeX = (baseShipX - dirX * 3.4f + 1.1f) + dirX * camDrift;
            eyeY = (baseShipY - dirY * 3.4f + 0.7f) + dirY * camDrift;
            eyeZ = (baseShipZ - dirZ * 3.4f + 1.4f) + dirZ * camDrift;

            targetX = baseShipX + dirX * 1.5f;
            targetY = baseShipY + dirY * 1.5f;
            targetZ = baseShipZ + dirZ * 1.5f;

            float shake = std::max(0.003f, 0.010f - dtBreak * 0.003f);
            eyeX += sinf(t * 30.0f) * shake;
            eyeY += cosf(t * 26.0f) * shake;
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

    if (seqTime < 50.5f) {
        // Shot 1 (44.0s - 50.5s): Pure space black for first 1.5s, then smooth 2.5s transition into complete red/orange atmosphere held for final 2.5s
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        glBegin(GL_QUADS);
        float entryProg = clampf((seqTime - 45.5f) / 2.5f, 0.0f, 1.0f);
        float smoothProg = entryProg * entryProg * (3.0f - 2.0f * entryProg);

        // Horizon transitions from space black (0,0,0) to Martian salmon/butterscotch
        float hR = lerpf(0.0f, 0.72f, smoothProg);
        float hG = lerpf(0.0f, 0.38f, smoothProg);
        float hB = lerpf(0.0f, 0.22f, smoothProg);
        glColor3f(hR, hG, hB);
        glVertex2f(0.0f, 0.0f);
        glVertex2f(1.0f, 0.0f);

        // Zenith transitions from space pitch black (0,0,0) to cosmic rust/indigo
        float zR = lerpf(0.0f, 0.20f, smoothProg);
        float zG = lerpf(0.0f, 0.08f, smoothProg);
        float zB = lerpf(0.0f, 0.12f, smoothProg);
        glColor3f(zR, zG, zB);
        glVertex2f(1.0f, 1.0f);
        glVertex2f(0.0f, 1.0f);
        glEnd();

        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
    } else {
        // Shot 2 (50.5s - 61.5s): Vibrant, 100% rich warm Martian orange atmosphere (zero black patches)
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        glBegin(GL_QUADS);
            // Lower atmosphere: glowing Martian butterscotch/salmon orange
            glColor3f(0.76f, 0.40f, 0.22f);
            glVertex2f(0.0f, 0.0f);
            glVertex2f(1.0f, 0.0f);
            // Upper atmosphere: rich warm Martian rust (pure orange tones, never black)
            glColor3f(0.52f, 0.24f, 0.14f);
            glVertex2f(1.0f, 1.0f);
            glVertex2f(0.0f, 1.0f);
        glEnd();

        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
    }

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

static void renderAtmosphericVaporStreaks() {
    if (seqTime < 46.0f || seqTime > 53.5f || s3Vapor.empty() || !smokeTexID) return;

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Alpha blend prevents additive whiteout
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, smokeTexID);
    glDepthMask(GL_FALSE);

    // Fade clouds in smoothly with atmospheric entry
    float cloudFade = clampf((seqTime - 46.0f) / 1.5f, 0.0f, 1.0f);

    // Billowing Martian high-altitude wisps & dust vapor strands (warm atmospheric rust/salmon)
    for (size_t i = 0; i < s3Vapor.size(); ++i) {
        const VaporStreak& v = s3Vapor[i];
        // Rich Martian atmospheric tones: deep salmon/rust to warm amber dust wisps
        float tint = (float)(i % 5) / 5.0f;
        float cr = lerpf(0.68f, 0.82f, tint);
        float cg = lerpf(0.32f, 0.44f, tint);
        float cb = lerpf(0.18f, 0.24f, tint);
        float ca = v.alpha * 0.32f * cloudFade;

        glColor4f(cr, cg, cb, ca);

        glPushMatrix();
        glTranslatef(v.x, v.y, v.z);
        glScalef(v.scale * 2.8f, v.scale * 1.4f, v.length * 1.5f);
        glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -0.5f, -1.0f);
            glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -0.5f, -1.0f);
            glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  0.5f,  1.0f);
            glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  0.5f,  1.0f);
        glEnd();
        glPopMatrix();
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}



// ============================================================================
// DENSE 3D VOLUMETRIC CLOUD DECK (Surrounding Ranger in Shot 2 with Breakthrough Reveal)
// ============================================================================
static void renderVolumetricCloudDeck() {
    if (seqTime < 50.5f || seqTime > 62.5f || s3VolumetricClouds.empty()) return;

    GLuint tex = cloudTexID ? cloudTexID : smokeTexID;
    if (!tex) return;

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, tex);
    glDepthMask(GL_FALSE);

    // Extract camera right & up vectors from modelview matrix for camera-facing billboarding
    float mv[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, mv);
    float camRightX = mv[0], camRightY = mv[4], camRightZ = mv[8];
    float camUpX    = mv[1], camUpY    = mv[5], camUpZ    = mv[9];

    // Breakthrough factor: 50.5s - 56.5s (6s) is inside dense clouds; 56.5s - 57.35s breaks through
    float breakthrough = 0.0f;
    if (seqTime >= 56.5f) {
        breakthrough = clampf((seqTime - 56.5f) / 0.85f, 0.0f, 1.0f);
        breakthrough = breakthrough * breakthrough * (3.0f - 2.0f * breakthrough);
    }

    for (size_t i = 0; i < s3VolumetricClouds.size(); ++i) {
        const VolumetricCloud& c = s3VolumetricClouds[i];

        // Part clouds underneath the Ranger as it bursts through the cloud floor
        float floorFade = 1.0f;
        if (c.y < s3ShipY + 1.2f && breakthrough > 0.0f) {
            floorFade = (1.0f - breakthrough * 0.95f);
        }
        if (floorFade <= 0.02f) continue;

        // Camera proximity fade to prevent near-plane clipping
        float dx = c.x - s3ShipX;
        float dy = c.y - s3ShipY;
        float dz = c.z - s3ShipZ;
        float distSq = dx * dx + dy * dy + dz * dz;
        float camFade = clampf((distSq - 1.2f) / 3.0f, 0.0f, 1.0f);

        float a = c.alpha * floorFade * camFade;
        if (a <= 0.01f) continue;

        // Authentic multi-tier Martian atmospheric lighting
        float cr = lerpf(0.85f, 0.72f, c.tint);
        float cg = lerpf(0.48f, 0.32f, c.tint);
        float cb = lerpf(0.26f, 0.16f, c.tint);

        // Warm amber/gold forward scattering for upper cloud tops
        if (c.y > 6.0f) {
            cr = lerpf(cr, 1.0f, 0.28f);
            cg = lerpf(cg, 0.75f, 0.28f);
            cb = lerpf(cb, 0.45f, 0.28f);
        }

        glColor4f(cr, cg, cb, a);

        float cosR = cosf(c.rot) * c.scale;
        float sinR = sinf(c.rot) * c.scale;
        float rx = camRightX * cosR + camUpX * sinR;
        float ry = camRightY * cosR + camUpY * sinR;
        float rz = camRightZ * cosR + camUpZ * sinR;
        float ux = -camRightX * sinR + camUpX * cosR;
        float uy = -camRightY * sinR + camUpY * cosR;
        float uz = -camRightZ * sinR + camUpZ * cosR;

        glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex3f(c.x - rx - ux, c.y - ry - uy, c.z - rz - uz);
            glTexCoord2f(1.0f, 0.0f); glVertex3f(c.x + rx - ux, c.y + ry - uy, c.z + rz - uz);
            glTexCoord2f(1.0f, 1.0f); glVertex3f(c.x + rx + ux, c.y + ry + uy, c.z + rz + uz);
            glTexCoord2f(0.0f, 1.0f); glVertex3f(c.x - rx + ux, c.y - ry + uy, c.z - rz + uz);
        glEnd();
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

// ============================================================================
// 1. SILKY CONTINUOUS AERODYNAMIC PLASMA SHEATH (Super-smooth flow matching reference)
// ============================================================================
static void renderSmoothPlasmaSheath(bool backPass) {
    if (s3HeatGlow <= 0.01f) return;

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Pure radiant blackbody plasma
    glDepthMask(GL_FALSE);

    float t = seqTime;
    float glow = s3HeatGlow;
    // High-frequency subtle atmospheric plasma shimmer
    float flicker = glow * (0.95f + 0.05f * sinf(t * 70.0f) * cosf(t * 45.0f));

    // Cull faces: backPass renders the inside/back cavity; !backPass renders front translucent sheath
    glEnable(GL_CULL_FACE);
    glCullFace(backPass ? GL_FRONT : GL_BACK);

    const int RINGS  = 44;
    const int SLICES = 52;
    const float zApex = 1.05f; // Fully encapsulates Ranger nose apex (Z = 1.00, Y = -0.0835, X = 0.0)
    const float zTail = -4.50f;

    auto getCrossSection = [](float z, float& w, float& h, float& x, float& y) {
        // Grand, sweeping plasma reentry envelope perfectly centered on nose
        if (z > 0.85f) {
            float frac = (1.05f - z) / 0.20f;
            frac = clampf(frac, 0.0f, 1.0f);
            float taper = powf(frac, 0.65f);
            w = 0.22f * taper;
            h = 0.16f * taper;
            x = 0.0f;
            y = -0.085f;
        } else if (z > -1.15f) {
            float frac = (0.85f - z) / 2.00f;
            w = 0.22f + 1.15f * sinf(frac * 1.57079f);
            h = 0.16f + 0.48f * sinf(frac * 1.57079f);
            x = 0.0f;
            y = -0.085f - 0.020f * frac;
        } else {
            float frac = clampf((-1.15f - z) / 3.35f, 0.0f, 1.0f);
            w = 1.37f - 0.38f * frac;
            h = 0.64f - 0.24f * frac;
            x = 0.0f;
            y = -0.105f + 0.02f * frac;
        }
    };

    auto getThermalColor = [&](float f, float sinP, float& cr, float& cg, float& cb, float& ca) {
        // Boost contrast and radiant brilliance against dark space
        float spaceBoost = (seqTime < 46.5f) ? 1.35f : 1.0f;

        if (f < 0.035f) {
            // Stagnation point: Sharp, pinpoint nuclear white-hot tip right at the nose point
            float s = f / 0.035f;
            cr = 1.0f;
            cg = lerpf(1.0f, 0.96f, s);
            cb = lerpf(0.98f, 0.65f, s);
            ca = flicker * lerpf(1.0f, 0.92f, s) * spaceBoost;
        } else if (f < 0.11f) {
            // Tight nosecap transition: Concentrated solar gold & radiant yellow
            float s = (f - 0.035f) / 0.075f;
            cr = 1.0f;
            cg = lerpf(0.96f, 0.60f, s);
            cb = lerpf(0.65f, 0.08f, s);
            ca = flicker * lerpf(0.92f, 0.65f, s) * spaceBoost;
        } else if (f < 0.42f) {
            // Chine flanks & fuselage: Deep luminous flame vermilion & amber
            float s = (f - 0.11f) / 0.31f;
            cr = 1.0f;
            cg = lerpf(0.60f, 0.22f, s);
            cb = lerpf(0.08f, 0.01f, s);
            ca = flicker * lerpf(0.65f, 0.38f, s) * spaceBoost;
        } else {
            // Trailing wake slipstream: Cooling Martian crimson fading smoothly to 0
            float s = (f - 0.42f) / 0.58f;
            cr = lerpf(1.0f, 0.65f, s);
            cg = lerpf(0.22f, 0.03f, s);
            cb = 0.01f;
            ca = flicker * (1.0f - s) * 0.34f * spaceBoost;
        }

        // Underbelly compression boost (angle of attack windward compression)
        float bellyFactor = clampf(-sinP, 0.0f, 1.0f);
        ca *= (1.0f + bellyFactor * 0.50f);

        if (backPass) {
            ca *= 0.50f; // Soft inner cavity glow behind craft
        } else {
            // Front pass: brilliant compact nose cap, while flanks remain translucent to reveal Ranger
            if (f < 0.035f) {
                ca *= 0.85f;
            } else {
                ca *= 0.32f;
            }
        }
    };

    // 1. Primary Aerodynamic Sheath Pass
    for (int r = 0; r < RINGS; ++r) {
        float f0 = (float)r / (float)RINGS;
        float f1 = (float)(r + 1) / (float)RINGS;

        float z0 = zApex - powf(f0, 1.25f) * (zApex - zTail);
        float z1 = zApex - powf(f1, 1.25f) * (zApex - zTail);

        float w0, h0, x0, y0, w1, h1, x1, y1;
        getCrossSection(z0, w0, h0, x0, y0);
        getCrossSection(z1, w1, h1, x1, y1);

        glBegin(GL_TRIANGLE_STRIP);
        for (int s = 0; s <= SLICES; ++s) {
            float phi = (float)s / (float)SLICES * 2.0f * (float)M_PI;
            float cosP = cosf(phi);
            float sinP = sinf(phi);

            // Subtle aerodynamic boundary shimmer
            float flutter0 = sinf(t * 60.0f + phi * 3.0f + f0 * 16.0f) * 0.005f;
            float flutter1 = sinf(t * 60.0f + phi * 3.0f + f1 * 16.0f) * 0.005f;

            float cr0, cg0, cb0, ca0;
            float cr1, cg1, cb1, ca1;
            getThermalColor(f0, sinP, cr0, cg0, cb0, ca0);
            getThermalColor(f1, sinP, cr1, cg1, cb1, ca1);

            glColor4f(cr0, cg0, cb0, ca0);
            glVertex3f(x0 + cosP * (w0 + flutter0), y0 + sinP * (h0 + flutter0), z0);

            glColor4f(cr1, cg1, cb1, ca1);
            glVertex3f(x1 + cosP * (w1 + flutter1), y1 + sinP * (h1 + flutter1), z1);
        }
        glEnd();
    }

    glCullFace(GL_BACK);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);
}

// ============================================================================
// 2. REALISTIC OBLIQUE SHOCK LAYER & COMPRESSION CRESCENT (Sharp Supersonic Conical Boundary)
// ============================================================================
static void renderDetachedShockArc() {
    if (s3HeatGlow <= 0.01f) return;
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE); // Ensure symmetric rendering across both port and starboard flanks
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);

    float t = seqTime;
    float flicker = s3HeatGlow * (0.94f + 0.06f * sinf(t * 85.0f));
    if (t < 46.5f) flicker *= 1.30f; // Boost shock brilliance against space black

    // Oblique shock wave filaments
    glLineWidth(1.6f);
    glBegin(GL_LINES);
    for (int side = -1; side <= 1; side += 2) {
        for (int ribbon = 0; ribbon < 3; ++ribbon) {
            float frac = (float)ribbon / 2.0f;
            float zStart = 0.98f - frac * 0.25f;
            float zEnd   = zStart - 3.20f;
            float xStart = (float)side * (0.16f + frac * 0.18f);
            float xEnd   = xStart + (float)side * 1.35f;
            float yStart = -0.085f - frac * 0.02f;
            float yEnd   = yStart - 0.05f;

            float shimmer = sinf(t * 70.0f + (float)side * 2.0f + (float)ribbon * 3.0f) * 0.015f;
            glColor4f(1.0f, 0.88f, 0.45f, flicker * 0.70f * (1.0f - frac * 0.3f));
            glVertex3f(xStart, yStart, zStart);

            glColor4f(0.85f, 0.22f, 0.02f, 0.0f);
            glVertex3f(xEnd + shimmer, yEnd, zEnd);
        }
    }
    glEnd();
    glLineWidth(1.0f);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);
}

// ============================================================================
// 3. UNDERBELLY CERAMIC TILE INCANDESCENCE (TPS Blackbody Direct Emission)
// ============================================================================
static void renderHeatshieldTiles() {
    if (s3HeatGlow <= 0.01f) return;

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);

    float t = seqTime;
    float flicker = s3HeatGlow * (0.92f + 0.08f * sinf(t * 73.0f) * cosf(t * 49.0f));

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
    glEnd();

    if (smokeTexID) {
        glDisable(GL_TEXTURE_2D);
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);
}

// ============================================================================
// 4. SUPERSONIC HORIZONTAL ABLATION SPARK STREAKS (Matching reference image)
// ============================================================================
static void renderReentrySparks() {
    if (s3Sparks.empty()) return;
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Pure radiant additive streaks
    glDepthMask(GL_FALSE);

    glLineWidth(2.2f);
    glBegin(GL_LINES);
    for (size_t i = 0; i < s3Sparks.size(); ++i) {
        const ReentrySpark& s = s3Sparks[i];
        float norm = s.life / s.maxLife; // 1.0 -> 0.0
        float alpha = norm * 0.92f;

        float vlen = sqrtf(s.vx * s.vx + s.vy * s.vy + s.vz * s.vz);
        float ndx = (vlen > 0.001f) ? s.vx / vlen : 0.0f;
        float ndy = (vlen > 0.001f) ? s.vy / vlen : 0.0f;
        float ndz = (vlen > 0.001f) ? s.vz / vlen : 0.0f;

        float trailLen = std::min(s.len, (s.maxLife - s.life) * 18.0f);
        float tailX = s.x - ndx * trailLen;
        float tailY = s.y - ndy * trailLen;
        float tailZ = s.z - ndz * trailLen;

        // Head (bright white-gold)
        glColor4f(s.r, s.g, s.b, alpha);
        glVertex3f(s.x, s.y, s.z);

        // Tail (trailing streak fading into orange-crimson at alpha 0)
        glColor4f(1.0f, s.g * 0.35f, 0.02f, 0.0f);
        glVertex3f(tailX, tailY, tailZ);
    }
    glEnd();
    glLineWidth(1.0f);

    glPointSize(3.5f);
    glBegin(GL_POINTS);
    for (size_t i = 0; i < s3Sparks.size(); ++i) {
        const ReentrySpark& s = s3Sparks[i];
        float norm = s.life / s.maxLife;
        glColor4f(1.0f, 0.98f, 0.85f, norm * 0.95f);
        glVertex3f(s.x, s.y, s.z);
    }
    glEnd();
    glPointSize(1.0f);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);
}



void renderScene3() {
    // 1. Martian Atmospheric Gradient Sky
    renderAtmosphericSkybox();

    // 2. High-Altitude Atmospheric Cloud & Vapor Shreds
    renderAtmosphericVaporStreaks();

    // 3. Official NASA Gale Crater 3D Topography Mesh
    // Completely shrouded by dense cloud deck for first 6 seconds (50.5s - 56.5s), then dramatically revealed at 56.5s
    float craterReveal = 1.0f;
    if (seqTime < 50.5f || seqTime < 56.5f) {
        craterReveal = 0.0f;
    } else if (seqTime < 57.35f) {
        float u = (seqTime - 56.5f) / 0.85f;
        craterReveal = u * u * (3.0f - 2.0f * u);
    } else {
        craterReveal = 1.0f;
    }

    if (craterLoaded && craterDisplayList != 0 && craterReveal > 0.01f) {
        glPushMatrix();
        float terrainY = -4.80f;
        glTranslatef(0.0f, terrainY, 0.0f);

        if (craterTexID) {
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, craterTexID);
            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        }

        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        // Direct sunlight shining from high behind the camera in Shot 2 to eliminate all terrain shadows
        GLfloat terrainSunPos[4] = { -15.0f, 45.0f, -25.0f, 0.0f };
        GLfloat terrainSunDiff[] = { 1.65f * craterReveal, 1.15f * craterReveal, 0.85f * craterReveal, 1.0f };
        GLfloat terrainSunAmb[]  = { 0.75f * craterReveal, 0.48f * craterReveal, 0.32f * craterReveal, 1.0f };
        glLightfv(GL_LIGHT0, GL_POSITION, terrainSunPos);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, terrainSunDiff);
        glLightfv(GL_LIGHT0, GL_AMBIENT, terrainSunAmb);

        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

        GLfloat matSpec[] = { 0.28f, 0.20f, 0.14f, 1.0f };
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpec);
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 16.0f);

        if (craterReveal < 0.99f) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glColor4f(1.0f, 1.0f, 1.0f, craterReveal);
        } else {
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        }

        glCallList(craterDisplayList);

        if (craterReveal < 0.99f) {
            glDisable(GL_BLEND);
        }
        if (craterTexID) {
            glDisable(GL_TEXTURE_2D);
        }
        glDisable(GL_COLOR_MATERIAL);
        glPopMatrix();
    }

    // 4. Star Sparrow / Ranger Lander Craft with Silky Smooth Plasma Sheath
    glPushMatrix();
    glTranslatef(s3ShipX, s3ShipY, s3ShipZ);
    glScalef(s3ShipScale, s3ShipScale, s3ShipScale);

    // Orientation: Compute true descent plunge pitch
    float yaw = atan2f(s3ShipDirX, s3ShipDirZ) * 180.0f / (float)M_PI;
    float horizLen = sqrtf(s3ShipDirX * s3ShipDirX + s3ShipDirZ * s3ShipDirZ);
    float flightPitch = -atan2f(s3ShipDirY, horizLen) * 180.0f / (float)M_PI;

    glRotatef(yaw, 0.0f, 1.0f, 0.0f);
    glRotatef(flightPitch + s3ShipPitchExtra, 1.0f, 0.0f, 0.0f);
    glRotatef(s3ShipRoll, 0.0f, 0.0f, 1.0f);

    // Pass A: Render Inside / Back Wall of the silky plasma envelope
    renderSmoothPlasmaSheath(true);

    // Pass B: Key & Fill lighting for crisp, brilliant Ranger spacecraft illumination
    glEnable(GL_LIGHT2);
    GLfloat shipKeyPos[]  = { 1.5f, 3.5f, 2.0f, 0.0f };
    GLfloat shipKeyDiff[] = { 2.4f, 2.3f, 2.2f, 1.0f };
    GLfloat shipKeyAmb[]  = { 0.95f, 0.95f, 1.0f, 1.0f };
    glLightfv(GL_LIGHT2, GL_POSITION, shipKeyPos);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, shipKeyDiff);
    glLightfv(GL_LIGHT2, GL_AMBIENT, shipKeyAmb);

    // Dynamic underbelly heatshield incandescence illumination
    if (s3HeatGlow > 0.05f) {
        glEnable(GL_LIGHT3);
        GLfloat heatPos[]  = { 0.0f, -1.0f, 0.35f, 1.0f };
        GLfloat heatDiff[] = { 3.4f * s3HeatGlow, 1.9f * s3HeatGlow, 0.35f * s3HeatGlow, 1.0f };
        GLfloat heatAmb[]  = { 0.65f * s3HeatGlow, 0.30f * s3HeatGlow, 0.05f * s3HeatGlow, 1.0f };
        glLightfv(GL_LIGHT3, GL_POSITION, heatPos);
        glLightfv(GL_LIGHT3, GL_DIFFUSE, heatDiff);
        glLightfv(GL_LIGHT3, GL_AMBIENT, heatAmb);
    } else {
        glDisable(GL_LIGHT3);
    }

    // Ranger spacecraft model (clearly visible inside the cocoon)
    renderRangerMesh();

    // Pass C: Ceramic tile underbelly blackbody direct emission
    renderHeatshieldTiles();

    // Pass D: Render Outside / Front Wall of the silky plasma envelope with feathered boundary
    renderSmoothPlasmaSheath(false);

    // Pass E: Oblique shock layer & supersonic conical boundary
    renderDetachedShockArc();

    glDisable(GL_LIGHT2);
    glDisable(GL_LIGHT3);
    glPopMatrix();

    // 5. Supersonic horizontal ablation sparks (in world space)
    renderReentrySparks();

    // 6. Dense 3D Volumetric Cloud Deck (Surrounding Ranger in Shot 2 with Breakthrough Reveal)
    renderVolumetricCloudDeck();
}
