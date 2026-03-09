#include "TerrainSampler.h"
#include <algorithm>
#include <cmath>

#define PI 3.14159265358979323846f

TerrainSampler::TerrainSampler(ITerrainQuery* terrain)
    : terrain(terrain) {
}

std::vector<Vector3> TerrainSampler::SampleTreePositions(
    int numSamples,
    float minSlope,
    float maxSlope,
    float minSpacing,
    unsigned int seed,
    bool useRegionFilter,
    float regionMinX,
    float regionMaxX,
    float regionMinZ,
    float regionMaxZ,
    float regionMinHeight,
    float regionMaxHeight
) {
    std::vector<Vector3> validPositions;

    if (!terrain) {
        return validPositions;
    }

    // Get terrain bounds
    float minX, maxX, minZ, maxZ;
    terrain->GetTerrainBounds(minX, maxX, minZ, maxZ);

    // Apply region filtering if enabled
    if (useRegionFilter) {
        // Convert normalized coords to world coords
        float terrainWidth = maxX - minX;
        float terrainDepth = maxZ - minZ;

        minX = minX + regionMinX * terrainWidth;
        maxX = minX + regionMaxX * terrainWidth;
        minZ = minZ + regionMinZ * terrainDepth;
        maxZ = minZ + regionMaxZ * terrainDepth;

        // Region filtering applied
    }

    // Generate random samples
    int attempts = 0;
    int maxAttempts = numSamples * 20;  // Try up to 20x for region filtering

    while (validPositions.size() < (size_t)numSamples && attempts < maxAttempts) {
        attempts++;

        // Random position within (possibly filtered) bounds
        float x = RandomRange(minX, maxX, seed);
        float z = RandomRange(minZ, maxZ, seed);

        // Get height at this position
        float y = terrain->GetHeightAt(x, z);
        Vector3 position(x, y, z);

        // Apply height filter if region filtering enabled
        if (useRegionFilter) {
            if (y < regionMinHeight || y > regionMaxHeight) {
                continue;  // Outside height range
            }
        }

        // Check if position is valid
        if (IsValidTreePosition(position, minSlope, maxSlope)) {
            // Check spacing with existing trees
            if (!IsTooClose(position, validPositions, minSpacing)) {
                validPositions.push_back(position);
            }
        }
    }

    return validPositions;
}

bool TerrainSampler::IsValidTreePosition(
    const Vector3& position,
    float minSlope,
    float maxSlope
) const {
    if (!terrain) {
        return false;
    }

    // Get terrain type
    TerrainType type = terrain->GetTerrainTypeAt(position.x, position.z);

    // Reject water and flat plains
    if (type == TerrainType::LAKE || type == TerrainType::WATER ||
        type == TerrainType::PLAIN || type == TerrainType::GRASSLAND) {
        return false;
    }

    // Check slope angle
    float slope = GetSlopeAngle(position);

    // Must be within slope range
    if (slope < minSlope || slope > maxSlope) {
        return false;
    }

    return true;
}

float TerrainSampler::GetSlopeAngle(const Vector3& position) const {
    if (!terrain) {
        return 0.0f;
    }

    // Get normal at position
    Vector3 normal = terrain->GetNormalAt(position.x, position.z);

    // Calculate angle between normal and up vector (0, 1, 0)
    Vector3 up(0, 1, 0);
    float dot = Vector3::Dot(normal, up);

    // Clamp to avoid acos domain errors
    dot = std::max(-1.0f, std::min(1.0f, dot));

    // Calculate angle in radians, then convert to degrees
    float angleRad = acosf(dot);
    float angleDeg = angleRad * (180.0f / PI);

    return angleDeg;
}

bool TerrainSampler::IsTooClose(
    const Vector3& pos,
    const std::vector<Vector3>& existing,
    float minSpacing
) const {
    float minSpacingSq = minSpacing * minSpacing;

    for (const auto& existingPos : existing) {
        // Calculate squared distance (avoid sqrt)
        float dx = pos.x - existingPos.x;
        float dz = pos.z - existingPos.z;
        float distSq = dx * dx + dz * dz;

        if (distSq < minSpacingSq) {
            return true;  // Too close
        }
    }

    return false;  // Safe distance
}

float TerrainSampler::RandomFloat(unsigned int& seed) const {
    // Simple LCG random number generator
    seed = (seed * 1103515245 + 12345) & 0x7fffffff;
    return (float)seed / (float)0x7fffffff;
}

float TerrainSampler::RandomRange(float min, float max, unsigned int& seed) const {
    return min + RandomFloat(seed) * (max - min);
}
