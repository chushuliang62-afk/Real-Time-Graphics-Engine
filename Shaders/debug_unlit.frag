#version 330 core

// Simple debug shader - displays solid color only

uniform vec4 debugColour;

out vec4 fragColour;

void main(void) {
    fragColour = debugColour;
}
