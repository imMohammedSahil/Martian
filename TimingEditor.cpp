#include "TimingEditor.h"
#include "Common.h"
#include "AudioEngine.h"
#include <cstdio>
#include <cstring>
#include <algorithm>

float g_cutToBlackTime   = 79.50f;
float g_titleRevealTime  = 80.20f;
bool  g_showTimingHUD    = true;

static char  g_statusMsg[128] = "";
static float g_statusMsgTimer = 0.0f;

void loadTimingConfig() {
    FILE* f = fopen("timing_config.txt", "r");
    if (!f) return;

    char line[128];
    while (fgets(line, sizeof(line), f)) {
        float val = 0.0f;
        if (sscanf(line, "cutToBlack %f", &val) == 1) {
            g_cutToBlackTime = val;
        } else if (sscanf(line, "titleReveal %f", &val) == 1) {
            g_titleRevealTime = val;
        }
    }
    fclose(f);
}

void saveTimingConfig() {
    FILE* f = fopen("timing_config.txt", "w");
    if (!f) return;
    fprintf(f, "cutToBlack %.2f\n", g_cutToBlackTime);
    fprintf(f, "titleReveal %.2f\n", g_titleRevealTime);
    fclose(f);

    snprintf(g_statusMsg, sizeof(g_statusMsg), "SAVED TO timing_config.txt! (cut=%.2fs, title=%.2fs)", g_cutToBlackTime, g_titleRevealTime);
    g_statusMsgTimer = 3.5f;
}

void initTimingEditor() {
    loadTimingConfig();
}

static void drawText(float x, float y, const char* str) {
    glRasterPos2f(x, y);
    for (const char* p = str; *p; ++p) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *p);
    }
}

static void drawTextBold(float x, float y, const char* str) {
    glRasterPos2f(x, y);
    for (const char* p = str; *p; ++p) {
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *p);
    }
}

void renderTimingHUD(int windowWidth, int windowHeight) {
    if (!g_showTimingHUD) return;

    if (g_statusMsgTimer > 0.0f) {
        g_statusMsgTimer -= 0.01667f;
    }

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

    // Box dimensions in top-left
    float boxW = 560.0f;
    float boxH = 155.0f;
    float boxX = 24.0f;
    float boxY = (float)windowHeight - boxH - 24.0f;

    // Dark sleek glass background
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.02f, 0.03f, 0.05f, 0.88f);
    glRectf(boxX, boxY, boxX + boxW, boxY + boxH);

    // Cyan / Gold border
    glColor4f(0.85f, 0.55f, 0.20f, 0.90f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(boxX, boxY);
        glVertex2f(boxX + boxW, boxY);
        glVertex2f(boxX + boxW, boxY + boxH);
        glVertex2f(boxX, boxY + boxH);
    glEnd();

    float tx = boxX + 16.0f;
    float ty = boxY + boxH - 22.0f;

    // Title
    glColor3f(1.0f, 0.75f, 0.25f);
    drawTextBold(tx, ty, "DIRECTOR TIMING CONTROLLER  [T: Toggle HUD]");

    // Current Time & Status
    ty -= 20.0f;
    char buf[256];
    glColor3f(0.90f, 0.92f, 0.95f);
    snprintf(buf, sizeof(buf), "Current Time: %.2fs / %.1fs   |  Status: %s",
             seqTime, SEQUENCE_DURATION, isPaused ? "PAUSED" : "PLAYING");
    drawText(tx, ty, buf);

    // Cut-to-black parameter
    ty -= 20.0f;
    glColor3f(0.35f, 0.85f, 1.0f);
    snprintf(buf, sizeof(buf), "Cut-to-Black Time:  %.2fs   [ [ / ] : -/+0.2s | { / } : -/+1.0s ]", g_cutToBlackTime);
    drawText(tx, ty, buf);

    // Title reveal parameter
    ty -= 18.0f;
    glColor3f(0.35f, 0.85f, 1.0f);
    snprintf(buf, sizeof(buf), "Title Reveal Time:  %.2fs   [ - / = : -/+0.2s ]", g_titleRevealTime);
    drawText(tx, ty, buf);

    // Controls
    ty -= 20.0f;
    glColor3f(0.70f, 0.75f, 0.80f);
    drawText(tx, ty, "[4] Jump to 70s Climax | [<- / ->] Scrub 0.5s | [Space] Pause");

    ty -= 18.0f;
    glColor3f(0.30f, 1.0f, 0.40f);
    drawText(tx, ty, "[Enter or S] Save Settings to Disk");

    // Status message
    if (g_statusMsgTimer > 0.0f) {
        glColor3f(0.20f, 1.0f, 0.40f);
        drawTextBold(boxX, boxY - 18.0f, g_statusMsg);
    }

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

bool handleTimingKeyboard(unsigned char key) {
    if (key == 't' || key == 'T') {
        g_showTimingHUD = !g_showTimingHUD;
        return true;
    }

    if (!g_showTimingHUD) return false;

    if (key == '[') {
        g_cutToBlackTime = std::max(65.0f, g_cutToBlackTime - 0.20f);
        if (g_titleRevealTime < g_cutToBlackTime + 0.3f) g_titleRevealTime = g_cutToBlackTime + 0.7f;
        return true;
    } else if (key == ']') {
        g_cutToBlackTime = std::min(SEQUENCE_DURATION - 1.0f, g_cutToBlackTime + 0.20f);
        if (g_titleRevealTime < g_cutToBlackTime + 0.3f) g_titleRevealTime = g_cutToBlackTime + 0.7f;
        return true;
    } else if (key == '{') { // Shift + [
        g_cutToBlackTime = std::max(65.0f, g_cutToBlackTime - 1.0f);
        if (g_titleRevealTime < g_cutToBlackTime + 0.3f) g_titleRevealTime = g_cutToBlackTime + 0.7f;
        return true;
    } else if (key == '}') { // Shift + ]
        g_cutToBlackTime = std::min(SEQUENCE_DURATION - 1.0f, g_cutToBlackTime + 1.0f);
        if (g_titleRevealTime < g_cutToBlackTime + 0.3f) g_titleRevealTime = g_cutToBlackTime + 0.7f;
        return true;
    } else if (key == '-' || key == '_') {
        g_titleRevealTime = std::max(g_cutToBlackTime + 0.1f, g_titleRevealTime - 0.20f);
        return true;
    } else if (key == '=' || key == '+') {
        g_titleRevealTime = std::min(SEQUENCE_DURATION - 0.5f, g_titleRevealTime + 0.20f);
        return true;
    } else if (key == 13 || key == 's' || key == 'S') { // Enter or S
        saveTimingConfig();
        return true;
    }

    return false;
}

bool handleTimingSpecial(int key) {
    if (!g_showTimingHUD) return false;

    if (key == GLUT_KEY_LEFT) {
        seqTime = std::max(0.0f, seqTime - 0.50f);
        return true;
    } else if (key == GLUT_KEY_RIGHT) {
        seqTime = std::min(SEQUENCE_DURATION - 0.1f, seqTime + 0.50f);
        return true;
    }

    return false;
}
