#include "Planet.h"
#include "Common.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

GLuint marsTextureID       = 0;
GLuint uranusTextureID     = 0;
GLuint jupiterTextureID    = 0;
GLuint saturnTextureID     = 0;
GLuint saturnRingTextureID = 0;
bool   hasTexture          = false;

float  marsSpinAngle    = 315.0f;
float  phobosAngle      = 0.8f;
float  uranusSpinAngle   = 45.0f;
float  jupiterSpinAngle  = 120.0f;
float  saturnSpinAngle   = 200.0f;

#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif

static GLuint loadTextureFromFile(const char* filename, bool isRGBA, bool repeatS) {
    int w, h, channels;
    int req = isRGBA ? 4 : 3;
    unsigned char* data = stbi_load(filename, &w, &h, &channels, req);
    if (!data) return 0;

    GLuint texID = 0;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.0f);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeatS ? GL_REPEAT : GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    GLenum format = isRGBA ? GL_RGBA : GL_RGB;
    gluBuild2DMipmaps(GL_TEXTURE_2D, format, w, h, format, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
    return texID;
}

static void createProceduralMarsTexture() {
    const int W = 512, H = 256;
    unsigned char* data = new unsigned char[W * H * 3];
    for (int y = 0; y < H; ++y) {
        float v = (float)y / (float)H;
        for (int x = 0; x < W; ++x) {
            float u = (float)x / (float)W;
            float n1 = sinf(u * 12.0f) * cosf(v * 8.0f);
            float n2 = sinf(u * 28.0f + 1.2f) * sinf(v * 20.0f);
            float blend = (n1 * 0.6f + n2 * 0.4f + 1.0f) * 0.5f;

            float r = lerpf(0.68f, 0.42f, blend);
            float g = lerpf(0.32f, 0.16f, blend);
            float b = lerpf(0.18f, 0.08f, blend);

            // Polar ice cap hints
            if (v < 0.08f || v > 0.92f) {
                float cap = (v < 0.08f) ? (1.0f - v / 0.08f) : ((v - 0.92f) / 0.08f);
                r = lerpf(r, 0.85f, cap * 0.7f);
                g = lerpf(g, 0.88f, cap * 0.7f);
                b = lerpf(b, 0.92f, cap * 0.8f);
            }

            int idx = (y * W + x) * 3;
            data[idx + 0] = (unsigned char)(clampf(r, 0.0f, 1.0f) * 255.0f);
            data[idx + 1] = (unsigned char)(clampf(g, 0.0f, 1.0f) * 255.0f);
            data[idx + 2] = (unsigned char)(clampf(b, 0.0f, 1.0f) * 255.0f);
        }
    }
    glGenTextures(1, &marsTextureID);
    glBindTexture(GL_TEXTURE_2D, marsTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.0f);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, W, H, GL_RGB, GL_UNSIGNED_BYTE, data);
    delete[] data;
    hasTexture = true;
}

void loadMarsTexture() {
    int w, h, channels;
    const char* filenames[] = {
        "mars_surface_4k.jpg",
        "mars_surface_8k.jpg",
        "mars_surface.jpg",
        "reference_archive/textures/Mars.jpeg",
        "textures/Mars.jpeg",
        "../mars_surface.jpg"
    };

    unsigned char* data = NULL;
    for (int i = 0; i < 6; ++i) {
        data = stbi_load(filenames[i], &w, &h, &channels, 3);
        if (data) break;
    }

    if (!data) {
        createProceduralMarsTexture();
        return;
    }

    glGenTextures(1, &marsTextureID);
    glBindTexture(GL_TEXTURE_2D, marsTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.0f);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, w, h, GL_RGB, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
    hasTexture = true;

    loadPlanetTextures();
}

void loadPlanetTextures() {
    if (!uranusTextureID) {
        uranusTextureID     = loadTextureFromFile("uranus_surface_2k.jpg", false, true);
        if (!uranusTextureID) uranusTextureID = loadTextureFromFile("textures/uranus_surface_2k.jpg", false, true);
    }
    if (!jupiterTextureID) {
        jupiterTextureID    = loadTextureFromFile("jupiter_surface_2k.jpg", false, true);
        if (!jupiterTextureID) jupiterTextureID = loadTextureFromFile("textures/jupiter_surface_2k.jpg", false, true);
    }
    if (!saturnTextureID) {
        saturnTextureID     = loadTextureFromFile("saturn_surface_4k.jpg", false, true);
        if (!saturnTextureID) saturnTextureID = loadTextureFromFile("saturn_surface_2k.jpg", false, true);
        if (!saturnTextureID) saturnTextureID = loadTextureFromFile("textures/saturn_surface_4k.jpg", false, true);
    }
    if (!saturnRingTextureID) {
        saturnRingTextureID = loadTextureFromFile("saturn_ring_alpha.png", true, false);
        if (!saturnRingTextureID) saturnRingTextureID = loadTextureFromFile("textures/saturn_ring_alpha.png", true, false);
    }
}

static void renderTexturedSphere(float radius, int slices, int stacks) {
    for (int i = 0; i < stacks; ++i) {
        float lat0 = (float)M_PI * (-0.5f + (float)i / stacks);
        float lat1 = (float)M_PI * (-0.5f + (float)(i + 1) / stacks);
        float y0   = radius * sinf(lat0);
        float yr0  = radius * cosf(lat0);
        float y1   = radius * sinf(lat1);
        float yr1  = radius * cosf(lat1);
        float v0   = 1.0f - (float)i / stacks;
        float v1   = 1.0f - (float)(i + 1) / stacks;

        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= slices; ++j) {
            float lon    = 2.0f * (float)M_PI * (float)j / slices;
            float cosLon = cosf(lon);
            float sinLon = sinf(lon);
            float u      = (float)j / slices;

            // Emit bottom vertex then top vertex (Counter-Clockwise outward winding)
            glNormal3f(cosLon * cosf(lat0), sinf(lat0), sinLon * cosf(lat0));
            glTexCoord2f(u, v0);
            glVertex3f(cosLon * yr0, y0, sinLon * yr0);

            glNormal3f(cosLon * cosf(lat1), sinf(lat1), sinLon * cosf(lat1));
            glTexCoord2f(u, v1);
            glVertex3f(cosLon * yr1, y1, sinLon * yr1);
        }
        glEnd();
    }
}

void renderMarsAndAtmosphere() {
    // 1. Mars Body (25.19 deg tilt + spin)
    glPushMatrix();
    glDisable(GL_CULL_FACE); // Guarantee front face is never culled
    glRotatef(25.19f, 0.0f, 0.0f, 1.0f);
    glRotatef(marsSpinAngle, 0.0f, 1.0f, 0.0f);

    if (hasTexture) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, marsTextureID);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }

    GLfloat marsDiffuse[]  = { 1.85f, 1.65f, 1.50f, 1.0f };
    GLfloat marsAmbient[]  = { 0.35f, 0.28f, 0.22f, 1.0f };
    GLfloat marsSpecular[] = { 0.12f, 0.09f, 0.06f, 1.0f };
    GLfloat marsShine[]    = { 6.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, marsDiffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, marsAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, marsSpecular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, marsShine);

    renderTexturedSphere(MARS_RADIUS, MARS_SLICES, MARS_STACKS);

    if (hasTexture) {
        glDisable(GL_TEXTURE_2D);
    }
    glEnable(GL_CULL_FACE);
    glPopMatrix();
}

static void renderAtmosphericLimb(float radius, float r, float g, float b, float a) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDisable(GL_CULL_FACE);
    GLfloat limbMat[] = { r, g, b, a };
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, limbMat);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, limbMat);
    glutSolidSphere(radius, 48, 28);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
}

static void renderSaturnRings(float innerR, float outerR, int segments) {
    if (saturnRingTextureID) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, saturnRingTextureID);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.03f);
    glDisable(GL_CULL_FACE);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

    GLfloat ringDiffuse[]  = { 1.70f, 1.58f, 1.36f, 0.98f };
    GLfloat ringAmbient[]  = { 0.45f, 0.40f, 0.32f, 0.98f };
    GLfloat ringSpecular[] = { 0.30f, 0.25f, 0.18f, 1.0f };
    GLfloat ringShine[]    = { 16.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, ringDiffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ringAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, ringSpecular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, ringShine);

    glBegin(GL_QUAD_STRIP);
    glNormal3f(0.0f, 1.0f, 0.0f);
    for (int i = 0; i <= segments; ++i) {
        float angle = (float)i / (float)segments * 2.0f * (float)M_PI;
        float cosA = cosf(angle);
        float sinA = sinf(angle);

        glTexCoord2f(0.0f, 0.5f);
        glVertex3f(innerR * cosA, 0.0f, innerR * sinA);

        glTexCoord2f(1.0f, 0.5f);
        glVertex3f(outerR * cosA, 0.0f, outerR * sinA);
    }
    glEnd();

    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
    if (saturnRingTextureID) {
        glDisable(GL_TEXTURE_2D);
    }
    glEnable(GL_CULL_FACE);
}

void renderBackgroundPlanets() {
    // 1. URANUS (Deep space void at start of video: smallest, distant pale cyan ice giant)
    glPushMatrix();
    glTranslatef(34.0f, 2.2f, 14.8f);
    glDisable(GL_CULL_FACE);
    glRotatef(97.77f, 1.0f, 0.0f, 0.0f);
    glRotatef(uranusSpinAngle, 0.0f, 0.0f, 1.0f);

    if (uranusTextureID) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, uranusTextureID);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }
    GLfloat uranusDiffuse[]  = { 1.30f, 1.45f, 1.55f, 1.0f };
    GLfloat uranusAmbient[]  = { 0.28f, 0.32f, 0.38f, 1.0f };
    GLfloat uranusSpecular[] = { 0.18f, 0.24f, 0.28f, 1.0f };
    GLfloat uranusShine[]    = { 12.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, uranusDiffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, uranusAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, uranusSpecular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, uranusShine);

    renderTexturedSphere(0.38f, 56, 32);

    if (uranusTextureID) {
        glDisable(GL_TEXTURE_2D);
    }
    // Atmospheric limb
    renderAtmosphericLimb(0.38f * 1.025f, 0.20f, 0.45f, 0.70f, 0.28f);

    glEnable(GL_CULL_FACE);
    glPopMatrix();

    // 2. JUPITER (Introduced as camera glides: noticeably bigger than Uranus, banded storms)
    glPushMatrix();
    glTranslatef(22.0f, 3.6f, 5.2f);
    glDisable(GL_CULL_FACE);
    glRotatef(3.13f, 0.0f, 0.0f, 1.0f);
    glRotatef(jupiterSpinAngle, 0.0f, 1.0f, 0.0f);

    if (jupiterTextureID) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, jupiterTextureID);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }
    GLfloat jupDiffuse[]  = { 2.15f, 2.00f, 1.80f, 1.0f };
    GLfloat jupAmbient[]  = { 0.50f, 0.44f, 0.38f, 1.0f };
    GLfloat jupSpecular[] = { 0.24f, 0.20f, 0.16f, 1.0f };
    GLfloat jupShine[]    = { 10.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, jupDiffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, jupAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, jupSpecular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, jupShine);

    renderTexturedSphere(0.68f, 72, 40);

    if (jupiterTextureID) {
        glDisable(GL_TEXTURE_2D);
    }
    // Atmospheric limb
    renderAtmosphericLimb(0.68f * 1.018f, 0.38f, 0.28f, 0.16f, 0.22f);

    glEnable(GL_CULL_FACE);
    glPopMatrix();

    // 3. SATURN & RINGS (In centered Mars shot: framed at bottom-right with iconic rings visible)
    glPushMatrix();
    glTranslatef(14.2f, -4.5f, -11.0f);
    glDisable(GL_CULL_FACE);

    // Dynamic tilt angling rings toward camera
    glRotatef(26.73f, 1.0f, 0.0f, 0.35f);
    glRotatef(saturnSpinAngle, 0.0f, 1.0f, 0.0f);

    if (saturnTextureID) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, saturnTextureID);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }
    GLfloat satDiffuse[]  = { 1.62f, 1.50f, 1.28f, 1.0f };
    GLfloat satAmbient[]  = { 0.36f, 0.32f, 0.24f, 1.0f };
    GLfloat satSpecular[] = { 0.22f, 0.18f, 0.12f, 1.0f };
    GLfloat satShine[]    = { 14.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, satDiffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, satAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, satSpecular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, satShine);

    renderTexturedSphere(0.65f, 96, 56);

    if (saturnTextureID) {
        glDisable(GL_TEXTURE_2D);
    }
    // Atmospheric limb
    renderAtmosphericLimb(0.65f * 1.018f, 0.45f, 0.36f, 0.20f, 0.25f);

    // Saturn's Rings (200 segments for master-grade curvature)
    renderSaturnRings(0.88f, 1.90f, 200);

    glEnable(GL_CULL_FACE);
    glPopMatrix();
}

void renderPhobos() {
    float pr = MARS_RADIUS + 3.15f;
    float px = pr * cosf(phobosAngle);
    float py = pr * 0.18f * sinf(phobosAngle);
    float pz = pr * sinf(phobosAngle);

    glPushMatrix();
    glTranslatef(px, py, pz);

    GLfloat phobosMat[] = { 0.38f, 0.35f, 0.32f, 1.0f };
    glMaterialfv(GL_FRONT, GL_DIFFUSE, phobosMat);

    glPushMatrix();
    glScalef(0.048f, 0.038f, 0.032f);
    glutSolidSphere(1.0, 14, 10);
    glPopMatrix();

    glPopMatrix();
}

