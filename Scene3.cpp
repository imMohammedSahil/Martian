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

    // ------------------------------------------------------------------------
    // SHOT 1: SIDE-PROFILE ATMOSPHERIC PIERCE & HEATSHIELD TEAR (44.0s - 50.0s)
    // ------------------------------------------------------------------------
    if (t < 50.0f) {
        float u = (t - 44.0f) / 6.0f; // Extended 6.0s cinematic hero shot

        // Horizontal hypersonic cruise across upper Martian atmosphere
        s3ShipX = lerpf(-14.0f, -6.0f, u);
        s3ShipY = lerpf( 12.0f, 10.5f, u); // Gentle shallow glide
        s3ShipZ = -12.0f;

        // Flying horizontally to the right across the upper atmosphere
        s3ShipDirX =  1.0f;
        s3ShipDirY = -0.04f;
        s3ShipDirZ =  0.0f;

        // Spacecraft attitude: nose pitched slightly up into airflow (+8 deg alpha), slight bank
        s3ShipPitchExtra = -8.0f;
        s3ShipRoll  = -12.0f;
        s3ShipScale = 0.50f;
        s3HeatGlow  = lerpf(0.75f, 1.0f, u);

        // Pre-warm sparks for instant steady-state wake stream across visible screen
        if (s3Sparks.empty()) {
            for (int i = 0; i < 55; ++i) {
                float ang = randRange(0.0f, 6.28318f);
                float rad = randRange(0.20f, 0.70f);
                float lAlong = randRange(-1.30f, 0.10f);
                float ly = sinf(ang) * rad * 0.50f - 0.04f;
                float lz = cosf(ang) * rad * 0.85f;

                float speed = randRange(4.5f, 9.5f);
                float maxLife = randRange(0.60f, 1.20f);
                float age = randRange(0.01f, maxLife * 0.95f);

                ReentrySpark sp;
                sp.vx = -speed;
                sp.vy = randRange(-0.15f, 0.15f);
                sp.vz = randRange(-0.25f, 0.25f);
                sp.x = (s3ShipX + lAlong * s3ShipScale) + sp.vx * age;
                sp.y = (s3ShipY + ly * s3ShipScale) + sp.vy * age;
                sp.z = (s3ShipZ + lz * s3ShipScale) + sp.vz * age;
                sp.len = randRange(0.8f, 2.2f);
                sp.maxLife = maxLife;
                sp.life = maxLife - age;
                sp.size = randRange(2.5f, 4.5f);
                sp.r = 1.0f;
                sp.g = randRange(0.75f, 0.98f);
                sp.b = randRange(0.20f, 0.55f);
                s3Sparks.push_back(sp);
            }
        }

        // Supersonic spallation embers & horizontal ablation sparks (matching reference image)
        for (int k = 0; k < 4; ++k) {
            float ang = randRange(0.0f, 6.28318f);
            float rad = randRange(0.20f, 0.65f);
            // Spawn strictly along the heatshield and mid-body (behind the nose!)
            float lAlong = randRange(-1.10f, 0.10f);
            float ly = sinf(ang) * rad * 0.48f - 0.04f;
            float lz = cosf(ang) * rad * 0.90f;

            // In world space: ship forward is +X, up is +Y, lateral is Z
            float spawnX = s3ShipX + lAlong * s3ShipScale;
            float spawnY = s3ShipY + ly * s3ShipScale;
            float spawnZ = s3ShipZ + lz * s3ShipScale;

            ReentrySpark sp;
            sp.x = spawnX;
            sp.y = spawnY;
            sp.z = spawnZ;
            float speed = randRange(4.5f, 9.5f);
            // Shooting horizontally to the left into the wake slipstream
            sp.vx = -speed;
            sp.vy = randRange(-0.15f, 0.15f);
            sp.vz = randRange(-0.25f, 0.25f);
            sp.len = randRange(0.8f, 2.2f);
            sp.maxLife = randRange(0.60f, 1.20f);
            sp.life = sp.maxLife;
            sp.size = randRange(2.5f, 4.5f);
            sp.r = 1.0f;
            sp.g = randRange(0.75f, 0.98f);
            sp.b = randRange(0.20f, 0.55f);
            s3Sparks.push_back(sp);
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

        // Residual sparks fading out
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
    // SHOT 1: CINEMATIC FLANK HERO SHOT (44.0s - 50.0s)
    // ------------------------------------------------------------------------
    // Majestic cinematic widescreen side-profile matching Apollo/Orion/reference footage:
    // Ranger cruises horizontally rightward across the frame with blunt nose/heatshield leading on the right,
    // silky smooth plasma sheath & ablation sparks streaming leftward into the cosmic void,
    // and the curved atmospheric horizon of Mars arcing across the lower third of the viewport.
    if (t < 50.0f) {
        float u = (t - 44.0f) / 6.0f; // Extended 6.0s hero view

        float dirX = s3ShipDirX, dirY = s3ShipDirY, dirZ = s3ShipDirZ;
        float wupX = 0.0f, wupY = 1.0f, wupZ = 0.0f;
        // Lateral side vector perpendicular to flight direction and world up
        float sideX = dirY * wupZ - dirZ * wupY;
        float sideY = dirZ * wupX - dirX * wupZ;
        float sideZ = dirX * wupY - dirY * wupX;
        normalize3(sideX, sideY, sideZ);

        // Position camera to the port flank of the ship
        // With +side, the flight direction projects towards the RIGHT of the screen:
        // - Leading front / bow shock on the right
        // - Silky trailing wake and sparks streaming to the left
        // - Deep cosmic void above
        // - Curved Martian planetary horizon below
        float sideDist = 2.25f;
        float upDist   = 0.30f;
        float backDist = 0.35f;

        eyeX = s3ShipX + sideX * sideDist - dirX * backDist + wupX * upDist;
        eyeY = s3ShipY + sideY * sideDist - dirY * backDist + wupY * upDist;
        eyeZ = s3ShipZ + sideZ * sideDist - dirZ * backDist + wupZ * upDist;

        // Target: framed so the glowing fireball and ship fill the hero view
        targetX = s3ShipX + dirX * 0.28f;
        targetY = s3ShipY + dirY * 0.28f - 0.03f;
        targetZ = s3ShipZ + dirZ * 0.28f;

        // Subtle high-speed hypersonic buffeting vibration
        float freq = t * 65.0f;
        float amp  = (1.0f - u * 0.2f) * 0.010f;
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

    float t = seqTime;

    if (t < 50.0f) {
        // ====================================================================
        // SHOT 1: CINEMATIC PLANETARY LIMB & UPPER ATMOSPHERE (Matching Reference)
        // ====================================================================
        // 1. Cosmic Void background (pitch black with deep cosmic gradient)
        glBegin(GL_QUADS);
            glColor3f(0.003f, 0.005f, 0.010f); // Pure cosmic blackness
            glVertex2f(0.0f, 1.0f);
            glVertex2f(1.0f, 1.0f);
            glColor3f(0.035f, 0.015f, 0.022f); // Faint cosmic glow above horizon
            glVertex2f(1.0f, 0.32f);
            glVertex2f(0.0f, 0.32f);
        glEnd();

        // 2. Curved Martian Horizon Arc & Atmospheric Rim Glow
        // Circle center placed below screen to create a gentle, majestic planetary curvature
        const int ARC_SEGS = 64;
        const float cx = 0.50f;
        const float cy = -2.20f;
        const float R  = 2.44f; // Radius reaching y ~ 0.24 in the middle

        // Planetary Body below the curved limb (rich Mars terrain tones)
        glBegin(GL_QUAD_STRIP);
        for (int i = 0; i <= ARC_SEGS; ++i) {
            float frac = (float)i / (float)ARC_SEGS;
            float x = frac;
            float dx = x - cx;
            float dySq = R * R - dx * dx;
            float yLimb = cy + (dySq > 0.0f ? sqrtf(dySq) : R);

            // Base of screen (deep shadowed Martian crust)
            glColor3f(0.16f, 0.07f, 0.04f);
            glVertex2f(x, 0.0f);

            // Sunlit surface near limb (warm terra-cotta with subtle latitude variation)
            float latVar = 0.03f * sinf(frac * 18.0f);
            glColor3f(0.52f + latVar, 0.25f + latVar * 0.5f, 0.13f);
            glVertex2f(x, yLimb);
        }
        glEnd();

        // Atmospheric Rim Glow Layer (sunlit forward-scattering haze separating planet from space)
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        // Tier A: Intense sunlit atmospheric edge (radiant pale cyan-white & golden rim)
        glBegin(GL_QUAD_STRIP);
        for (int i = 0; i <= ARC_SEGS; ++i) {
            float frac = (float)i / (float)ARC_SEGS;
            float x = frac;
            float dx = x - cx;
            float dySq = R * R - dx * dx;
            float yLimb = cy + (dySq > 0.0f ? sqrtf(dySq) : R);

            // Warm golden dust interface
            glColor4f(0.98f, 0.72f, 0.45f, 0.90f);
            glVertex2f(x, yLimb - 0.012f);

            // Brilliant sunlit upper atmosphere (pale cyan/white scattering)
            glColor4f(0.85f, 0.93f, 1.0f, 0.85f);
            glVertex2f(x, yLimb + 0.024f);
        }
        glEnd();

        // Tier B: Soft diffuse atmospheric haze extending smoothly into the cosmic void
        glBegin(GL_QUAD_STRIP);
        for (int i = 0; i <= ARC_SEGS; ++i) {
            float frac = (float)i / (float)ARC_SEGS;
            float x = frac;
            float dx = x - cx;
            float dySq = R * R - dx * dx;
            float yLimb = cy + (dySq > 0.0f ? sqrtf(dySq) : R);

            glColor4f(0.55f, 0.70f, 0.95f, 0.45f);
            glVertex2f(x, yLimb + 0.015f);

            glColor4f(0.15f, 0.10f, 0.25f, 0.0f);
            glVertex2f(x, yLimb + 0.105f);
        }
        glEnd();

        glDisable(GL_BLEND);
    } else {
        // Shots 2..5: Ground descent skybox
        glBegin(GL_QUADS);
            glColor3f(0.72f, 0.38f, 0.22f);
            glVertex2f(0.0f, 0.0f);
            glVertex2f(1.0f, 0.0f);
            glColor3f(0.20f, 0.08f, 0.12f);
            glVertex2f(1.0f, 1.0f);
            glVertex2f(0.0f, 1.0f);
        glEnd();
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

static void renderAtmosphericVaporStreaks() {
    if (seqTime < 50.0f || seqTime > 53.5f || s3Vapor.empty() || !smokeTexID) return;

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
    const float zApex = 1.40f; // Wraps generously ahead of Ranger nose (Z ~ 1.10)
    const float zTail = -4.50f;

    auto getCrossSection = [](float z, float& w, float& h, float& y) {
        if (z > 0.80f) {
            // Super-blunt ellipsoidal nose bow shock (matching Apollo/Orion/reference shape)
            float frac = (1.40f - z) / 0.60f;
            frac = clampf(frac, 0.0f, 1.0f);
            float blunt = sqrtf(clampf(1.0f - (1.0f - frac) * (1.0f - frac), 0.0f, 1.0f));
            w = 0.85f * blunt;
            h = 0.54f * blunt;
            y = -0.05f - 0.02f * frac;
        } else if (z > -1.15f) {
            // Fuselage & delta wings lifting body envelope
            float frac = (0.80f - z) / 1.95f;
            w = 0.85f + 0.58f * sinf(frac * 1.57079f);
            h = 0.54f + 0.16f * sinf(frac * 1.57079f);
            y = -0.07f - 0.025f * frac;
        } else {
            // Trailing aerodynamic slipstream tube streaming into wake
            float frac = clampf((-1.15f - z) / 3.35f, 0.0f, 1.0f);
            w = 1.43f - 0.40f * frac;
            h = 0.70f - 0.25f * frac;
            y = -0.095f + 0.02f * frac;
        }
    };

    auto getThermalColor = [&](float f, float sinP, float& cr, float& cg, float& cb, float& ca) {
        if (f < 0.12f) {
            // Stagnation point: Brilliant nuclear white-hot (> 6,500 K)
            float s = f / 0.12f;
            cr = 1.0f;
            cg = lerpf(1.0f, 0.95f, s);
            cb = lerpf(0.98f, 0.55f, s);
            ca = flicker * lerpf(1.0f, 0.82f, s);
        } else if (f < 0.40f) {
            // Compression & chine flanks: Incandescent solar gold & radiant amber
            float s = (f - 0.12f) / 0.28f;
            cr = 1.0f;
            cg = lerpf(0.95f, 0.60f, s);
            cb = lerpf(0.55f, 0.08f, s);
            ca = flicker * lerpf(0.82f, 0.50f, s);
        } else if (f < 0.72f) {
            // Mid-body slipstream: Luminous flame vermilion
            float s = (f - 0.40f) / 0.32f;
            cr = 1.0f;
            cg = lerpf(0.60f, 0.22f, s);
            cb = lerpf(0.08f, 0.01f, s);
            ca = flicker * lerpf(0.50f, 0.26f, s);
        } else {
            // Trailing wake slipstream: Cooling Martian crimson fading smoothly to 0
            float s = (f - 0.72f) / 0.28f;
            cr = lerpf(1.0f, 0.65f, s);
            cg = lerpf(0.22f, 0.03f, s);
            cb = 0.01f;
            ca = flicker * (1.0f - s) * 0.26f;
        }

        // Underbelly compression boost (angle of attack windward compression)
        float bellyFactor = clampf(-sinP, 0.0f, 1.0f);
        ca *= (1.0f + bellyFactor * 0.50f);

        if (backPass) {
            ca *= 0.50f; // Soft inner cavity glow behind craft
        } else {
            // Front pass: brilliant nose cap, while flanks remain translucent to reveal Ranger
            if (f < 0.12f) {
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

        float w0, h0, y0, w1, h1, y1;
        getCrossSection(z0, w0, h0, y0);
        getCrossSection(z1, w1, h1, y1);

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
            glVertex3f(cosP * (w0 + flutter0), y0 + sinP * (h0 + flutter0), z0);

            glColor4f(cr1, cg1, cb1, ca1);
            glVertex3f(cosP * (w1 + flutter1), y1 + sinP * (h1 + flutter1), z1);
        }
        glEnd();
    }

    glCullFace(GL_BACK);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

// ============================================================================
// 2. DETACHED OUTER BOW-SHOCK ARC (Sharp thin crescent ahead of nose)
// ============================================================================
static void renderDetachedShockArc() {
    if (s3HeatGlow <= 0.01f) return;
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);

    float t = seqTime;
    float flicker = s3HeatGlow * (0.95f + 0.05f * sinf(t * 85.0f));

    // Outer faint detached shock crescent (matching the sharp outer line in the reference photo)
    glLineWidth(2.4f);
    glBegin(GL_LINE_STRIP);
    for (int s = -24; s <= 24; ++s) {
        float phi = (float)s / 24.0f * 1.45f;
        float sinP = sinf(phi);
        float cosP = cosf(phi);
        float x = sinP * 0.74f;
        float y = -0.06f - (1.0f - cosP) * 0.20f;
        float z = 1.42f - (1.0f - cosP) * 0.45f;
        float edgeFade = 1.0f - fabsf((float)s / 24.0f);
        glColor4f(1.0f, 0.94f, 0.76f, flicker * 0.65f * edgeFade);
        glVertex3f(x, y, z);
    }
    glEnd();
    glLineWidth(1.0f);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
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

        // In Shot 1, flight is to the right (+X), so trailing wake is to the LEFT (-X)
        // Trail extends to the left, behind the spark head
        float trailLen = std::min(s.len, (s.maxLife - s.life) * 28.0f);
        float tailX = s.x - trailLen;

        // Head (bright white-gold)
        glColor4f(s.r, s.g, s.b, alpha);
        glVertex3f(s.x, s.y, s.z);

        // Tail (trailing streak fading into orange-crimson at alpha 0)
        glColor4f(1.0f, s.g * 0.35f, 0.02f, 0.0f);
        glVertex3f(tailX, s.y, s.z);
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

    // 5. Star Sparrow / Ranger Lander Craft with Silky Smooth Plasma Sheath
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

    glDisable(GL_LIGHT2);
    glDisable(GL_LIGHT3);
    glPopMatrix();

    // 6. Supersonic horizontal ablation sparks (in world space)
    renderReentrySparks();

    // 7. Volumetric Touchdown Dust Clouds
    renderTouchdownDustParticles();
}
