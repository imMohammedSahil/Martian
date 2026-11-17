#include "VideoExporter.h"
#include "Common.h"
#include "SceneDirector.h"
#include <windows.h>
#include <GL/gl.h>
#include <GL/freeglut.h>
#include <cstdio>
#include <vector>
#include <string>

#define GL_FRAMEBUFFER 0x8D40
#define GL_RENDERBUFFER 0x8D41
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_DEPTH_ATTACHMENT 0x8D00
#define GL_DEPTH_COMPONENT24 0x81A6
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5

typedef void (APIENTRY *PFNGLGENFRAMEBUFFERSPROC) (GLsizei n, GLuint *framebuffers);
typedef void (APIENTRY *PFNGLBINDFRAMEBUFFERPROC) (GLenum target, GLuint framebuffer);
typedef void (APIENTRY *PFNGLGENRENDERBUFFERSPROC) (GLsizei n, GLuint *renderbuffers);
typedef void (APIENTRY *PFNGLBINDRENDERBUFFERPROC) (GLenum target, GLuint renderbuffer);
typedef void (APIENTRY *PFNGLRENDERBUFFERSTORAGEPROC) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
typedef void (APIENTRY *PFNGLFRAMEBUFFERRENDERBUFFERPROC) (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
typedef void (APIENTRY *PFNGLFRAMEBUFFERTEXTURE2DPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef GLenum (APIENTRY *PFNGLCHECKFRAMEBUFFERSTATUSPROC) (GLenum target);
typedef void (APIENTRY *PFNGLDELETEFRAMEBUFFERSPROC) (GLsizei n, const GLuint *framebuffers);
typedef void (APIENTRY *PFNGLDELETERENDERBUFFERSPROC) (GLsizei n, const GLuint *renderbuffers);

extern void display();
extern void reshape(int w, int h);

static bool exportSingle(int outW, int outH, const char* outFilename, const char* label, float maxDuration = -1.0f) {
    PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)wglGetProcAddress("glGenFramebuffers");
    if (!glGenFramebuffers) glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)wglGetProcAddress("glGenFramebuffersEXT");

    PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)wglGetProcAddress("glBindFramebuffer");
    if (!glBindFramebuffer) glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)wglGetProcAddress("glBindFramebufferEXT");

    PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)wglGetProcAddress("glGenRenderbuffers");
    if (!glGenRenderbuffers) glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)wglGetProcAddress("glGenRenderbuffersEXT");

    PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)wglGetProcAddress("glBindRenderbuffer");
    if (!glBindRenderbuffer) glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)wglGetProcAddress("glBindRenderbufferEXT");

    PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)wglGetProcAddress("glRenderbufferStorage");
    if (!glRenderbufferStorage) glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)wglGetProcAddress("glRenderbufferStorageEXT");

    PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)wglGetProcAddress("glFramebufferRenderbuffer");
    if (!glFramebufferRenderbuffer) glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)wglGetProcAddress("glFramebufferRenderbufferEXT");

    PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)wglGetProcAddress("glFramebufferTexture2D");
    if (!glFramebufferTexture2D) glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)wglGetProcAddress("glFramebufferTexture2DEXT");

    PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)wglGetProcAddress("glCheckFramebufferStatus");
    if (!glCheckFramebufferStatus) glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)wglGetProcAddress("glCheckFramebufferStatusEXT");

    PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)wglGetProcAddress("glDeleteFramebuffers");
    if (!glDeleteFramebuffers) glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)wglGetProcAddress("glDeleteFramebuffersEXT");

    PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)wglGetProcAddress("glDeleteRenderbuffers");
    if (!glDeleteRenderbuffers) glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)wglGetProcAddress("glDeleteRenderbuffersEXT");

    if (!glGenFramebuffers || !glBindFramebuffer || !glGenRenderbuffers || !glRenderbufferStorage) {
        printf("Error: GPU does not support FBO rendering.\n");
        return false;
    }

    GLuint fbo = 0, tex = 0, depth = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, outW, outH, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

    glGenRenderbuffers(1, &depth);
    glBindRenderbuffer(GL_RENDERBUFFER, depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, outW, outH);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        printf("Error: Framebuffer incomplete (0x%X)\n", glCheckFramebufferStatus(GL_FRAMEBUFFER));
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }

    windowWidth  = outW;
    windowHeight = outH;
    glViewport(0, 0, outW, outH);
    reshape(outW, outH);

    g_showTimingHUD = false;

    char ffmpegCmd[1024];
    // Check if soundtrack.mp3 exists
    bool hasAudio = false;
    FILE* fa = fopen("soundtrack.mp3", "rb");
    if (fa) { fclose(fa); hasAudio = true; }

    float dur = (maxDuration > 0.0f) ? maxDuration : SEQUENCE_DURATION;

    if (hasAudio) {
        float audioCut = (maxDuration > 0.0f) ? maxDuration : g_cutToBlackTime;
        float fadeDur = (maxDuration > 0.0f) ? 1.5f : 0.08f;
        float fadeStart = audioCut - fadeDur;
        if (fadeStart < 0.0f) fadeStart = 0.0f;
        snprintf(ffmpegCmd, sizeof(ffmpegCmd),
            "C:\\ffmpeg\\bin\\ffmpeg.exe -y -f rawvideo -vcodec rawvideo -s %dx%d -pix_fmt rgb24 -r 60 -i - -i soundtrack.mp3 "
            "-filter_complex \"[1:a]atrim=0:%.2f,asetpts=PTS-STARTPTS,afade=t=out:st=%.2f:d=%.2f,apad=whole_dur=%.2f[aout]\" "
            "-map 0:v -map \"[aout]\" -vf vflip -c:v h264_nvenc -pix_fmt yuv420p -preset p7 -cq 16 -b:v 0 -c:a aac -b:a 320k -shortest \"%s\"",
            outW, outH, audioCut, fadeStart, fadeDur, dur, outFilename);
    } else {
        snprintf(ffmpegCmd, sizeof(ffmpegCmd),
            "C:\\ffmpeg\\bin\\ffmpeg.exe -y -f rawvideo -vcodec rawvideo -s %dx%d -pix_fmt rgb24 -r 60 -i - -vf vflip -c:v h264_nvenc -pix_fmt yuv420p -preset p7 -cq 16 -b:v 0 \"%s\"",
            outW, outH, outFilename);
    }

    FILE* pipe = _popen(ffmpegCmd, "wb");
    if (!pipe) {
        printf("Error: Could not open FFmpeg pipe.\n");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }

    const int FPS = 60;
    const int totalFrames = (int)(dur * (float)FPS);
    const float dt = 1.0f / (float)FPS;

    std::vector<unsigned char> fullBuffer(outW * outH * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    printf("\n========================================================\n");
    printf(" EXPORTING CINEMATIC VIDEO [%s: %dx%d @ 60 FPS]\n", label, outW, outH);
    printf(" Target File: %s\n", outFilename);
    printf(" Total Frames: %d (Duration: %.1fs)\n", totalFrames, dur);
    printf("========================================================\n");

    seqTime = 0.0f;
    totalTime = 0.0f;
    particles.clear();

    DWORD startMs = GetTickCount();

    for (int frame = 0; frame < totalFrames; ++frame) {
        seqTime = (float)frame * dt;
        totalTime = seqTime;

        updateSceneDirector(dt);
        display();

        glReadPixels(0, 0, outW, outH, GL_RGB, GL_UNSIGNED_BYTE, fullBuffer.data());

        // Fast single bulk write (FFmpeg -vf vflip handles vertical orientation losslessly)
        fwrite(fullBuffer.data(), 1, (size_t)outW * outH * 3, pipe);

        if (frame % 30 == 0 || frame == totalFrames - 1) {
            float progress = (float)(frame + 1) / (float)totalFrames;
            int barWidth = 28;
            int pos = (int)(barWidth * progress);
            char bar[32];
            for (int b = 0; b < barWidth; ++b) bar[b] = (b < pos) ? '=' : (b == pos ? '>' : ' ');
            bar[barWidth] = '\0';

            DWORD elapsedMs = GetTickCount() - startMs;
            float elapsedSec = (float)elapsedMs / 1000.0f;
            float renderFps = (elapsedSec > 0.1f) ? ((float)(frame + 1) / elapsedSec) : 0.0f;
            float etaSec = (renderFps > 0.1f) ? ((float)(totalFrames - frame - 1) / renderFps) : 0.0f;

            printf("\r[%s] %5.1f%% | Frame %4d/%d (T=%.1fs) | Speed: %4.1f fps | ETA: %3.0fs ",
                   bar, progress * 100.0f, frame + 1, totalFrames, seqTime, renderFps, etaSec);
            fflush(stdout);
        }
    }

    int code = _pclose(pipe);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &tex);
    glDeleteRenderbuffers(1, &depth);

    if (code != 0) {
        printf("\n[ERROR] FFmpeg exited with error code %d\n", code);
        return false;
    }

    printf("\n[SUCCESS] %s generated successfully: %s\n", label, outFilename);
    return true;
}

bool runVideoExport(const char* preset, float duration) {
    if (!preset) return false;

    char filename1080[256];
    char filename4k[256];
    if (duration > 0.0f) {
        snprintf(filename1080, sizeof(filename1080), "Martian_Scene1_Title_1080p_60fps.mp4");
        snprintf(filename4k, sizeof(filename4k), "Martian_Scene1_Title_4K_60fps.mp4");
    } else {
        snprintf(filename1080, sizeof(filename1080), "Martian_1080p_60fps.mp4");
        snprintf(filename4k, sizeof(filename4k), "Martian_4K_60fps.mp4");
    }

    if (strcmp(preset, "1080p") == 0 || strcmp(preset, "1080") == 0) {
        return exportSingle(1920, 1080, filename1080, "Full HD 1080p", duration);
    } else if (strcmp(preset, "4k") == 0 || strcmp(preset, "4K") == 0 || strcmp(preset, "2160p") == 0) {
        return exportSingle(3840, 2160, filename4k, "Ultra HD 4K", duration);
    } else if (strcmp(preset, "both") == 0) {
        bool ok1 = exportSingle(1920, 1080, filename1080, "Full HD 1080p", duration);
        bool ok2 = exportSingle(3840, 2160, filename4k, "Ultra HD 4K", duration);
        return ok1 && ok2;
    }

    printf("Unknown export preset '%s'. Use '1080p', '4k', or 'both'.\n", preset);
    return false;
}
