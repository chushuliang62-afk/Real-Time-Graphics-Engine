#version 330 core

// Input from vertex shader
in Vertex {
    vec3 worldPos;
    vec2 texCoord;
    vec3 normal;
    vec3 tangent;
    vec3 binormal;
    vec4 colour;
} IN;

// Uniforms
uniform sampler2D diffuseTex;
uniform sampler2D normalTex;
uniform int useNormalMap;

uniform vec3 lightPos;
uniform vec4 lightColour;
uniform vec3 cameraPos;

// Output
out vec4 fragColour;

void main(void) {
    // Sample albedo texture
    vec4 albedo = texture(diffuseTex, IN.texCoord);

    // Get normal (either from normal map or vertex normal)
    vec3 normal;
    if (useNormalMap == 1) {
        // Sample normal map
        vec3 tangentNormal = texture(normalTex, IN.texCoord).xyz;
        tangentNormal = normalize(tangentNormal * 2.0 - 1.0);

        // Transform to world space using TBN matrix
        mat3 TBN = mat3(
            normalize(IN.tangent),
            normalize(IN.binormal),
            normalize(IN.normal)
        );
        normal = normalize(TBN * tangentNormal);
    } else {
        normal = normalize(IN.normal);
    }

    // Lighting calculation (Phong lighting model)
    vec3 lightDir = normalize(lightPos - IN.worldPos);
    vec3 viewDir = normalize(cameraPos - IN.worldPos);
    vec3 halfDir = normalize(lightDir + viewDir);

    // Ambient
    vec3 ambient = 0.2 * albedo.rgb;

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * albedo.rgb * lightColour.rgb;

    // Specular
    float spec = pow(max(dot(normal, halfDir), 0.0), 32.0);
    vec3 specular = spec * lightColour.rgb * 0.3;

    // Combine
    vec3 colour = ambient + diffuse + specular;

    // Output
    fragColour = vec4(colour, albedo.a);
}
