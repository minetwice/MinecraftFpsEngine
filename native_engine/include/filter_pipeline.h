#ifndef FILTER_PIPELINE_H
#define FILTER_PIPELINE_H

#include <GLES3/gl3.h>

void init_pipeline();
void process_draw_call(GLenum mode, GLsizei count, GLenum type, const void* indices);
void end_frame();

#endif
