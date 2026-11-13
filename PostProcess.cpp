#include "PostProcess.h"
#include "Common.h"
#include "stb_image.h"

static GLuint sunTextureID = 0;
static bool   hasSunTexture = false;

void initSolarAssets() {
    int w, h, channels;
    unsigned char* data = stbi_load("sun_flare.png", &w, &h, &channels, 4);
    if (!data) return;

    glGenTextures(1, &sunTextureID);
    glBindTexture(GL_TEXTURE_2D, sunTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
    hasSunTexture = true;
}

void renderSunDisc3D(float& outWinX, float& outWinY, float& outWinZ, bool& outInFront) {
    glDisable(GL_LIGHTING);
    glPushMatrix();
    glTranslatef(lightSunPos[0], lightSunPos[1], lightSunPos[2]);

    // Blazing pure white solar disc (realistic size for Mars orbit, 1.52 AU)
    glColor3f(1.0f, 0.99f, 0.96f);
    glutSolidSphere(0.85f, 20, 16);

    // Screen-space 2D coordinate extraction
    GLdouble model[16], proj[16];
    GLint view[4];
    glGetDoublev(GL_MODELVIEW_MATRIX, model);
    glGetDoublev(GL_PROJECTION_MATRIX, proj);
    glGetIntegerv(GL_VIEWPORT, view);

    GLdouble wx, wy, wz;
    gluProject(0.0, 0.0, 0.0, model, proj, view, &wx, &wy, &wz);
    outWinX = (float)wx;
    outWinY = (float)wy;
    outWinZ = (float)wz;
    outInFront = (wz >= 0.0 && wz <= 1.0);

    glPopMatrix();
    glEnable(GL_LIGHTING);
}

static void renderAnamorphicFlare2D(float winX, float winY, float visibility) {
    if (visibility <= 0.01f) return;
    // Don't render flare if Sun is far off-screen
    if (winX < -150.0f || winX > (float)windowWidth + 150.0f) return;
    if (winY < -150.0f || winY > (float)windowHeight + 150.0f) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    float cx = (float)windowWidth * 0.5f;
    float cy = (float)windowHeight * 0.5f;
    float dx = winX - cx;
    float dy = winY - cy;

    // 1. Horizontal Anamorphic Blue Streak
    float beamH = 3.6f;
    float beamAlpha = 0.58f * visibility;
    glBegin(GL_QUADS);
        glColor4f(0.42f, 0.76f, 1.00f, beamAlpha);
        glVertex2f(0.0f, winY - beamH);
        glVertex2f((float)windowWidth, winY - beamH);
        glVertex2f((float)windowWidth, winY + beamH);
        glVertex2f(0.0f, winY + beamH);
    glEnd();

    // 2. Razor-Sharp Golden-White Core Beam
    float coreH = 1.2f;
    float coreAlpha = 0.88f * visibility;
    glBegin(GL_QUADS);
        glColor4f(1.00f, 0.96f, 0.88f, coreAlpha);
        glVertex2f(0.0f, winY - coreH);
        glVertex2f((float)windowWidth, winY - coreH);
        glVertex2f((float)windowWidth, winY + coreH);
        glVertex2f(0.0f, winY + coreH);
    glEnd();

    // 3. Radial Solar Bloom Halo
    const int bloomSegments = 32;
    float bloomR = 48.0f * (0.8f + 0.4f * visibility);
    glBegin(GL_TRIANGLE_FAN);
        glColor4f(1.00f, 0.98f, 0.90f, 0.85f * visibility);
        glVertex2f(winX, winY);
        glColor4f(0.55f, 0.80f, 1.00f, 0.0f);
        for (int i = 0; i <= bloomSegments; ++i) {
            float ang = 2.0f * (float)M_PI * (float)i / bloomSegments;
            glVertex2f(winX + bloomR * cosf(ang), winY + bloomR * sinf(ang));
        }
    glEnd();

    // 4. Hexagonal Optical Lens Ghosts along the optical axis
    const float ghostOffsets[] = { -0.35f, -0.70f, 0.40f, 0.85f };
    const float ghostSizes[]   = {  18.0f,  32.0f, 14.0f, 26.0f };
    const float ghostAlphas[]  = {  0.18f,  0.12f, 0.16f, 0.10f };

    for (int g = 0; g < 4; ++g) {
        float gx = cx + dx * ghostOffsets[g];
        float gy = cy + dy * ghostOffsets[g];
        float gr = ghostSizes[g];
        float ga = ghostAlphas[g] * visibility;

        glBegin(GL_TRIANGLE_FAN);
            glColor4f(0.35f, 0.75f, 1.00f, ga);
            glVertex2f(gx, gy);
            glColor4f(0.85f, 0.55f, 0.20f, 0.0f);
            for (int i = 0; i <= 6; ++i) {
                float ang = (float)M_PI / 3.0f * (float)i;
                glVertex2f(gx + gr * cosf(ang), gy + gr * sinf(ang));
            }
        glEnd();
    }

    glDisable(GL_BLEND);
}

void renderCinemaPost(float flareX, float flareY, bool flareInFront, float flareVisibility) {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, windowWidth, 0, windowHeight);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // 1. Anamorphic Lens Flare
    if (flareInFront) {
        renderAnamorphicFlare2D(flareX, flareY, flareVisibility);
    }

    // 2. 2.39:1 Widescreen Cinema Letterbox Bars (Top & Bottom)
    float barH = (windowHeight - ((float)windowWidth / 2.39f)) * 0.5f;
    if (barH < windowHeight * 0.135f) barH = windowHeight * 0.135f;

    glDisable(GL_BLEND);
    glColor3f(0.0f, 0.0f, 0.0f);

    // Bottom Letterbox Bar
    glRectf(0.0f, 0.0f, (float)windowWidth, barH);

    // Top Letterbox Bar
    glRectf(0.0f, (float)windowHeight - barH, (float)windowWidth, (float)windowHeight);

    // 3. Panel 4 Storyboard Title Card: "M A R T I A N" (26.0s - 34.0s)
    if (seqTime >= 26.0f && seqTime <= 34.0f) {
        float titleAlpha = smoothstep(26.0f, 27.5f, seqTime) * (1.0f - smoothstep(32.8f, 34.0f, seqTime));
        if (titleAlpha > 0.01f) {
            const char* title = "M   A   R   T   I   A   N";
            float scale = (windowWidth / 1280.0f) * 0.28f;
            
            // Accurate width calculation (handling spaces correctly in FreeGLUT)
            float strWidth = 0.0f;
            for (const char* p = title; *p; ++p) {
                if (*p == ' ') {
                    strWidth += 110.0f;
                } else {
                    strWidth += (float)glutStrokeWidth(GLUT_STROKE_ROMAN, *p);
                }
            }
            float startX = (windowWidth - strWidth * scale) * 0.5f;
            float startY = windowHeight * 0.5f - 14.0f * scale;

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_LINE_SMOOTH);

            // Glow / Shadow pass (Bold & Cinematic)
            glLineWidth(5.0f);
            glPushMatrix();
            glTranslatef(startX, startY, 0.0f);
            glScalef(scale, scale, 1.0f);
            glColor4f(0.0f, 0.0f, 0.0f, titleAlpha * 0.75f);
            for (const char* p = title; *p; ++p) {
                glutStrokeCharacter(GLUT_STROKE_ROMAN, *p);
            }
            glPopMatrix();

            // Crisp Core pass
            glLineWidth(2.6f);
            glPushMatrix();
            glTranslatef(startX, startY, 0.0f);
            glScalef(scale, scale, 1.0f);
            glColor4f(0.98f, 0.99f, 1.00f, titleAlpha * 0.95f);
            for (const char* p = title; *p; ++p) {
                glutStrokeCharacter(GLUT_STROKE_ROMAN, *p);
            }
            glPopMatrix();

            glDisable(GL_LINE_SMOOTH);
            glDisable(GL_BLEND);
        }
    }

    // 4. Theatrical Transitions & Fades
    float fadeAlpha = 0.0f;
    if (seqTime < 1.5f) {
        fadeAlpha = 1.0f - smoothstep(0.0f, 1.5f, seqTime);
    }
    // Fade at the very end of sequence loop
    if (seqTime > 34.0f) {
        fadeAlpha = std::max(fadeAlpha, smoothstep(34.0f, 35.0f, seqTime));
    }

    if (fadeAlpha > 0.005f) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.0f, 0.0f, 0.0f, fadeAlpha);
        glBegin(GL_QUADS);
            glVertex2f(0.0f, 0.0f);
            glVertex2f((float)windowWidth, 0.0f);
            glVertex2f((float)windowWidth, (float)windowHeight);
            glVertex2f(0.0f, (float)windowHeight);
        glEnd();
        glDisable(GL_BLEND);
    }

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

