#ifndef COMMON_H
#define COMMON_H

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>

#ifndef FREEGLUT_STATIC
#define FREEGLUT_STATIC
#endif
#include <GL/freeglut.h>

#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <vector>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// CONSTANTS
// ============================================================================
const int   WINDOW_INIT_WIDTH   = 1280;
const int   WINDOW_INIT_HEIGHT  = 720;
const float MARS_RADIUS         = 2.20f;
const int   MARS_SLICES         = 112;
const int   MARS_STACKS         = 72;
const int   NUM_STARS           = 4200;
const int   NUM_DUST_STREAKS    = 90;
const float SEQUENCE_DURATION   = 69.0f; // Scene 1 (35s) + Scene 2 (9s) + Scene 3 (25s)

// ============================================================================
// DATA STRUCTURES
// ============================================================================
struct Star {
    float x, y, z;
    float r, g, b;
    float brightness;
    float size;
    float twinkleSpeed;
    float phase;
};

struct Particle {
    float x, y, z;
    float vx, vy, vz;
    float life;
    float maxLife;
    float size;
    float r, g, b;
    bool  additive;
};

struct DustStreak {
    float x, y, z;
    float speed;
    float length;
    float alpha;
};

// ============================================================================
// GLOBAL STATE (EXTERNS)
// ============================================================================
extern int windowWidth;
extern int windowHeight;

extern GLuint marsTextureID;
extern bool   hasTexture;

extern std::vector<Star>       stars;
extern std::vector<Particle>   particles;
extern std::vector<DustStreak> dustStreaks;

extern float seqTime;
extern float totalTime;
extern float lastClockTime;
extern bool  isPaused;

extern float marsSpinAngle;
extern float phobosAngle;

extern float enduranceOrbitT;
extern float ringSpinAngle;
extern float endPosX, endPosY, endPosZ;
extern float endTanX, endTanY, endTanZ;

extern float rangerPosX, rangerPosY, rangerPosZ;
extern float rangerDirX, rangerDirY, rangerDirZ;
extern float rangerPitchExtra;
extern float rangerRollAngle;
extern float heatshieldThermal;
extern float rcsActive;
extern float mainBurnActive;
extern bool  rangerDetached;

extern const float SUN_DIR_X;
extern const float SUN_DIR_Y;
extern const float SUN_DIR_Z;
extern GLfloat lightSunPos[4];
extern GLfloat lightSunAmbient[4];
extern GLfloat lightSunDiffuse[4];
extern GLfloat lightSunSpecular[4];

// ============================================================================
// INLINE MATH UTILITIES
// ============================================================================
inline float clampf(float v, float lo, float hi) {
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

inline float smoothstep(float edge0, float edge1, float x) {
    float t = clampf((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

inline float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

inline void normalize3(float& x, float& y, float& z) {
    float len = sqrtf(x * x + y * y + z * z);
    if (len > 1e-6f) {
        x /= len; y /= len; z /= len;
    }
}

inline float randRange(float lo, float hi) {
    return lo + ((float)rand() / (float)RAND_MAX) * (hi - lo);
}

inline void drawBox(float sx, float sy, float sz) {
    glPushMatrix();
    glScalef(sx, sy, sz);
    glutSolidCube(1.0);
    glPopMatrix();
}

#endif // COMMON_H

