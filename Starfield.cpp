#include "Starfield.h"
#include "Common.h"

std::vector<Star> stars;

void initStarfield() {
    stars.resize(NUM_STARS);
    for (int i = 0; i < NUM_STARS; ++i) {
        float theta  = randRange(0.0f, 2.0f * (float)M_PI);
        float phi    = acosf(randRange(-1.0f, 1.0f));
        float radius = randRange(80.0f, 125.0f);

        stars[i].x = radius * sinf(phi) * cosf(theta);
        stars[i].y = radius * cosf(phi);
        stars[i].z = radius * sinf(phi) * sinf(theta);

        // Interstellar cold blue/cyan & crisp star palette
        float type = randRange(0.0f, 1.0f);
        if (type > 0.62f) {
            stars[i].r = 0.82f; stars[i].g = 0.90f; stars[i].b = 1.00f; // Cold blue
        } else if (type > 0.35f) {
            stars[i].r = 0.95f; stars[i].g = 0.96f; stars[i].b = 1.00f; // Pure white
        } else if (type > 0.12f) {
            stars[i].r = 1.00f; stars[i].g = 0.92f; stars[i].b = 0.80f; // Warm gold
        } else {
            stars[i].r = 0.65f; stars[i].g = 0.85f; stars[i].b = 1.00f; // Deep cyan
        }

        stars[i].brightness   = randRange(0.35f, 1.0f);
        stars[i].size         = randRange(1.0f, 2.4f);
        stars[i].twinkleSpeed = randRange(0.4f, 2.2f);
        stars[i].phase        = randRange(0.0f, 6.28f);
    }
}

void renderStarfield() {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glPointSize(1.4f);
    glBegin(GL_POINTS);
    for (size_t i = 0; i < stars.size(); ++i) {
        float tw = 0.78f + 0.22f * sinf(totalTime * stars[i].twinkleSpeed + stars[i].phase);
        float alpha = stars[i].brightness * tw;
        glColor4f(stars[i].r, stars[i].g, stars[i].b, alpha * 0.85f);
        glVertex3f(stars[i].x, stars[i].y, stars[i].z);
    }
    glEnd();

    // Prominent foreground bright stars
    glPointSize(2.6f);
    glBegin(GL_POINTS);
    for (size_t i = 0; i < stars.size(); i += 14) {
        float tw = 0.70f + 0.30f * sinf(totalTime * stars[i].twinkleSpeed * 1.5f + stars[i].phase);
        glColor4f(stars[i].r * 1.1f, stars[i].g * 1.1f, stars[i].b * 1.1f, stars[i].brightness * tw);
        glVertex3f(stars[i].x, stars[i].y, stars[i].z);
    }
    glEnd();

    glPointSize(1.0f);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

