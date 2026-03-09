#include "Light.h"
#include <string>

// ========================================
// Light System Implementation
// ========================================
// NOTE: These implementations are currently unused since the Renderer
// directly calls GetPosition()/GetRadius() and uploads uniforms itself.
// This is a transitional design - ideally the renderer would call
// light->UploadToShader() for cleaner code.

void PointLight::UploadToShader(Shader* shader, int lightIndex) const {
    std::string base = "lights[" + std::to_string(lightIndex) + "].";

    glUniform1i(glGetUniformLocation(shader->GetProgram(), (base + "type").c_str()), (int)LightType::POINT);

    // Use explicit array conversion for clarity
    float posArr[3] = {position.x, position.y, position.z};
    glUniform3fv(glGetUniformLocation(shader->GetProgram(), (base + "position").c_str()), 1, posArr);

    float colArr[4] = {colour.x, colour.y, colour.z, colour.w};
    glUniform4fv(glGetUniformLocation(shader->GetProgram(), (base + "colour").c_str()), 1, colArr);

    glUniform1f(glGetUniformLocation(shader->GetProgram(), (base + "radius").c_str()), radius);
}

void DirectionalLight::UploadToShader(Shader* shader, int lightIndex) const {
    std::string base = "lights[" + std::to_string(lightIndex) + "].";

    glUniform1i(glGetUniformLocation(shader->GetProgram(), (base + "type").c_str()), (int)LightType::DIRECTIONAL);

    float dirArr[3] = {direction.x, direction.y, direction.z};
    glUniform3fv(glGetUniformLocation(shader->GetProgram(), (base + "direction").c_str()), 1, dirArr);

    float colArr[4] = {colour.x, colour.y, colour.z, colour.w};
    glUniform4fv(glGetUniformLocation(shader->GetProgram(), (base + "colour").c_str()), 1, colArr);
}

void SpotLight::UploadToShader(Shader* shader, int lightIndex) const {
    std::string base = "lights[" + std::to_string(lightIndex) + "].";

    glUniform1i(glGetUniformLocation(shader->GetProgram(), (base + "type").c_str()), (int)LightType::SPOT);

    float posArr[3] = {position.x, position.y, position.z};
    glUniform3fv(glGetUniformLocation(shader->GetProgram(), (base + "position").c_str()), 1, posArr);

    float dirArr[3] = {direction.x, direction.y, direction.z};
    glUniform3fv(glGetUniformLocation(shader->GetProgram(), (base + "direction").c_str()), 1, dirArr);

    float colArr[4] = {colour.x, colour.y, colour.z, colour.w};
    glUniform4fv(glGetUniformLocation(shader->GetProgram(), (base + "colour").c_str()), 1, colArr);

    glUniform1f(glGetUniformLocation(shader->GetProgram(), (base + "radius").c_str()), radius);
    glUniform1f(glGetUniformLocation(shader->GetProgram(), (base + "cutoffAngle").c_str()), cutoffAngle);
}
