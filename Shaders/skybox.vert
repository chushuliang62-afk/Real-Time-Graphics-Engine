#version 330 core

uniform mat4 viewMatrix;
uniform mat4 projMatrix;

in vec3 position;

out Vertex {
    vec3 viewDir;
} OUT;

void main(void) {
    // Output quad in NDC space with depth at far plane
    gl_Position = vec4(position.xy, 0.9999, 1.0);

    // Remove translation from view matrix
    mat3 viewRot = mat3(viewMatrix);

    // Compute view direction: inverse projection to get view space ray,
    // then inverse view rotation to get world space direction
    mat4 invProj = inverse(projMatrix);
    vec4 viewSpacePos = invProj * vec4(position.xy, 1.0, 1.0);
    vec3 viewSpaceDir = viewSpacePos.xyz / viewSpacePos.w;

    OUT.viewDir = inverse(viewRot) * viewSpaceDir;
}
