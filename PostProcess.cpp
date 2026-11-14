#include "PostProcess.h"
#include "Common.h"
#include "stb_image.h"

static GLuint sunTextureID = 0;
static bool   hasSunTexture = false;
static GLuint titleTextureID = 0;
static bool   hasTitleTexture = false;
static bool   titleLoadAttempted = false;

static void loadTitleTexture() {
    if (titleLoadAttempted) return;
    titleLoadAttempted = true;

    const char* paths[] = {
        "martian_title.png",
        "../martian_title.png",
        "bin/Release/martian_title.png",
        "bin/Debug/martian_title.png",
        "C:/Holidays/Martian/martian_title.png"
    };
    unsigned char* data = NULL;
    int w = 0, h = 0, channels = 0;
    for (int i = 0; i < 5; ++i) {
        data = stbi_load(paths[i], &w, &h, &channels, 4);
        if (data) break;
    }
    if (!data) return;

    glGenTextures(1, &titleTextureID);
    glBindTexture(GL_TEXTURE_2D, titleTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
    hasTitleTexture = true;
}

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

    // 3. Panel 4 Storyboard Title Card: "M A R T I A N" (25.0s - 34.5s)
    // Mars background fully covers the screen and locks at 22.0s, allowing exactly 3.0s before title appears
    if (seqTime >= 25.0f && seqTime <= 34.5f) {
        float titleAlpha = smoothstep(25.0f, 26.2f, seqTime) * (1.0f - smoothstep(33.0f, 34.5f, seqTime));
        if (titleAlpha > 0.01f) {
            loadTitleTexture();
            if (hasTitleTexture && titleTextureID) {
                // High-resolution geometric sans-serif title (Century Gothic Bold - 78% screen width)
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, titleTextureID);

                // Colossal, bold cinematic proportion (covers 92% of screen width)
                float titleW = (float)windowWidth * 0.92f;
                float titleH = titleW * (640.0f / 2560.0f);
                float cx = (float)windowWidth * 0.5f;
                float cy = (float)windowHeight * 0.5f;

                float x0 = cx - titleW * 0.5f;
                float x1 = cx + titleW * 0.5f;
                float y0 = cy - titleH * 0.5f;
                float y1 = cy + titleH * 0.5f;

                glColor4f(1.0f, 1.0f, 1.0f, titleAlpha);
                glBegin(GL_QUADS);
                    glTexCoord2f(0.0f, 1.0f); glVertex2f(x0, y0);
                    glTexCoord2f(1.0f, 1.0f); glVertex2f(x1, y0);
                    glTexCoord2f(1.0f, 0.0f); glVertex2f(x1, y1);
                    glTexCoord2f(0.0f, 0.0f); glVertex2f(x0, y1);
                glEnd();

                glDisable(GL_TEXTURE_2D);
                glDisable(GL_BLEND);
            } else {
                // Vector fallback
                const char* title = "M   A   R   T   I   A   N";
                float scale = (windowWidth / 1280.0f) * 0.42f;
                float strWidth = 0.0f;
                for (const char* p = title; *p; ++p) {
                    if (*p == ' ') strWidth += 110.0f;
                    else strWidth += (float)glutStrokeWidth(GLUT_STROKE_ROMAN, *p);
                }
                float startX = (windowWidth - strWidth * scale) * 0.5f;
                float startY = windowHeight * 0.5f - 14.0f * scale;

                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glLineWidth(4.0f);
                glPushMatrix();
                glTranslatef(startX, startY, 0.0f);
                glScalef(scale, scale, 1.0f);
                glColor4f(0.05f, 0.05f, 0.05f, titleAlpha);
                for (const char* p = title; *p; ++p) {
                    glutStrokeCharacter(GLUT_STROKE_ROMAN, *p);
                }
                glPopMatrix();
                glDisable(GL_BLEND);
            }
        }
    }

    // 4. Theatrical Transitions & Fades
    float fadeAlpha = 0.0f;
    if (seqTime < 1.5f) {
        fadeAlpha = 1.0f - smoothstep(0.0f, 1.5f, seqTime);
    }
    // Sudden screen black out between Shot 1 and Shot 2 (38.8s - 39.3s)
    if (seqTime >= 38.8f && seqTime <= 39.3f) {
        if (seqTime <= 39.0f) {
            fadeAlpha = std::max(fadeAlpha, smoothstep(38.8f, 39.0f, seqTime));
        } else {
            fadeAlpha = std::max(fadeAlpha, 1.0f - smoothstep(39.0f, 39.3f, seqTime));
        }
    }
    // Sudden cut to black at end of Scene 2 (43.70s - 44.0s) and clean reveal of Scene 3
    if (seqTime >= 43.70f && seqTime < 44.60f) {
        if (seqTime < 44.0f) {
            fadeAlpha = 1.0f;
        } else {
            fadeAlpha = 1.0f - smoothstep(44.0f, 44.50f, seqTime);
        }
    }
    // Scene 3 finale cut to black (63.5s - 64.0s)
    if (seqTime >= 63.50f) {
        fadeAlpha = smoothstep(63.50f, 63.80f, seqTime);
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

