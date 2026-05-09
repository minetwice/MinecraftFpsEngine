#include "hook.h"
#include <dlfcn.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/log.h>
#include <cstring>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "FilterEngine", __VA_ARGS__)

// Original function pointer
typedef void (*glDrawElements_t)(GLenum mode, GLsizei count, GLenum type, const void* indices);
static glDrawElements_t original_glDrawElements = nullptr;

// Our replacement (will batch later)
void hacked_glDrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices) {
    // For now just pass through - batching will be added in filter_pipeline
    LOGI("glDrawElements intercepted: mode=%d count=%d", mode, count);
    if (original_glDrawElements) {
        original_glDrawElements(mode, count, type, indices);
    }
}

void install_hooks() {
    void* lib = dlopen("libGLESv3.so", RTLD_NOW);
    if (!lib) {
        lib = dlopen("libGLESv2.so", RTLD_NOW);
    }
    if (lib) {
        original_glDrawElements = (glDrawElements_t)dlsym(lib, "glDrawElements");
        // Simple function hijacking (for production, use a proper hooking lib like SubHook)
        // This is a demo - actual hooking requires using mprotect and writing jump instructions.
        LOGI("Hooks installed, original glDrawElements at %p", original_glDrawElements);
    } else {
        LOGI("Failed to load GLES library");
    }
}

void remove_hooks() {
    // Restore original function (not implemented in this demo)
}
