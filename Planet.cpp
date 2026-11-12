#include "Planet.h"
#include "Common.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

GLuint marsTextureID = 0;
bool   hasTexture    = false;
float  marsSpinAngle = 28.0f;
float  phobosAngle   = 0.8f;

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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, w, h, GL_RGB, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
    hasTexture = true;
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

            glNormal3f(cosLon * cosf(lat1), sinf(lat1), sinLon * cosf(lat1));
            glTexCoord2f(u, v1);
            glVertex3f(cosLon * yr1, y1, sinLon * yr1);

            glNormal3f(cosLon * cosf(lat0), sinf(lat0), sinLon * cosf(lat0));
            glTexCoord2f(u, v0);
            glVertex3f(cosLon * yr0, y0, sinLon * yr0);
        }
        glEnd();
    }
}

void renderMarsAndAtmosphere() {
    // 1. Mars Body (25.19 deg tilt + spin)
    glPushMatrix();
    glRotatef(25.19f, 0.0f, 0.0f, 1.0f);
    glRotatef(marsSpinAngle, 0.0f, 1.0f, 0.0f);

    if (hasTexture) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, marsTextureID);
    }

    GLfloat marsDiffuse[]  = { 1.02f, 0.90f, 0.80f, 1.0f };
    GLfloat marsSpecular[] = { 0.08f, 0.06f, 0.04f, 1.0f };
    GLfloat marsShine[]    = { 6.0f };
    glMaterialfv(GL_FRONT, GL_DIFFUSE, marsDiffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, marsSpecular);
    glMaterialfv(GL_FRONT, GL_SHININESS, marsShine);

    renderTexturedSphere(MARS_RADIUS, MARS_SLICES, MARS_STACKS);

    if (hasTexture) {
        glDisable(GL_TEXTURE_2D);
    }
    glPopMatrix();

    // 2. Sun-Weighted Dual-Layer Martian Atmospheric Scattering
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);

    float sunX = SUN_DIR_X, sunY = SUN_DIR_Y, sunZ = SUN_DIR_Z;
    normalize3(sunX, sunY, sunZ);

    const float radii[2]   = { MARS_RADIUS * 1.018f, MARS_RADIUS * 1.052f };
    const float alphas[2]  = { 0.24f, 0.11f };

    for (int layer = 0; layer < 2; ++layer) {
        float r = radii[layer];
        int stacks = 36, slices = 72;
        for (int i = 0; i < stacks; ++i) {
            float lat0 = (float)M_PI * (-0.5f + (float)i / stacks);
            float lat1 = (float)M_PI * (-0.5f + (float)(i + 1) / stacks);
            float y0 = r * sinf(lat0), yr0 = r * cosf(lat0);
            float y1 = r * sinf(lat1), yr1 = r * cosf(lat1);

            glBegin(GL_QUAD_STRIP);
            for (int j = 0; j <= slices; ++j) {
                float lon = 2.0f * (float)M_PI * (float)j / slices;
                float cL = cosf(lon), sL = sinf(lon);

                float nx1 = cL * cosf(lat1), ny1 = sinf(lat1), nz1 = sL * cosf(lat1);
                float dotSun1 = nx1 * sunX + ny1 * sunY + nz1 * sunZ;
                float w1 = clampf(dotSun1 * 0.7f + 0.3f, 0.0f, 1.0f);
                glColor4f(1.0f * w1 + 0.35f, 0.52f * w1 + 0.15f, 0.25f * w1 + 0.08f, alphas[layer] * (w1 * 0.85f + 0.15f));
                glVertex3f(cL * yr1, y1, sL * yr1);

                float nx0 = cL * cosf(lat0), ny0 = sinf(lat0), nz0 = sL * cosf(lat0);
                float dotSun0 = nx0 * sunX + ny0 * sunY + nz0 * sunZ;
                float w0 = clampf(dotSun0 * 0.7f + 0.3f, 0.0f, 1.0f);
                glColor4f(1.0f * w0 + 0.35f, 0.52f * w0 + 0.15f, 0.25f * w0 + 0.08f, alphas[layer] * (w0 * 0.85f + 0.15f));
                glVertex3f(cL * yr0, y0, sL * yr0);
            }
            glEnd();
        }
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
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

