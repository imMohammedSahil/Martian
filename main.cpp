/*
 * ============================================================================
 *  MARTIAN — CINEMATIC INTERSTELLAR SPACE SEQUENCE
 * ============================================================================
 *  Modular Architecture:
 *  - Common.h        : Shared types, math, and global state
 *  - AudioEngine     : In-memory Hans Zimmer ambient audio synthesizer
 *  - Starfield       : Stellar skybox & star twinkling
 *  - Planet          : Mars 4K/8K sphere, dual-layer atmosphere, Phobos
 *  - Spacecraft      : Endurance ring ship, Ranger lander, particles
 *  - PostProcess     : Anamorphic lens flare, 2.39:1 letterbox, title cards
 *  - SceneDirector   : Unified timeline director routing to Scene 1..5
 *  - Scene1.cpp      : The Cosmic Void & Interstellar Scale Reveal
 * ============================================================================
 */

#include "Common.h"
#include "AudioEngine.h"
#include "Starfield.h"
#include "Planet.h"
#include "Spacecraft.h"
#include "PostProcess.h"
#include "SceneDirector.h"

// ============================================================================
// GLOBAL STATE DEFINITIONS
// ============================================================================
int windowWidth  = WINDOW_INIT_WIDTH;
int windowHeight = WINDOW_INIT_HEIGHT;

float seqTime          = 0.0f;
float totalTime        = 0.0f;
float lastClockTime    = 0.0f;
bool  isPaused         = false;

// Directional Sunlight shining directly onto the camera-facing surface of Mars
const float SUN_DIR_X = -0.38f;
const float SUN_DIR_Y =  0.22f;
const float SUN_DIR_Z =  0.90f;
GLfloat lightSunPos[4]      = { -18.0f, 12.0f, 44.0f, 0.0f }; // Intense directional light directly illuminating Mars
GLfloat lightSunAmbient[4]  = { 0.25f, 0.25f, 0.25f, 1.0f };
GLfloat lightSunDiffuse[4]  = { 2.10f, 1.95f, 1.80f, 1.0f };
GLfloat lightSunSpecular[4] = { 0.55f, 0.52f, 0.48f, 1.0f };

// ============================================================================
// MAIN RENDER LOOP
// ============================================================================
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float eyeX, eyeY, eyeZ;
    float targetX, targetY, targetZ;
    float upX, upY, upZ;
    computeCinematicCamera(eyeX, eyeY, eyeZ, targetX, targetY, targetZ, upX, upY, upZ);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(eyeX, eyeY, eyeZ, targetX, targetY, targetZ, upX, upY, upZ);

    glLightfv(GL_LIGHT0, GL_POSITION, lightSunPos);

    // Secondary Warm Martian Planet-Bounce Fill Light
    GLfloat lightMarsPos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glLightfv(GL_LIGHT1, GL_POSITION, lightMarsPos);

    float flareX = 0.0f, flareY = 0.0f, flareZ = 0.0f;
    bool flareInFront = false;
    float sunVisibility = 0.0f;

    if (seqTime < 44.0f) {
        // SCENES 1 & 2: DEEP SPACE & MARS ORBIT
        // 1. Stellar Skybox
        renderStarfield();

        // 2. Solar Disc & Flares (only after opening void)
        if (seqTime >= 6.0f) {
            renderSunDisc3D(flareX, flareY, flareZ, flareInFront);
            sunVisibility = computeSunMarsOcclusion(eyeX, eyeY, eyeZ);
        }

        // 3. Mars & Atmosphere (reveals smoothly at 6.0s)
        if (seqTime >= 5.8f) {
            renderMarsAndAtmosphere();
            if (seqTime < 39.0f) {
                renderEnduranceShip();
            }
            if (rangerDetached && seqTime >= 39.0f) {
                renderRangerLander();
            }
        }

        // 4. Particles & Speed Streaks
        renderParticles();
        if (seqTime > 41.0f) {
            renderSpeedDustStreaks(eyeX, eyeY, eyeZ);
        }
    } else {
        // SCENE 3: ATMOSPHERIC ENTRY, CLOUD BREAK & CANYON TOUCHDOWN
        renderScene3();
        renderParticles();
    }

    // 5. 2D Post: Anamorphic Lens Flare, 2.39:1 Letterbox Bars, Title & Fades

    // 5. 2D Post: Anamorphic Lens Flare, 2.39:1 Letterbox Bars, Title & Fades
    renderCinemaPost(flareX, flareY, flareInFront, sunVisibility);

    glutSwapBuffers();
}

// ============================================================================
// SIMULATION UPDATE & CLOCK
// ============================================================================
void timer(int value) {
    int currentMs = glutGet(GLUT_ELAPSED_TIME);
    float dt = (currentMs / 1000.0f) - lastClockTime;
    if (dt > 0.05f) dt = 0.05f;
    if (dt < 0.0f)  dt = 0.016f;
    lastClockTime = currentMs / 1000.0f;

    if (!isPaused) {
        totalTime += dt;
        float prevSeq = seqTime;
        seqTime += dt;
        if (seqTime >= SEQUENCE_DURATION) {
            seqTime -= SEQUENCE_DURATION;
            particles.clear();
        } else if (prevSeq > seqTime) {
            particles.clear();
        }
        updateSceneDirector(dt);
    }

    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

void reshape(int w, int h) {
    windowWidth  = (w <= 0) ? 1 : w;
    windowHeight = (h <= 0) ? 1 : h;

    glViewport(0, 0, windowWidth, windowHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(44.0, (GLdouble)windowWidth / (GLdouble)windowHeight, 0.08, 350.0);
    glMatrixMode(GL_MODELVIEW);
}

void keyboard(unsigned char key, int x, int y) {
    if (key == 27) { // ESC: Exit Theater
        PlaySoundA(NULL, NULL, 0);
        glutLeaveMainLoop();
    } else if (key == ' ') { // Freeze-Frame Pause
        isPaused = !isPaused;
    } else if (key == 'r' || key == 'R') { // Replay Scene from start
        seqTime = 0.0f;
    } else if (key == '1') {
        seqTime = 0.0f;
        particles.clear();
    } else if (key == '2') {
        seqTime = 35.0f;
        particles.clear();
    } else if (key == '3') {
        seqTime = 44.0f;
        particles.clear();
    } else if (key == 'f' || key == 'F' || key == '.' || key == '>') { // Fast-Forward 5s
        seqTime = clampf(seqTime + 5.0f, 0.0f, SEQUENCE_DURATION - 0.1f);
    } else if (key == 'b' || key == 'B' || key == ',' || key == '<') { // Rewind 5s
        seqTime = clampf(seqTime - 5.0f, 0.0f, SEQUENCE_DURATION - 0.1f);
    }
}

void specialKeys(int key, int x, int y) {
    if (key == GLUT_KEY_RIGHT) { // Arrow Right: Forward 5s
        seqTime = clampf(seqTime + 5.0f, 0.0f, SEQUENCE_DURATION - 0.1f);
    } else if (key == GLUT_KEY_LEFT) { // Arrow Left: Backward 5s
        seqTime = clampf(seqTime - 5.0f, 0.0f, SEQUENCE_DURATION - 0.1f);
    }
}

// ============================================================================
// INITIALIZATION
// ============================================================================
void initOpenGL() {
    glClearColor(0.004f, 0.006f, 0.012f, 1.0f); // Pitch-black space void
    glClearDepth(1.0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_NORMALIZE);

    // Primary Solar Lighting
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  lightSunAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  lightSunDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSunSpecular);

    // Secondary Warm Martian Planet-Bounce Light
    glEnable(GL_LIGHT1);
    GLfloat lightMarsAmbient[4]  = { 0.0f, 0.0f, 0.0f, 1.0f };
    GLfloat lightMarsDiffuse[4]  = { 0.42f, 0.17f, 0.08f, 1.0f };
    GLfloat lightMarsSpecular[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glLightfv(GL_LIGHT1, GL_AMBIENT,  lightMarsAmbient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE,  lightMarsDiffuse);
    glLightfv(GL_LIGHT1, GL_SPECULAR, lightMarsSpecular);

    // Dust particles
    dustStreaks.resize(NUM_DUST_STREAKS);
    for (int i = 0; i < NUM_DUST_STREAKS; ++i) {
        dustStreaks[i].x = randRange(-1.6f, 1.6f);
        dustStreaks[i].y = randRange(-1.2f, 1.2f);
        dustStreaks[i].z = randRange(-2.2f, 2.2f);
        dustStreaks[i].speed  = randRange(3.5f, 9.0f);
        dustStreaks[i].length = randRange(0.08f, 0.28f);
        dustStreaks[i].alpha  = randRange(0.35f, 0.85f);
    }
}

// ============================================================================
// MAIN ENTRY
// ============================================================================
int main(int argc, char** argv) {
    // Prevent Windows DPI scaling from pushing the bottom of the window below the monitor
    typedef BOOL (WINAPI *SetProcessDPIAwareFn)();
    HMODULE hUser32 = LoadLibraryA("user32.dll");
    if (hUser32) {
        SetProcessDPIAwareFn setDPIAware = (SetProcessDPIAwareFn)GetProcAddress(hUser32, "SetProcessDPIAware");
        if (setDPIAware) setDPIAware();
        FreeLibrary(hUser32);
    }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);

    int scrW = GetSystemMetrics(SM_CXSCREEN);
    int scrH = GetSystemMetrics(SM_CYSCREEN);
    if (scrW <= 0) scrW = WINDOW_INIT_WIDTH;
    if (scrH <= 0) scrH = WINDOW_INIT_HEIGHT;

    glutInitWindowSize(scrW, scrH);
    glutInitWindowPosition(0, 0);
    glutCreateWindow("MARTIAN");
    glutFullScreen();

    glutSetCursor(GLUT_CURSOR_NONE);

    initOpenGL();
    loadMarsTexture();
    initSolarAssets();
    initStarfield();
    initAndPlaySpaceAmbience();

    lastClockTime = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutTimerFunc(16, timer, 0);

    glutMainLoop();
    return 0;
}
