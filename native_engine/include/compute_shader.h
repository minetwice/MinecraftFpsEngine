#ifndef COMPUTE_SHADER_H
#define COMPUTE_SHADER_H

#include <GLES3/gl31.h>
#include <vector>

class ComputeShader {
public:
    ComputeShader();
    ~ComputeShader();

    // Load and compile compute shader from source string
    bool loadShader(const char* source, const char* name);
    
    // Dispatch compute shader with given work group dimensions
    void dispatch(GLuint x, GLuint y, GLuint z);
    
    // Create and bind SSBO (Shader Storage Buffer Object)
    GLuint createSSBO(GLsizeiptr size, const void* data, GLenum usage);
    
    // Bind SSBO to a binding point
    void bindSSBO(GLuint ssbo, GLuint bindingPoint);
    
    // Unbind and cleanup
    void unbind();

private:
    GLuint program;
    GLuint shader;
    bool valid;
};

// Global functions for your engine's culling pipeline
void initComputeCulling();
void performEntityCulling(GLuint entityPositionsSSBO, GLuint outputVisibilitySSBO, int entityCount, const float* viewProjMatrix);
void performUpscale(GLuint inputTexture, GLuint outputTexture, int width, int height, float scale);

#endif
