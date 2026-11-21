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
    // Negative LOD bias ensures crisp high-frequency cloud bands and storm structures
    glTexParameterf(GL_TEXTURE_2D, 0x8501 /* GL_TEXTURE_LOD_BIAS */, -0.65f);
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
        jupiterTextureID    = loadTextureFromFile("jupiter_surface_8k.jpg", false, true);
        if (!jupiterTextureID) jupiterTextureID = loadTextureFromFile("jupiter_surface_2k.jpg", false, true);
        if (!jupiterTextureID) jupiterTextureID = loadTextureFromFile("textures/jupiter_surface_8k.jpg", false, true);
        if (!jupiterTextureID) jupiterTextureID = loadTextureFromFile("textures/jupiter_surface_2k.jpg", false, true);
    }
    if (!saturnTextureID) {
        saturnTextureID     = loadTextureFromFile("saturn_surface_8k.jpg", false, true);
        if (!saturnTextureID) saturnTextureID = loadTextureFromFile("saturn_surface_4k.jpg", false, true);
        if (!saturnTextureID) saturnTextureID = loadTextureFromFile("saturn_surface_2k.jpg", false, true);
        if (!saturnTextureID) saturnTextureID = loadTextureFromFile("textures/saturn_surface_8k.jpg", false, true);
        if (!saturnTextureID) saturnTextureID = loadTextureFromFile("textures/saturn_surface_4k.jpg", false, true);
    }
    if (!saturnRingTextureID) {
        saturnRingTextureID = loadTextureFromFile("saturn_ring_8k_alpha.png", true, false);
        if (!saturnRingTextureID) saturnRingTextureID = loadTextureFromFile("saturn_ring_alpha.png", true, false);
        if (!saturnRingTextureID) saturnRingTextureID = loadTextureFromFile("textures/saturn_ring_8k_alpha.png", true, false);
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

static void worldToJupiterLocal(float wx, float wy, float wz, float& lx, float& ly, float& lz) {
    // 1. Inverse tilt: -3.13 deg around Z
    float angle = -3.13f * (float)M_PI / 180.0f;
    float cosA = cosf(angle);
    float sinA = sinf(angle);
    float x1 = wx * cosA - wy * sinA;
    float y1 = wx * sinA + wy * cosA;
    float z1 = wz;

    // 2. Inverse spin: -jupiterSpinAngle around Y
    float spin = -jupiterSpinAngle * (float)M_PI / 180.0f;
    float cosS = cosf(spin);
    float sinS = sinf(spin);
    lx = x1 * cosS + z1 * sinS;
    ly = y1;
    lz = -x1 * sinS + z1 * cosS;
    normalize3(lx, ly, lz);
}

static void renderJupiterGlobeWithShadow(float radius, int slices, int stacks, float lx, float ly, float lz) {
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

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

            // Vertex 0
            float nx0 = cosLon * cosf(lat0);
            float ny0 = sinf(lat0);
            float nz0 = sinLon * cosf(lat0);
            float dot0 = nx0 * lx + ny0 * ly + nz0 * lz;
            // Soft deep-space terminator with subtle ambient floor
            float t0 = clampf((dot0 + 0.05f) / 0.22f, 0.0f, 1.0f);
            float shade0 = 0.08f + 0.92f * t0;

            glNormal3f(nx0, ny0, nz0);
            glColor4f(shade0, shade0, shade0, 1.0f);
            glTexCoord2f(u, v0);
            glVertex3f(cosLon * yr0, y0, sinLon * yr0);

            // Vertex 1
            float nx1 = cosLon * cosf(lat1);
            float ny1 = sinf(lat1);
            float nz1 = sinLon * cosf(lat1);
            float dot1 = nx1 * lx + ny1 * ly + nz1 * lz;
            float t1 = clampf((dot1 + 0.05f) / 0.22f, 0.0f, 1.0f);
            float shade1 = 0.08f + 0.92f * t1;

            glNormal3f(nx1, ny1, nz1);
            glColor4f(shade1, shade1, shade1, 1.0f);
            glTexCoord2f(u, v1);
            glVertex3f(cosLon * yr1, y1, sinLon * yr1);
        }
        glEnd();
    }
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glDisable(GL_COLOR_MATERIAL);
}

static void worldToSaturnLocal(float wx, float wy, float wz, float& lx, float& ly, float& lz) {
    // 1. Inverse tilt: -26.73 deg around axis (1.0, 0.0, 0.35)
    float ax = 1.0f, ay = 0.0f, az = 0.35f;
    normalize3(ax, ay, az);
    float angle = -26.73f * (float)M_PI / 180.0f;
    float cosA = cosf(angle);
    float sinA = sinf(angle);

    float dot = ax * wx + ay * wy + az * wz;
    float cx = ay * wz - az * wy;
    float cy = az * wx - ax * wz;
    float cz = ax * wy - ay * wx;

    float x1 = wx * cosA + cx * sinA + ax * dot * (1.0f - cosA);
    float y1 = wy * cosA + cy * sinA + ay * dot * (1.0f - cosA);
    float z1 = wz * cosA + cz * sinA + az * dot * (1.0f - cosA);

    // 2. Inverse spin: -saturnSpinAngle around (0, 1, 0)
    float spin = -saturnSpinAngle * (float)M_PI / 180.0f;
    float cosS = cosf(spin);
    float sinS = sinf(spin);

    lx = x1 * cosS + z1 * sinS;
    ly = y1;
    lz = -x1 * sinS + z1 * cosS;
    normalize3(lx, ly, lz);
}

static void renderSaturnGlobeWithShadow(float radius, int slices, int stacks, float innerR, float outerR, float lx, float ly, float lz) {
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

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

            // Vertex 0
            float px0 = cosLon * yr0;
            float py0 = y0;
            float pz0 = sinLon * yr0;

            float shadow0 = 1.0f;
            if (fabsf(ly) > 1e-4f) {
                float t = -py0 / ly;
                if (t > 0.005f) {
                    float ix = px0 + t * lx;
                    float iz = pz0 + t * lz;
                    float r = sqrtf(ix * ix + iz * iz);
                    if (r >= innerR && r <= outerR) {
                        float uR = (r - innerR) / (outerR - innerR);
                        float op = 0.70f;
                        if (uR >= 0.56f && uR <= 0.64f) op = 0.12f; // Cassini division gap
                        else if (uR < 0.22f) op = 0.35f;            // C ring
                        else if (uR >= 0.22f && uR < 0.56f) op = 0.86f; // B ring (dense crisp shadow)
                        else op = 0.68f;                            // A ring
                        shadow0 = 1.0f - op;
                    }
                }
            }
            // Ambient Occlusion: equatorial contact crease darkening
            float ao0 = 1.0f;
            float dEq0 = fabsf(py0) / radius;
            if (dEq0 < 0.28f) {
                ao0 *= (0.55f + 0.45f * (dEq0 / 0.28f));
            }

            // Planetary day/night terminator shadow
            float nx0 = cosLon * cosf(lat0);
            float ny0 = sinf(lat0);
            float nz0 = sinLon * cosf(lat0);
            float dot0 = nx0 * lx + ny0 * ly + nz0 * lz;
            float term0 = 0.06f + 0.94f * clampf((dot0 + 0.04f) / 0.22f, 0.0f, 1.0f);

            float shade0 = shadow0 * ao0 * term0;

            glNormal3f(nx0, ny0, nz0);
            glColor4f(shade0, shade0, shade0, 1.0f);
            glTexCoord2f(u, v0);
            glVertex3f(px0, py0, pz0);

            // Vertex 1
            float px1 = cosLon * yr1;
            float py1 = y1;
            float pz1 = sinLon * yr1;

            float shadow1 = 1.0f;
            if (fabsf(ly) > 1e-4f) {
                float t = -py1 / ly;
                if (t > 0.005f) {
                    float ix = px1 + t * lx;
                    float iz = pz1 + t * lz;
                    float r = sqrtf(ix * ix + iz * iz);
                    if (r >= innerR && r <= outerR) {
                        float uR = (r - innerR) / (outerR - innerR);
                        float op = 0.70f;
                        if (uR >= 0.56f && uR <= 0.64f) op = 0.12f;
                        else if (uR < 0.22f) op = 0.35f;
                        else if (uR >= 0.22f && uR < 0.56f) op = 0.86f;
                        else op = 0.68f;
                        shadow1 = 1.0f - op;
                    }
                }
            }
            float ao1 = 1.0f;
            float dEq1 = fabsf(py1) / radius;
            if (dEq1 < 0.28f) {
                ao1 *= (0.55f + 0.45f * (dEq1 / 0.28f));
            }

            float nx1 = cosLon * cosf(lat1);
            float ny1 = sinf(lat1);
            float nz1 = sinLon * cosf(lat1);
            float dot1 = nx1 * lx + ny1 * ly + nz1 * lz;
            float term1 = 0.06f + 0.94f * clampf((dot1 + 0.04f) / 0.22f, 0.0f, 1.0f);

            float shade1 = shadow1 * ao1 * term1;

            glNormal3f(cosLon * cosf(lat1), sinf(lat1), sinLon * cosf(lat1));
            glColor4f(shade1, shade1, shade1, 1.0f);
            glTexCoord2f(u, v1);
            glVertex3f(px1, py1, pz1);
        }
        glEnd();
    }
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glDisable(GL_COLOR_MATERIAL);
}

static void renderSaturnRings(float innerR, float outerR, int segments, float lx, float ly, float lz) {
    float flare = getOpeningFlareIntensity();

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

    GLfloat ringDiffuse[]  = { 2.15f + 0.25f * flare, 2.00f + 0.22f * flare, 1.75f + 0.18f * flare, 1.0f };
    GLfloat ringAmbient[]  = { 0.62f + 0.12f * flare, 0.56f + 0.10f * flare, 0.48f + 0.08f * flare, 1.0f };
    GLfloat ringEmission[] = { 0.14f, 0.12f, 0.10f, 1.0f };
    GLfloat ringSpecular[] = { 0.45f + 0.30f * flare, 0.40f + 0.25f * flare, 0.32f + 0.20f * flare, 1.0f };
    GLfloat ringShine[]    = { 28.0f + 16.0f * flare };
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, ringDiffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ringAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, ringEmission);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, ringSpecular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, ringShine);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    const int radialBands = 6;
    for (int b = 0; b < radialBands; ++b) {
        float r0 = innerR + (outerR - innerR) * ((float)b / radialBands);
        float r1 = innerR + (outerR - innerR) * ((float)(b + 1) / radialBands);
        float u0 = (float)b / radialBands;
        float u1 = (float)(b + 1) / radialBands;

        glBegin(GL_QUAD_STRIP);
        glNormal3f(0.0f, 1.0f, 0.0f);
        for (int i = 0; i <= segments; ++i) {
            float angle = (float)i / (float)segments * 2.0f * (float)M_PI;
            float cosA = cosf(angle);
            float sinA = sinf(angle);

            float px0 = r0 * cosA;
            float pz0 = r0 * sinA;
            float shade0 = 1.0f;
            // Ray-sphere intersection: ray from ring point (px0, 0, pz0) along L towards sun hits Saturn globe (radius 0.42)
            float b0 = 2.0f * (px0 * lx + pz0 * lz);
            float c0 = (px0 * px0 + pz0 * pz0) - 0.425f * 0.425f;
            float disc0 = b0 * b0 - 4.0f * c0;
            if (disc0 >= 0.0f) {
                float t2_0 = (-b0 + sqrtf(disc0)) * 0.5f;
                if (t2_0 > 0.01f) {
                    shade0 = 0.08f; // Deep, crisp planetary globe shadow cast on rings
                }
            }

            float px1 = r1 * cosA;
            float pz1 = r1 * sinA;
            float shade1 = 1.0f;
            float b1 = 2.0f * (px1 * lx + pz1 * lz);
            float c1 = (px1 * px1 + pz1 * pz1) - 0.425f * 0.425f;
            float disc1 = b1 * b1 - 4.0f * c1;
            if (disc1 >= 0.0f) {
                float t2_1 = (-b1 + sqrtf(disc1)) * 0.5f;
                if (t2_1 > 0.01f) {
                    shade1 = 0.08f; // Deep, crisp planetary globe shadow cast on rings
                }
            }

            glColor4f(shade0, shade0, shade0, 1.0f);
            glTexCoord2f(u0, 0.5f);
            glVertex3f(px0, 0.0f, pz0);

            glColor4f(shade1, shade1, shade1, 1.0f);
            glTexCoord2f(u1, 0.5f);
            glVertex3f(px1, 0.0f, pz1);
        }
        glEnd();
    }

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glDisable(GL_COLOR_MATERIAL);

    GLfloat zeroEmission[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, zeroEmission);

    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_BLEND);
    if (saturnRingTextureID) {
        glDisable(GL_TEXTURE_2D);
    }
    glEnable(GL_CULL_FACE);
}

void renderBackgroundPlanets() {
    float flare = getOpeningFlareIntensity();

    // 1. URANUS (Pale cyan ice giant: refined natural lighting, crisp specular glint from flare)
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
    GLfloat uranusDiffuse[]  = { 0.95f + 0.15f * flare, 1.10f + 0.18f * flare, 1.25f + 0.20f * flare, 1.0f };
    GLfloat uranusAmbient[]  = { 0.20f, 0.24f, 0.28f, 1.0f };
    GLfloat uranusSpecular[] = { 0.22f + 0.30f * flare, 0.28f + 0.35f * flare, 0.35f + 0.38f * flare, 1.0f };
    GLfloat uranusShine[]    = { 20.0f + 20.0f * flare };
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, uranusDiffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, uranusAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, uranusSpecular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, uranusShine);

    renderTexturedSphere(0.15f, 56, 32);

    if (uranusTextureID) {
        glDisable(GL_TEXTURE_2D);
    }
    // Atmospheric limb
    renderAtmosphericLimb(0.15f * 1.025f, 0.15f, 0.35f, 0.55f, 0.20f + 0.10f * flare);

    glEnable(GL_CULL_FACE);
    glPopMatrix();

    // 2. JUPITER (Rich banded cloud textures, pure pitch-black space shadow on backside)
    glPushMatrix();
    glTranslatef(22.0f, 3.6f, 5.2f);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHT1); // Disable Martian planetary bounce fill so backside merges seamlessly with space
    glRotatef(3.13f, 0.0f, 0.0f, 1.0f);
    glRotatef(jupiterSpinAngle, 0.0f, 1.0f, 0.0f);

    if (jupiterTextureID) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, jupiterTextureID);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }
    GLfloat jupDiffuse[]  = { 1.25f + 0.15f * flare, 1.10f + 0.12f * flare, 0.95f + 0.10f * flare, 1.0f };
    GLfloat jupAmbient[]  = { 0.08f, 0.07f, 0.06f, 1.0f };
    GLfloat jupSpecular[] = { 0.22f + 0.25f * flare, 0.18f + 0.20f * flare, 0.14f + 0.16f * flare, 1.0f };
    GLfloat jupShine[]    = { 16.0f + 20.0f * flare };
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, jupDiffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, jupAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, jupSpecular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, jupShine);

    float jupSunLx = 0.0f, jupSunLy = 0.0f, jupSunLz = 1.0f;
    worldToJupiterLocal(SUN_DIR_X, SUN_DIR_Y, SUN_DIR_Z, jupSunLx, jupSunLy, jupSunLz);

    renderJupiterGlobeWithShadow(0.44f, 144, 72, jupSunLx, jupSunLy, jupSunLz);

    if (jupiterTextureID) {
        glDisable(GL_TEXTURE_2D);
    }
    // Delicate atmospheric rim
    renderAtmosphericLimb(0.44f * 1.015f, 0.20f, 0.15f, 0.10f, 0.06f);

    glEnable(GL_LIGHT1);
    glEnable(GL_CULL_FACE);
    glPopMatrix();

    // 3. SATURN & RINGS
    glPushMatrix();
    glTranslatef(14.2f, -4.5f, -11.0f);
    glDisable(GL_CULL_FACE);

    // Dynamic tilt angling rings toward camera
    glRotatef(26.73f, 1.0f, 0.0f, 0.35f);
    glRotatef(saturnSpinAngle, 0.0f, 1.0f, 0.0f);

    glDisable(GL_LIGHT1); // Disable Martian bounce fill light so Saturn's nightside shadow stays clean and deep

    if (saturnTextureID) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, saturnTextureID);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }
    GLfloat satDiffuse[]  = { 0.90f + 0.10f * flare, 0.82f + 0.08f * flare, 0.70f + 0.06f * flare, 1.0f };
    GLfloat satAmbient[]  = { 0.05f, 0.045f, 0.035f, 1.0f };
    GLfloat satSpecular[] = { 0.15f + 0.18f * flare, 0.13f + 0.15f * flare, 0.09f + 0.12f * flare, 1.0f };
    GLfloat satShine[]    = { 18.0f + 20.0f * flare };
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, satDiffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, satAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, satSpecular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, satShine);

    float satSunLx = 0.0f, satSunLy = 0.0f, satSunLz = 1.0f;
    worldToSaturnLocal(SUN_DIR_X, SUN_DIR_Y, SUN_DIR_Z, satSunLx, satSunLy, satSunLz);
    // Low solar elevation angle relative to equatorial ring plane so globe shadow extends across the rings
    satSunLy = 0.16f;
    normalize3(satSunLx, satSunLy, satSunLz);

    renderSaturnGlobeWithShadow(0.42f, 144, 72, 0.54f, 1.28f, satSunLx, satSunLy, satSunLz);

    if (saturnTextureID) {
        glDisable(GL_TEXTURE_2D);
    }
    // Atmospheric limb (toned down)
    renderAtmosphericLimb(0.42f * 1.015f, 0.28f, 0.22f, 0.12f, 0.09f + 0.04f * flare);

    // Saturn's Rings (300 segments for master-grade curvature)
    renderSaturnRings(0.54f, 1.28f, 300, satSunLx, satSunLy, satSunLz);

    glEnable(GL_LIGHT1);
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

