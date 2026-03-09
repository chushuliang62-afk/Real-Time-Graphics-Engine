#pragma once

#include "VoxelTreeTypes.h"
#include "../nclgl/CustomTerrain.h"
#include "../nclgl/Vector3.h"
#include <cmath>

// ========================================
// Custom Terrain Adapter
// ========================================
// Adapts CustomTerrain to ITerrainQuery interface
// Allows VoxelTreeSystem to query terrain without tight coupling
class CustomTerrainAdapter : public ITerrainQuery {
public:
    CustomTerrainAdapter(CustomTerrain* terrain)
        : terrain(terrain) {
    }

    virtual ~CustomTerrainAdapter() = default;

    virtual float GetHeightAt(float x, float z) const override {
        if (!terrain) return 0.0f;
        return terrain->GetHeightAt(x, z);
    }

    virtual Vector3 GetNormalAt(float x, float z) const override {
        if (!terrain) return Vector3(0, 1, 0);

        // Estimate normal using finite differences
        const float delta = 1.0f;

        float hL = terrain->GetHeightAt(x - delta, z);
        float hR = terrain->GetHeightAt(x + delta, z);
        float hD = terrain->GetHeightAt(x, z - delta);
        float hU = terrain->GetHeightAt(x, z + delta);

        // Calculate tangent vectors
        Vector3 tangentX(2.0f * delta, hR - hL, 0.0f);
        Vector3 tangentZ(0.0f, hU - hD, 2.0f * delta);

        // Cross product gives normal
        Vector3 normal = Vector3::Cross(tangentZ, tangentX);
        normal.Normalise();

        return normal;
    }

    virtual TerrainType GetTerrainTypeAt(float x, float z) const override {
        if (!terrain) return TerrainType::UNKNOWN;

        // Get height at this position
        float height = terrain->GetHeightAt(x, z);

        // Get slope angle
        Vector3 normal = GetNormalAt(x, z);
        float slopeAngle = acosf(Vector3::Dot(normal, Vector3(0, 1, 0))) * (180.0f / 3.14159265f);

        // Based on CustomTerrain.cpp analysis:
        // - Water level: 10.0f
        // - Basin (lake): height < 10.0f
        // - Shore area: 10.0f to 30.0f
        // - Plain: 25-30 units, flat
        // - Hill slopes: 30-320 units, with slope
        // - Plateau top: 312-320 units, relatively flat

        if (height < 10.0f) {
            return TerrainType::WATER;  // Below water level = lake
        } else if (height < 30.0f) {
            // Shore and low plain area
            if (slopeAngle < 5.0f) {
                return TerrainType::PLAIN;  // Flat shore/plain
            } else {
                return TerrainType::HILLSIDE;  // Sloped shore
            }
        } else if (height < 300.0f) {
            // Main hill slopes - PERFECT FOR TREES!
            if (slopeAngle > 5.0f && slopeAngle < 50.0f) {
                return TerrainType::HILLSIDE;  // Ideal tree location
            } else if (slopeAngle <= 5.0f) {
                return TerrainType::GRASSLAND;  // Flat areas on hill
            } else {
                return TerrainType::MOUNTAIN;  // Too steep
            }
        } else {
            // Plateau top (312-320) - relatively flat
            if (slopeAngle < 5.0f) {
                return TerrainType::GRASSLAND;  // Flat plateau
            } else {
                return TerrainType::HILLSIDE;  // Edges of plateau
            }
        }
    }

    virtual void GetTerrainBounds(float& minX, float& maxX, float& minZ, float& maxZ) const override {
        if (!terrain) {
            minX = maxX = minZ = maxZ = 0.0f;
            return;
        }

        Vector3 size = terrain->GetTerrainSize();

        // CustomTerrain coordinates range from [0, size], NOT centered at origin
        minX = 0.0f;
        maxX = size.x;
        minZ = 0.0f;
        maxZ = size.z;
    }

private:
    CustomTerrain* terrain;
};
