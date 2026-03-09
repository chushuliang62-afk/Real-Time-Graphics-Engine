#version 330 core
uniform sampler2D diffuseTex;

in Vertex {
   vec2 texCoord;
} IN;

out vec4 fragColour;
void main(void) {
  // Use texture color for realistic rendering
  fragColour = texture(diffuseTex, IN.texCoord);

  // If no texture is bound, use a neutral gray instead of debug red
  if (fragColour.rgb == vec3(0.0)) {
    fragColour = vec4(0.7, 0.7, 0.7, 1.0);
  }
}
