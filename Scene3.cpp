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
static GLuint cloudTexID = 0;

static bool   soundTriggered = false;

// Dynamic Scene 3 Lander State
static float s3ShipX = 0.0f, s3ShipY = 0.0f, s3ShipZ = 0.0f;
static float s3ShipDirX = 0.0f, s3ShipDirY = -0.5f, s3ShipDirZ = 1.0f;
static float s3ShipRoll = 0.0f, s3ShipPitch = 0.0f;
static float s3ShipScale = 0.35f;

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

    GLfloat pDiff[] = { 1.30f, 1.25f, 1.20f, 1.0f };
    GLfloat pAmb[]  = { 0.70f, 0.65f, 0.60f, 1.0f };
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
    if (!cloudTexID) cloudTexID = loadTextureHelper("cloud.png", false);
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

    // PHASE 1: HYPERSONIC ENTRY & PLASMA FIRE (44.0s - 48.0s)
    if (t < 48.0f) {
        float u = (t - 44.0f) / 4.0f;
        float ease = u * u;

        // Blazing descent through upper atmosphere above crater rim
        s3ShipX = lerpf(-18.0f, -6.0f, u);
        s3ShipY = lerpf( 16.5f,  7.2f, ease);
        s3ShipZ = lerpf(-22.0f, -8.0f, u);

        s3ShipDirX =  0.65f;
        s3ShipDirY = -0.55f;
        s3ShipDirZ =  0.75f;
        normalize3(s3ShipDirX, s3ShipDirY, s3ShipDirZ);

        s3ShipPitch = -24.0f;
        s3ShipRoll  = lerpf(45.0f, 22.0f, u);
        s3ShipScale = 0.40f;

        // Spawn plasma entry flame & heatshield spark particles
        for (int k = 0; k < 4; ++k) {
            float ox = randRange(-0.4f, 0.4f);
            float oy = randRange(-0.3f, 0.3f);
            float oz = randRange(-0.4f, 0.4f);
            spawnParticle(s3ShipX + ox, s3ShipY + oy, s3ShipZ + oz,
                          -s3ShipDirX * 12.0f + randRange(-1.5f, 1.5f),
                          -s3ShipDirY * 12.0f + randRange(-1.0f, 1.0f),
                          -s3ShipDirZ * 12.0f + randRange(-1.5f, 1.5f),
                          randRange(0.25f, 0.55f), randRange(1.8f, 3.2f),
                          1.0f, randRange(0.4f, 0.85f), 0.15f, true);
        }
    }
    // PHASE 2: CLOUD BREAK & CANYON REVEAL (48.0s - 53.0s)
    else if (t < 53.0f) {
        float u = (t - 48.0f) / 5.0f;
        float smoothU = smoothstep(0.0f, 1.0f, u);

        // Sweeping descent banking through Gale Crater mountain pass
        s3ShipX = lerpf(-6.0f, 2.5f, smoothU);
        s3ShipY = lerpf( 7.2f, 2.6f, smoothU);
        s3ShipZ = lerpf(-8.0f, 3.0f, smoothU);

        s3ShipDirX =  0.60f;
        s3ShipDirY = -0.35f;
        s3ShipDirZ =  0.70f;
        normalize3(s3ShipDirX, s3ShipDirY, s3ShipDirZ);

        s3ShipPitch = lerpf(-24.0f, -10.0f, smoothU);
        s3ShipRoll  = lerpf( 22.0f,  -8.0f, smoothU);
        s3ShipScale = 0.40f;

        // Residual cooling plasma streaks fading out
        if (u < 0.4f && (rand() % 3 == 0)) {
            spawnParticle(s3ShipX, s3ShipY, s3ShipZ,
                          -s3ShipDirX * 6.0f, -s3ShipDirY * 6.0f, -s3ShipDirZ * 6.0f,
                          0.3f, 1.5f, 0.9f, 0.35f, 0.1f, true);
        }
    }
    // PHASE 3: SUPERSONIC PARACHUTE DEPLOYMENT (53.0s - 58.0s)
    else if (t < 58.0f) {
        float u = (t - 53.0f) / 5.0f;
        float smoothU = smoothstep(0.0f, 1.0f, u);

        s3ChuteDeploy = clampf((t - 53.0f) * 2.2f, 0.0f, 1.0f); // Unfurls in 0.45s

        // Rapid aerodynamic deceleration, leveling off descent
        s3ShipX = lerpf( 2.5f, 5.0f, smoothU);
        s3ShipY = lerpf( 2.6f, -0.6f, smoothU); // Glides downward steadily
        s3ShipZ = lerpf( 3.0f, 7.5f, smoothU);

        s3ShipDirX = lerpf(0.60f, 0.20f, smoothU);
        s3ShipDirY = lerpf(-0.35f, -0.15f, smoothU);
        s3ShipDirZ = lerpf(0.70f, 0.95f, smoothU);
        normalize3(s3ShipDirX, s3ShipDirY, s3ShipDirZ);

        // Stabilizing pitch & roll
        s3ShipPitch = lerpf(-10.0f, 4.0f, smoothU);
        s3ShipRoll  = lerpf(-8.0f,  0.0f, smoothU);
        s3ShipScale = 0.40f;

        // Position parachute canopy high above craft trailing behind
        s3ChuteX = s3ShipX - s3ShipDirX * 1.8f;
        s3ChuteY = s3ShipY + 2.8f + 0.1f * sinf(t * 8.0f);
        s3ChuteZ = s3ShipZ - s3ShipDirZ * 1.8f;
    }
    // PHASE 4: RETRO-THRUSTERS & TOUCHDOWN (58.0s - 64.0s)
    else {
        float u = (t - 58.0f) / 5.5f;
        u = clampf(u, 0.0f, 1.0f);
        float landEase = smoothstep(0.0f, 1.0f, u);

        s3ChuteCut = true;
        // Jettisoned parachute drifts away on Martian wind
        s3ChuteX += 1.8f * dt;
        s3ChuteY += 0.8f * dt;
        s3ChuteZ -= 2.2f * dt;

        // Lander vertical settle down onto crater plateau (target landing Y = -2.40f)
        s3ShipX = lerpf( 5.0f,  5.8f, landEase);
        s3ShipY = lerpf(-0.6f, -2.35f, landEase);
        s3ShipZ = lerpf( 7.5f,  9.2f, landEase);

        s3ShipPitch = lerpf(4.0f, 0.0f, landEase);
        s3ShipRoll  = 0.0f;
        s3ShipDirX = 0.10f; s3ShipDirY = -0.05f; s3ShipDirZ = 0.99f;
        normalize3(s3ShipDirX, s3ShipDirY, s3ShipDirZ);

        // Ventral retro thrusters pulsing down before touchdown (58.0s - 61.5s)
        if (t >= 58.0f && t <= 61.5f) {
            for (int k = 0; k < 2; ++k) {
                float ox = randRange(-0.35f, 0.35f);
                float oz = randRange(-0.35f, 0.35f);
                // Fiery cyan/blue retro-exhaust firing downward into ground
                spawnParticle(s3ShipX + ox, s3ShipY - 0.2f, s3ShipZ + oz,
                              randRange(-0.3f, 0.3f), -5.5f, randRange(-0.3f, 0.3f),
                              0.18f, 1.2f, 0.2f, 0.8f, 1.0f, true);
            }
        }

        // Touchdown point contact at 61.2s: kick up radial dust plumes
        if (t >= 61.0f && t <= 63.5f) {
            for (int d = 0; d < 3; ++d) {
                float ang = randRange(0.0f, 2.0f * (float)M_PI);
                float spd = randRange(1.8f, 4.2f);
                TouchdownDust p;
                p.x = s3ShipX + cosf(ang) * 0.4f;
                p.y = -2.38f;
                p.z = s3ShipZ + sinf(ang) * 0.4f;
                p.vx = cosf(ang) * spd;
                p.vy = randRange(0.4f, 1.6f);
                p.vz = sinf(ang) * spd;
                p.scale = randRange(0.4f, 0.9f);
                p.alpha = 0.85f;
                p.maxLife = randRange(1.8f, 2.8f);
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
        s3Dust[i].vy -= 0.6f * dt; // Martian gravity
        s3Dust[i].scale += 0.8f * dt; // Expands outward
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

    // PHASE 1: HYPERSONIC CHASE CAM (44.0s - 48.0s)
    if (t < 48.0f) {
        float u = (t - 44.0f) / 4.0f;
        eyeX = s3ShipX - s3ShipDirX * 3.8f + 1.2f;
        eyeY = s3ShipY - s3ShipDirY * 3.8f + 0.9f;
        eyeZ = s3ShipZ - s3ShipDirZ * 3.8f + 1.6f;

        targetX = s3ShipX + s3ShipDirX * 1.5f;
        targetY = s3ShipY + s3ShipDirY * 1.5f;
        targetZ = s3ShipZ + s3ShipDirZ * 1.5f;

        // Supersonic entry turbulence shake
        float shakeFreq = t * 45.0f;
        float shakeAmp = (1.0f - u * 0.4f) * 0.045f;
        eyeX += sinf(shakeFreq * 1.1f) * shakeAmp;
        eyeY += cosf(shakeFreq * 0.9f) * shakeAmp;
        eyeZ += sinf(shakeFreq * 1.3f) * shakeAmp;
    }
    // PHASE 2: SWEEPING CANYON REVEAL (48.0s - 53.0s)
    else if (t < 53.0f) {
        float u = (t - 48.0f) / 5.0f;
        float smoothU = smoothstep(0.0f, 1.0f, u);

        // Wide panoramic view framing the mountains and the descending ship
        eyeX = lerpf(-12.0f, -4.0f, smoothU);
        eyeY = lerpf(  9.5f,  5.8f, smoothU);
        eyeZ = lerpf( -4.0f,  6.0f, smoothU);

        targetX = s3ShipX;
        targetY = s3ShipY;
        targetZ = s3ShipZ;
    }
    // PHASE 3: PARACHUTE DEPLOYMENT TRACK (53.0s - 58.0s)
    else if (t < 58.0f) {
        float u = (t - 53.0f) / 5.0f;
        float smoothU = smoothstep(0.0f, 1.0f, u);

        // Low angle looking upward past the craft toward the blossoming parachute
        eyeX = lerpf(s3ShipX + 3.2f, s3ShipX + 2.5f, smoothU);
        eyeY = lerpf(s3ShipY - 1.2f, s3ShipY - 0.9f, smoothU);
        eyeZ = lerpf(s3ShipZ - 3.5f, s3ShipZ - 3.0f, smoothU);

        targetX = s3ShipX;
        targetY = s3ShipY + 1.4f; // Look up toward canopy
        targetZ = s3ShipZ;

        // Snapping jolt upon parachute opening at 53.1s - 53.6s
        if (t >= 53.1f && t <= 53.6f) {
            float snap = sinf((t - 53.1f) * 40.0f) * 0.035f;
            eyeY += snap;
        }
    }
    // PHASE 4: GROUND-LEVEL CINEMATIC TOUCHDOWN (58.0s - 64.0s)
    else {
        float u = (t - 58.0f) / 6.0f;
        float smoothU = smoothstep(0.0f, 1.0f, u);

        // Tripod angle sitting on the red plateau foreground
        eyeX = lerpf( 2.2f,  2.8f, smoothU);
        eyeY = -2.15f; // Just above ground level
        eyeZ = lerpf( 5.5f,  6.2f, smoothU);

        targetX = lerpf( 5.0f,  5.8f, smoothU);
        targetY = -2.25f;
        targetZ = lerpf( 7.5f,  9.2f, smoothU);

        // Subtle camera shudder when lander impacts at 61.2s - 61.8s
        if (t >= 61.2f && t <= 61.8f) {
            float impact = sinf((t - 61.2f) * 28.0f) * 0.022f * (1.0f - (t - 61.2f) / 0.6f);
            eyeY += impact;
        }
    }
}

// ============================================================================
// SCENE 3 RENDERING
// ============================================================================
static void renderAtmosphericSkybox() {
    glDisable(GL_LIGHTING);
    glPushMatrix();
    glLoadIdentity();

    // Deep Martian gradient sky: crimson dusk at horizon, purple-black at zenith
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, 1, 0, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);

    glBegin(GL_QUADS);
        // Horizon: rich Martian butterscotch / salmon pink
        glColor3f(0.68f, 0.36f, 0.24f);
        glVertex2f(0.0f, 0.0f);
        glVertex2f(1.0f, 0.0f);
        // Zenith: deep cosmic rust-purple
        glColor3f(0.18f, 0.08f, 0.12f);
        glVertex2f(1.0f, 1.0f);
        glVertex2f(0.0f, 1.0f);
    glEnd();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glEnable(GL_LIGHTING);
}

static void renderAtmosphericCloudDeck(float shipY) {
    if (seqTime > 51.5f || !cloudTexID) return;

    float cloudAlpha = clampf((shipY - 4.0f) / 6.0f, 0.0f, 0.70f);
    if (cloudAlpha <= 0.01f) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, cloudTexID);
    glDisable(GL_LIGHTING);

    glColor4f(0.85f, 0.48f, 0.32f, cloudAlpha);

    // Billowing atmospheric layers whipping past the camera
    float t = seqTime;
    for (int i = 0; i < 6; ++i) {
        float cz = s3ShipZ + (float)i * 6.0f - 12.0f;
        float cy = 9.0f + (float)i * 1.5f;
        glPushMatrix();
        glTranslatef(s3ShipX + sinf(t + i) * 2.0f, cy, cz);
        glScalef(24.0f, 8.0f, 24.0f);
        glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, 0.0f, -1.0f);
            glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, 0.0f, -1.0f);
            glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f, 0.0f,  1.0f);
            glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f, 0.0f,  1.0f);
        glEnd();
        glPopMatrix();
    }

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
    float pScale = 0.45f * s3ChuteDeploy;
    glScalef(pScale, pScale, pScale);
    glCallList(parachuteDisplayList);
    glPopMatrix();

    // Shroud lines connecting parachute skirt to ship hardpoints (only before cut)
    if (!s3ChuteCut && s3ChuteDeploy > 0.5f) {
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glLineWidth(1.8f);
        glColor4f(0.92f, 0.90f, 0.88f, 0.75f);

        float canopyBottomY = s3ChuteY - 0.2f;
        glBegin(GL_LINES);
            // 4 Corner suspension cords
            glVertex3f(s3ChuteX - 0.8f, canopyBottomY, s3ChuteZ - 0.8f);
            glVertex3f(s3ShipX - 0.2f, s3ShipY + 0.3f, s3ShipZ - 0.2f);

            glVertex3f(s3ChuteX + 0.8f, canopyBottomY, s3ChuteZ - 0.8f);
            glVertex3f(s3ShipX + 0.2f, s3ShipY + 0.3f, s3ShipZ - 0.2f);

            glVertex3f(s3ChuteX + 0.8f, canopyBottomY, s3ChuteZ + 0.8f);
            glVertex3f(s3ShipX + 0.2f, s3ShipY + 0.3f, s3ShipZ + 0.2f);

            glVertex3f(s3ChuteX - 0.8f, canopyBottomY, s3ChuteZ + 0.8f);
            glVertex3f(s3ShipX - 0.2f, s3ShipY + 0.3f, s3ShipZ + 0.2f);
        glEnd();

        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
    }
}

void renderScene3() {
    // 1. Martian Atmospheric Gradient Sky
    renderAtmosphericSkybox();

    // 2. High-Altitude Cloud Deck
    renderAtmosphericCloudDeck(s3ShipY);

    // 3. Official NASA Gale Crater 3D Topography Mesh
    if (craterLoaded && craterDisplayList != 0) {
        glPushMatrix();
        // Position Gale Crater terrain floor
        glTranslatef(0.0f, -4.5f, 0.0f);

        GLfloat matDiff[] = { 1.25f, 1.20f, 1.15f, 1.0f };
        GLfloat matAmb[]  = { 0.45f, 0.40f, 0.38f, 1.0f };
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, matDiff);
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, matAmb);

        glCallList(craterDisplayList);
        glPopMatrix();
    }

    // 4. Parachute Canopy & Suspension Lines
    renderParachuteWithLines();

    // 5. Star Sparrow / Ranger Lander Craft
    glPushMatrix();
    glTranslatef(s3ShipX, s3ShipY, s3ShipZ);
    glScalef(s3ShipScale, s3ShipScale, s3ShipScale);

    // Heading orientation
    float yaw = atan2f(s3ShipDirX, s3ShipDirZ) * 180.0f / (float)M_PI;
    glRotatef(yaw, 0.0f, 1.0f, 0.0f);
    glRotatef(s3ShipPitch, 1.0f, 0.0f, 0.0f);
    glRotatef(s3ShipRoll, 0.0f, 0.0f, 1.0f);

    renderRangerLander();
    glPopMatrix();

    // 6. Volumetric Touchdown Dust Clouds
    renderTouchdownDustParticles();
}
