#include "compute_shader.h"
#include <android/log.h>
#include <GLES3/gl31.h>
#include <cstring>

#define LOG_TAG "ComputeShader"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

ComputeShader::ComputeShader() : program(0), shader(0), valid(false) {}

ComputeShader::~ComputeShader() {
    if (program) glDeleteProgram(program);
    if (shader) glDeleteShader(shader);
}

bool ComputeShader::loadShader(const char* source, const char* name) {
    shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1) {
            char* infoLog = (char*)malloc(infoLen);
            glGetShaderInfoLog(shader, infoLen, nullptr, infoLog);
            LOGE("Error compiling compute shader '%s':\n%s", name, infoLog);
            free(infoLog);
        }
        glDeleteShader(shader);
        shader = 0;
        return false;
    }

    program = glCreateProgram();
    glAttachShader(program, shader);
    glLinkProgram(program);

    GLint linked;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        GLint infoLen = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1) {
            char* infoLog = (char*)malloc(infoLen);
            glGetProgramInfoLog(program, infoLen, nullptr, infoLog);
            LOGE("Error linking compute shader program '%s':\n%s", name, infoLog);
            free(infoLog);
        }
        glDeleteProgram(program);
        program = 0;
        return false;
    }

    valid = true;
    LOGI("Compute shader '%s' loaded successfully", name);
    return true;
}

void ComputeShader::dispatch(GLuint x, GLuint y, GLuint z) {
    if (!valid) return;
    glUseProgram(program);
    glDispatchCompute(x, y, z);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
}

GLuint ComputeShader::createSSBO(GLsizeiptr size, const void* data, GLenum usage) {
    GLuint ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, usage);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    return ssbo;
}

void ComputeShader::bindSSBO(GLuint ssbo, GLuint bindingPoint) {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, ssbo);
}

void ComputeShader::unbind() {
    glUseProgram(0);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

// ------------------------------------------------------------
// Global pipeline functions (integrate with your filter engine)
// ------------------------------------------------------------

static ComputeShader* s_cullingShader = nullptr;
static ComputeShader* s_upscaleShader = nullptr;

void initComputeCulling() {
    // Culling shader source (see culling.comp)
    const char* cullSource = R"(
        #version 310 es
        layout(local_size_x = 64) in;
        layout(std430, binding = 0) buffer EntityPositions {
            vec4 positions[];
        };
        layout(std430, binding = 1) buffer Visibility {
            uint visible[];
        };
        layout(std140, binding = 2) uniform CameraData {
            mat4 viewProj;
            vec4 frustumPlanes[6];
        };
        bool isInside(vec4 worldPos) {
            vec4 clip = viewProj * worldPos;
            return (clip.x <= clip.w && clip.x >= -clip.w &&
                    clip.y <= clip.w && clip.y >= -clip.w &&
                    clip.z <= clip.w && clip.z >= 0.0);
        }
        void main() {
            uint id = gl_GlobalInvocationID.x;
            visible[id] = isInside(positions[id]) ? 1u : 0u;
        }
    )";
    s_cullingShader = new ComputeShader();
    if (!s_cullingShader->loadShader(cullSource, "EntityCulling")) {
        LOGE("Failed to load culling compute shader");
    }
}

void performEntityCulling(GLuint entityPositionsSSBO, GLuint outputVisibilitySSBO, int entityCount, const float* viewProjMatrix) {
    if (!s_cullingShader) return;
    s_cullingShader->bindSSBO(entityPositionsSSBO, 0);
    s_cullingShader->bindSSBO(outputVisibilitySSBO, 1);
    
    // Upload view-projection matrix as uniform buffer
    GLuint ubo;
    glGenBuffers(1, &ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferData(GL_UNIFORM_BUFFER, 16 * sizeof(float), viewProjMatrix, GL_STREAM_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, ubo);
    
    // Dispatch: one work group per 64 entities
    GLuint groups = (entityCount + 63) / 64;
    s_cullingShader->dispatch(groups, 1, 1);
    
    glDeleteBuffers(1, &ubo);
}

// Upscaling shader (bilinear) from lower resolution texture to output
static const char* upscaleSource = R"(
    #version 310 es
    layout(local_size_x = 16, local_size_y = 16) in;
    layout(binding = 0) uniform sampler2D inputTex;
    layout(rgba8, binding = 1) uniform writeonly image2D outputImg;
    uniform vec2 invTexSize;
    uniform vec2 scale;
    void main() {
        ivec2 outPos = ivec2(gl_GlobalInvocationID.xy);
        ivec2 outSize = imageSize(outputImg);
        if (outPos.x >= outSize.x || outPos.y >= outSize.y) return;
        vec2 uv = (vec2(outPos) + 0.5) / vec2(outSize);
        // Scale UV to sample from lower res input
        vec2 sampleUV = uv * scale;
        vec4 color = texture(inputTex, sampleUV);
        imageStore(outputImg, outPos, color);
    }
)";

void performUpscale(GLuint inputTexture, GLuint outputTexture, int width, int height, float scaleFactor) {
    if (!s_upscaleShader) {
        s_upscaleShader = new ComputeShader();
        if (!s_upscaleShader->loadShader(upscaleSource, "Upscale")) {
            LOGE("Failed to load upscale compute shader");
            return;
        }
    }
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTexture);
    glBindImageTexture(1, outputTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
    
    GLint invTexSizeLoc = glGetUniformLocation(s_upscaleShader->program, "invTexSize");
    GLint scaleLoc = glGetUniformLocation(s_upscaleShader->program, "scale");
    int smallW = width / scaleFactor;
    int smallH = height / scaleFactor;
    glUniform2f(invTexSizeLoc, 1.0f / smallW, 1.0f / smallH);
    glUniform2f(scaleLoc, (float)width / smallW, (float)height / smallH);
    
    s_upscaleShader->dispatch((width + 15) / 16, (height + 15) / 16, 1);
    s_upscaleShader->unbind();
}
