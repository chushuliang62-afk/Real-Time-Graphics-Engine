#version 330 core

// Shadow pass: depth is written automatically by OpenGL
// No color output needed (GL_NONE is set in FBO setup)

void main(void) {
    // Empty - depth is written automatically during depth test
    // Fragment shader is required even though we don't output anything
}
