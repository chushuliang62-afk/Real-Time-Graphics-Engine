#pragma once

/*
 * LightingConfig.h
 *
 * Lighting System Configuration Loader
 *
 * Features:
 * - Load deferred rendering and lighting parameters from config file
 * - Support multiple point light configuration
 * - Zero hardcoding, fully config-driven
 *
 * Configuration file format (INI-style):
 * [DeferredRendering]
 * enable_deferred = true
 * use_hdr = true
 *
 * [Ambient]
 * colour = 0.1, 0.1, 0.15
 * intensity = 0.3
 *
 * [PointLight0]
 * position = 100, 50, 200
 * colour = 1.0, 0.7, 0.3
 * intensity = 2.0
 * ...
 */

#include "PointLight.h"
#include "../nclgl/Vector3.h"
#include "../nclgl/Vector4.h"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

class LightingConfig {
public:
    // Deferred rendering settings
    bool enableDeferred = true;
    bool useHDR = true;
    float hdrExposure = 1.0f;

    // Ambient light
    Vector3 ambientColour = Vector3(0.1f, 0.1f, 0.15f);
    float ambientIntensity = 0.3f;

    // Point light array
    std::vector<DeferredPointLight> pointLights;

    /**
     * Load configuration from file
     * @param filename Config file path
     * @return true on success, false on failure
     */
    bool LoadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "[LightingConfig] ERROR: Cannot open file: " << filename << std::endl;
            return false;
        }

        // Loading message (init-time only)

        std::string line;
        std::string currentSection;
        DeferredPointLight currentLight;
        bool parsingLight = false;
        int lightIndex = 0;

        while (std::getline(file, line)) {
            // Remove comments and whitespace
            size_t commentPos = line.find('#');
            if (commentPos != std::string::npos) {
                line = line.substr(0, commentPos);
            }
            line = Trim(line);

            if (line.empty()) continue;

            // Detect section ([SectionName])
            if (line.front() == '[' && line.back() == ']') {
                // If previously parsing a light, save it
                if (parsingLight) {
                    pointLights.push_back(currentLight);
                    currentLight = DeferredPointLight();  // Reset
                    parsingLight = false;
                }

                currentSection = line.substr(1, line.size() - 2);
                currentSection = Trim(currentSection);

                // Check if it's a point light section
                if (currentSection.find("PointLight") == 0) {
                    parsingLight = true;
                    lightIndex++;
                }

                continue;
            }

            // Parse key-value pair (key = value)
            size_t equalPos = line.find('=');
            if (equalPos == std::string::npos) continue;

            std::string key = Trim(line.substr(0, equalPos));
            std::string value = Trim(line.substr(equalPos + 1));

            // Handle based on current section
            if (currentSection == "DeferredRendering") {
                ParseDeferredRenderingParam(key, value);
            }
            else if (currentSection == "Ambient") {
                ParseAmbientParam(key, value);
            }
            else if (parsingLight) {
                ParsePointLightParam(key, value, currentLight);
            }
        }

        // Save the last light
        if (parsingLight) {
            pointLights.push_back(currentLight);
        }

        file.close();

        return true;
    }

private:
    void ParseDeferredRenderingParam(const std::string& key, const std::string& value) {
        if (key == "enable_deferred") {
            enableDeferred = ParseBool(value);
        }
        else if (key == "use_hdr") {
            useHDR = ParseBool(value);
        }
        else if (key == "hdr_exposure") {
            hdrExposure = std::stof(value);
        }
    }

    void ParseAmbientParam(const std::string& key, const std::string& value) {
        if (key == "colour" || key == "color") {
            ambientColour = ParseVector3(value);
        }
        else if (key == "intensity") {
            ambientIntensity = std::stof(value);
        }
    }

    void ParsePointLightParam(const std::string& key, const std::string& value, DeferredPointLight& light) {
        if (key == "position") {
            light.position = ParseVector3(value);
        }
        else if (key == "colour" || key == "color") {
            Vector3 col = ParseVector3(value);
            light.colour = Vector4(col.x, col.y, col.z, 1.0f);
        }
        else if (key == "intensity") {
            light.intensity = std::stof(value);
            light.currentIntensity = light.intensity;
        }
        else if (key == "radius") {
            light.CalculateAttenuationFromRadius(std::stof(value));
        }
        else if (key == "constant") {
            light.constant = std::stof(value);
        }
        else if (key == "linear") {
            light.linear = std::stof(value);
        }
        else if (key == "quadratic") {
            light.quadratic = std::stof(value);
        }
        else if (key == "anim_type") {
            std::string animStr = Trim(value);
            std::transform(animStr.begin(), animStr.end(), animStr.begin(), ::tolower);
            if (animStr == "flicker") {
                light.animType = LightAnimationType::FLICKER;
            }
            else if (animStr == "pulse") {
                light.animType = LightAnimationType::PULSE;
            }
            else if (animStr == "random") {
                light.animType = LightAnimationType::RANDOM;
            }
            else {
                light.animType = LightAnimationType::NONE;
            }
        }
        else if (key == "anim_speed") {
            light.animSpeed = std::stof(value);
        }
        else if (key == "anim_amplitude") {
            light.animAmplitude = std::stof(value);
        }
        else if (key == "anim_phase") {
            light.animPhase = std::stof(value);
        }
    }

    // Utility functions
    static std::string Trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, last - first + 1);
    }

    static bool ParseBool(const std::string& value) {
        std::string lower = value;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        return (lower == "true" || lower == "1" || lower == "yes" || lower == "on");
    }

    static Vector3 ParseVector3(const std::string& value) {
        std::stringstream ss(value);
        Vector3 result;
        char comma;
        ss >> result.x >> comma >> result.y >> comma >> result.z;
        return result;
    }
};
