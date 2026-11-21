#include "PostProcess.h"
#include "Common.h"
#include "Scene4.h"
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

    // Ensure typography RGB is pure brilliant white so alpha drives the cinematic glyph silhouette
    for (int i = 0; i < w * h; ++i) {
        data[i * 4 + 0] = 255;
        data[i * 4 + 1] = 255;
        data[i * 4 + 2] = 255;
    }

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

static void renderOpeningSolarFlare(float intensity) {
    if (intensity <= 0.005f) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive optical blending

    // Flare origin sweeps subtly in upper frame with camera pan
    float tNorm = clampf(seqTime / 4.8f, 0.0f, 1.0f);
    float winX = (float)windowWidth * (0.22f + 0.10f * tNorm);
    float winY = (float)windowHeight * (0.76f - 0.06f * tNorm);

    float cx = (float)windowWidth * 0.5f;
    float cy = (float)windowHeight * 0.5f;
    float dx = winX - cx;
    float dy = winY - cy;

    // 1. Fullscreen Subtle Glare Veil (illuminates the deep space void during flare)
    float barH = (windowHeight - ((float)windowWidth / 2.39f)) * 0.5f;
    if (barH < windowHeight * 0.135f) barH = windowHeight * 0.135f;

    glBegin(GL_QUADS);
        glColor4f(0.06f, 0.10f, 0.16f, 0.18f * intensity);
        glVertex2f(0.0f, barH);
        glVertex2f((float)windowWidth, barH);
        glVertex2f((float)windowWidth, (float)windowHeight - barH);
        glVertex2f(0.0f, (float)windowHeight - barH);
    glEnd();

    // 2. Sweeping Anamorphic Cyan Beam across widescreen
    float beamH = 4.2f * (0.8f + 0.4f * intensity);
    float beamAlpha = 0.68f * intensity;
    glBegin(GL_QUADS);
        glColor4f(0.35f, 0.72f, 1.00f, beamAlpha);
        glVertex2f(0.0f, winY - beamH);
        glVertex2f((float)windowWidth, winY - beamH);
        glVertex2f((float)windowWidth, winY + beamH);
        glVertex2f(0.0f, winY + beamH);
    glEnd();

    // 3. Razor-Sharp Intense Golden-White Core Anamorphic Streak
    float coreH = 1.4f * (0.8f + 0.3f * intensity);
    float coreAlpha = 0.95f * intensity;
    glBegin(GL_QUADS);
        glColor4f(1.00f, 0.98f, 0.90f, coreAlpha);
        glVertex2f(0.0f, winY - coreH);
        glVertex2f((float)windowWidth, winY - coreH);
        glVertex2f((float)windowWidth, winY + coreH);
        glVertex2f(0.0f, winY + coreH);
    glEnd();

    // 4. Brilliant Solar Coronal Halo & Core Bloom
    const int segments = 36;
    float bloomR = 76.0f * (0.8f + 0.5f * intensity);
    glBegin(GL_TRIANGLE_FAN);
        glColor4f(1.00f, 0.99f, 0.94f, 0.95f * intensity);
        glVertex2f(winX, winY);
        glColor4f(0.40f, 0.75f, 1.00f, 0.0f);
        for (int i = 0; i <= segments; ++i) {
            float ang = 2.0f * (float)M_PI * (float)i / segments;
            glVertex2f(winX + bloomR * cosf(ang), winY + bloomR * sinf(ang));
        }
    glEnd();

    // 5. Starburst Glare Spikes radiating from solar flare center
    const int numSpikes = 6;
    float spikeLength = 260.0f * (0.7f + 0.5f * intensity);
    float spikeWidth  = 3.2f;
    for (int i = 0; i < numSpikes; ++i) {
        float baseAngle = (float)M_PI * (float)i / (float)numSpikes + 0.26f;
        float cosA = cosf(baseAngle);
        float sinA = sinf(baseAngle);
        float perpX = -sinA * spikeWidth;
        float perpY =  cosA * spikeWidth;

        glBegin(GL_TRIANGLES);
            glColor4f(1.00f, 0.96f, 0.85f, 0.65f * intensity);
            glVertex2f(winX - perpX, winY - perpY);
            glVertex2f(winX + perpX, winY + perpY);
            glColor4f(0.35f, 0.70f, 1.00f, 0.0f);
            glVertex2f(winX + cosA * spikeLength, winY + sinA * spikeLength);

            glColor4f(1.00f, 0.96f, 0.85f, 0.65f * intensity);
            glVertex2f(winX - perpX, winY - perpY);
            glVertex2f(winX + perpX, winY + perpY);
            glColor4f(0.35f, 0.70f, 1.00f, 0.0f);
            glVertex2f(winX - cosA * spikeLength, winY - sinA * spikeLength);
        glEnd();
    }

    // 6. Optical Lens Ghosts along the optical axis
    const float ghostOffsets[] = { -0.28f, -0.62f, 0.35f, 0.75f, 1.10f };
    const float ghostSizes[]   = {  22.0f,  38.0f, 16.0f, 32.0f, 18.0f };
    const float ghostAlphas[]  = {  0.25f,  0.16f, 0.22f, 0.14f, 0.10f };

    for (int g = 0; g < 5; ++g) {
        float gx = cx + dx * ghostOffsets[g];
        float gy = cy + dy * ghostOffsets[g];
        float gr = ghostSizes[g] * (0.8f + 0.3f * intensity);
        float ga = ghostAlphas[g] * intensity;

        glBegin(GL_TRIANGLE_FAN);
            if (g % 2 == 0) {
                glColor4f(0.30f, 0.72f, 1.00f, ga);
            } else {
                glColor4f(1.00f, 0.70f, 0.35f, ga);
            }
            glVertex2f(gx, gy);
            glColor4f(0.20f, 0.40f, 0.80f, 0.0f);
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

    // 1. Opening Scene 1 Solar Flare & Glare Event (0.0s - 5.0s)
    float openingFlare = getOpeningFlareIntensity();
    if (openingFlare > 0.005f) {
        renderOpeningSolarFlare(openingFlare);
    }

    // 2. Anamorphic Lens Flare (for Mars orbit sun reveal)
    if (flareInFront) {
        renderAnamorphicFlare2D(flareX, flareY, flareVisibility);
    }

    // 2. 2.39:1 Cinema Letterbox Bar Dimensions
    float barH = (windowHeight - ((float)windowWidth / 2.39f)) * 0.5f;
    if (barH < windowHeight * 0.135f) barH = windowHeight * 0.135f;

    // 3. Theatrical Transitions & Fades
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
    // Scene 3 into Scene 4 Smooth Filmic Transition (64.2s - 65.8s)
    // Seamlessly dissolves the high-speed canyon dive into the dark cabin silhouette
    if (seqTime >= 64.20f && seqTime < 65.80f) {
        if (seqTime < 65.00f) {
            fadeAlpha = smoothstep(64.20f, 65.00f, seqTime);
        } else {
            fadeAlpha = 1.0f - smoothstep(65.00f, 65.80f, seqTime);
        }
    }
    // Scene 4 final theatrical CUT TO BLACK (controlled interactively by g_cutToBlackTime)
    if (seqTime >= g_cutToBlackTime) {
        fadeAlpha = 1.0f;
    }

    // Scene 4 Optical Exposure Bloom when ramp lowers and daylight floods in (67.5s - 70.5s)
    if (seqTime >= 67.5f && seqTime <= 70.5f) {
        float bloom = 0.0f;
        float maxBloom = getScene4BloomStrength();
        if (seqTime <= 68.3f) {
            bloom = smoothstep(67.5f, 68.3f, seqTime) * maxBloom;
        } else {
            bloom = (1.0f - smoothstep(68.3f, 70.5f, seqTime)) * maxBloom;
        }
        if (bloom > 0.005f) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive warm solar bloom
            glColor4f(1.0f, 0.94f, 0.82f, bloom);
            glRectf(0.0f, barH, (float)windowWidth, (float)windowHeight - barH);
            glDisable(GL_BLEND);
        }
    }

    // Fullscreen Fade / Cut-to-Black Quad
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

    // 4. Storyboard Title Card: "M A R T I A N"
    // - Scene 1 opening: 27.0s - 34.5s (locked frame-accurately to the 27.0s organ chord drop)
    // - Scene 4 finale: after scene ends and cuts to complete black, title reveals smoothly on black in dead silence
    float titleAlpha = 0.0f;
    if (seqTime >= 27.0f && seqTime <= 34.5f) {
        titleAlpha = smoothstep(27.0f, 27.6f, seqTime) * (1.0f - smoothstep(33.2f, 34.5f, seqTime));
    } else if (seqTime >= g_titleRevealTime && seqTime <= (g_titleRevealTime + 5.5f)) {
        titleAlpha = smoothstep(g_titleRevealTime, g_titleRevealTime + 0.8f, seqTime) * 
                     (1.0f - smoothstep(g_titleRevealTime + 4.2f, g_titleRevealTime + 5.5f, seqTime));
    }

    if (titleAlpha > 0.01f) {
        loadTitleTexture();
        if (hasTitleTexture && titleTextureID) {
            // High-resolution geometric sans-serif title (Century Gothic Bold - 92% screen width)
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, titleTextureID);

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
            // Vector fallback (rendered in clean pure white)
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
            glColor4f(1.0f, 1.0f, 1.0f, titleAlpha);
            for (const char* p = title; *p; ++p) {
                glutStrokeCharacter(GLUT_STROKE_ROMAN, *p);
            }
            glPopMatrix();
            glDisable(GL_BLEND);
        }
    }

    // 5. Master 2.39:1 Widescreen Cinema Letterbox Bars (Top & Bottom)
    // Rendered as the final absolute pass to guarantee 100% solid, impenetrable black bars
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glColor3f(0.0f, 0.0f, 0.0f);
    glRectf(0.0f, 0.0f, (float)windowWidth, barH);
    glRectf(0.0f, (float)windowHeight - barH, (float)windowWidth, (float)windowHeight);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

