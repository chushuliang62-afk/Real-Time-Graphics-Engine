#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

// ========================================
// Tree Configuration Loader
// ========================================
// Loads tree system configuration from file
// Removes all hardcoding!
class TreeConfigLoader {
public:
    // Generation parameters
    int treeCount = 50;
    float minSlope = 5.0f;
    float maxSlope = 45.0f;
    float minSpacing = 20.0f;
    unsigned int randomSeed = 42;

    // Tree shape parameters
    int minHeight = 4;
    int maxHeight = 7;
    int canopyRadius = 2;
    int canopyHeight = 3;
    float canopyDensity = 0.75f;
    bool sphericalCanopy = true;
    float blockSize = 2.0f;

    // Growth parameters
    bool enableGrowth = true;
    float growthSpeed = 0.1f;
    float initialGrowth = 0.3f;

    // Region filtering parameters (normalized 0-1 coordinates)
    bool enableRegionFilter = false;
    float regionMinX = 0.0f;
    float regionMaxX = 1.0f;
    float regionMinZ = 0.0f;
    float regionMaxZ = 1.0f;
    float regionMinHeight = 0.0f;
    float regionMaxHeight = 1000.0f;

    // Load from file
    bool LoadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "WARNING: Could not open tree config file: " << filename << std::endl;
            std::cerr << "Using default values." << std::endl;
            return false;
        }

        std::string line;
        std::string currentSection;

        while (std::getline(file, line)) {
            // Trim whitespace
            line.erase(0, line.find_first_not_of(" \t"));
            line.erase(line.find_last_not_of(" \t") + 1);

            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') {
                continue;
            }

            // Check for section header
            if (line[0] == '[' && line[line.length() - 1] == ']') {
                currentSection = line.substr(1, line.length() - 2);
                continue;
            }

            // Parse key = value
            size_t eqPos = line.find('=');
            if (eqPos == std::string::npos) {
                continue;
            }

            std::string key = line.substr(0, eqPos);
            std::string value = line.substr(eqPos + 1);

            // Trim key and value
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            // Remove inline comments
            size_t commentPos = value.find('#');
            if (commentPos != std::string::npos) {
                value = value.substr(0, commentPos);
                value.erase(value.find_last_not_of(" \t") + 1);
            }

            // Set values based on section and key
            if (currentSection == "Generation") {
                if (key == "tree_count") treeCount = std::stoi(value);
                else if (key == "min_slope") minSlope = std::stof(value);
                else if (key == "max_slope") maxSlope = std::stof(value);
                else if (key == "min_spacing") minSpacing = std::stof(value);
                else if (key == "random_seed") randomSeed = std::stoul(value);
            }
            else if (currentSection == "TreeShape") {
                if (key == "min_height") minHeight = std::stoi(value);
                else if (key == "max_height") maxHeight = std::stoi(value);
                else if (key == "canopy_radius") canopyRadius = std::stoi(value);
                else if (key == "canopy_height") canopyHeight = std::stoi(value);
                else if (key == "canopy_density") canopyDensity = std::stof(value);
                else if (key == "spherical_canopy") sphericalCanopy = (value == "true" || value == "1");
                else if (key == "block_size") blockSize = std::stof(value);
            }
            else if (currentSection == "Growth") {
                if (key == "enable_growth") enableGrowth = (value == "true" || value == "1");
                else if (key == "growth_speed") growthSpeed = std::stof(value);
                else if (key == "initial_growth") initialGrowth = std::stof(value);
            }
            else if (currentSection == "Region") {
                if (key == "enable_region_filter") enableRegionFilter = (value == "true" || value == "1");
                else if (key == "region_min_x") regionMinX = std::stof(value);
                else if (key == "region_max_x") regionMaxX = std::stof(value);
                else if (key == "region_min_z") regionMinZ = std::stof(value);
                else if (key == "region_max_z") regionMaxZ = std::stof(value);
                else if (key == "min_height") regionMinHeight = std::stof(value);
                else if (key == "max_height") regionMaxHeight = std::stof(value);
            }
        }

        file.close();
        // Configuration loaded successfully
        return true;
    }

    void PrintConfig() const {
        // Debug output disabled for release
    }
};
