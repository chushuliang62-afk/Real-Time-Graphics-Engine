#include "CustomTerrain.h"
#include <iostream>
#include <cmath>

CustomTerrain::CustomTerrain(int width, int depth, float heightScale) {
    gridWidth = width;
    gridDepth = depth;

    // Allocate height data
    heightData = new float[width * depth];

    GenerateTerrain(width, depth, heightScale);
}

float CustomTerrain::CalculateHeight(float x, float z, int width, int depth) {
    // Normalize coordinates to 0-1 range
    float nx = x / (float)width;
    float nz = z / (float)depth;

    // Define terrain layout:
    // - Bottom half (z < 0.5): Flat plain
    // - Top half (z >= 0.5): Hill slope
    // - Hill center around (0.5, 0.75)

    float height = 0.0f;

    // Hill center position
    float hillCenterX = 0.5f;
    float hillCenterZ = 0.75f;

    // Distance from hill center
    float dx = nx - hillCenterX;
    float dz = nz - hillCenterZ;
    float distFromHill = sqrt(dx * dx + dz * dz);

    // Create hill in upper portion
    if (nz >= 0.4f) {  // Hill area starts at 40% depth
        // Main hill
        float hillRadius = 0.3f;
        if (distFromHill < hillRadius) {
            // Smooth hill using cosine interpolation
            float hillFactor = (cos(distFromHill / hillRadius * 3.14159f) + 1.0f) * 0.5f;
            height = hillFactor * 320.0f;  // Max hill height = 320 units (+200 from original 120)
        }

        // MODIFIED: Larger plateau to accommodate 10x scaled temple
        // Gentle rounded top for natural appearance
        float plateauRadius = 0.12f;  // Increased to support larger temple
        if (distFromHill < plateauRadius) {
            // Gentle rounded top instead of completely flat
            float plateauFactor = (cos(distFromHill / plateauRadius * 3.14159f) + 1.0f) * 0.5f;
            height = 312.0f + plateauFactor * 8.0f;  // Range: 312-320 (gentle dome, +200 from original)
        }

        // River valley cutting through hill (from top to bottom)
        float riverCenterX = 0.55f;  // Slightly offset from center
        float riverDistX = abs(nx - riverCenterX);
        float riverWidth = 0.04f;

        // River only flows from hilltop down
        if (riverDistX < riverWidth && nz > 0.6f) {
            // Create river valley
            float riverDepthFactor = 1.0f - (riverDistX / riverWidth);
            float riverCutDepth = riverDepthFactor * 15.0f;  // River cuts 15 units deep
            height = std::max(0.0f, height - riverCutDepth);
        }
    } else {
        // Lower half: Create a large basin (lake bed) instead of flat plain
        // Basin center around (0.25, 0.2) - in the lower-left quadrant
        float basinCenterX = 0.25f;
        float basinCenterZ = 0.2f;

        float basinDx = nx - basinCenterX;
        float basinDz = nz - basinCenterZ;
        float distFromBasin = sqrt(basinDx * basinDx + basinDz * basinDz);

        // Basin parameters - VERY DEEP BASIN (REDUCED SIZE)
        float basinRadius = 0.18f;     // Basin radius (covers ~18% of terrain) - REDUCED for better containment
        float basinDepth = 90.0f;      // Basin depth - VERY DEEP: 90 units (increased 50 more)
        float waterLevel = 10.0f;      // Water surface height - LOWERED
        float surroundingHeight = 30.0f;  // Area around basin - RAISED for contrast

        if (distFromBasin < basinRadius) {
            // Inside basin - underwater terrain (VERY deep bowl)
            // Create smooth bowl shape, all BELOW water level
            float basinFactor = (cos(distFromBasin / basinRadius * 3.14159f) + 1.0f) * 0.5f;
            // Ranges from -80 (center, extremely deep) to 10 (edge, at water level)
            // Total depth: 90 units below water surface
            height = waterLevel - basinDepth * basinFactor;
        } else if (distFromBasin < basinRadius * 1.5f) {
            // Lake shore - steep rise from water to surrounding land
            float shoreDist = distFromBasin - basinRadius;
            float shoreRange = basinRadius * 0.5f;
            float shoreFactor = shoreDist / shoreRange;
            // Steep slope from water level (10) to surrounding height (30)
            height = waterLevel + shoreFactor * (surroundingHeight - waterLevel);
        } else {
            // Outer plain area - slightly elevated
            height = surroundingHeight * 0.85f;  // ~25.5 units
        }

        // Gentle transition from plain to hill
        float transitionStart = 0.3f;
        float transitionEnd = 0.4f;
        if (nz > transitionStart && nz < transitionEnd) {
            float transitionFactor = (nz - transitionStart) / (transitionEnd - transitionStart);
            // Smooth transition
            transitionFactor = (cos((1.0f - transitionFactor) * 3.14159f) + 1.0f) * 0.5f;
            float hillStart = 20.0f;
            height = height + transitionFactor * (hillStart - height);
        }
    }

    return height;  // Don't apply heightScale here, it's applied when storing vertices
}

void CustomTerrain::GenerateTerrain(int width, int depth, float heightScale) {
    numVertices = width * depth;
    numIndices = (width - 1) * (depth - 1) * 6;

    vertices = new Vector3[numVertices];
    textureCoords = new Vector2[numVertices];
    colours = new Vector4[numVertices];
    normals = new Vector3[numVertices];
    tangents = new Vector4[numVertices];
    indices = new GLuint[numIndices];

    // Terrain size: 2000 x 2000 units
    float terrainWidth = 2000.0f;
    float terrainDepth = 2000.0f;
    terrainSize = Vector3(terrainWidth, 255.0f, terrainDepth);

    // Generate vertices
    for (int z = 0; z < depth; ++z) {
        for (int x = 0; x < width; ++x) {
            int offset = (z * width) + x;

            float height = CalculateHeight((float)x, (float)z, width, depth);
            heightData[offset] = height;

            vertices[offset] = Vector3(
                (float)x * terrainWidth / (float)(width - 1),
                height * heightScale,  // Apply height scale here
                (float)z * terrainDepth / (float)(depth - 1)
            );

            textureCoords[offset] = Vector2(
                (float)x / (float)(width - 1),
                (float)z / (float)(depth - 1)
            );

            colours[offset] = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        }
    }

    // Generate indices
    int i = 0;
    for (int z = 0; z < depth - 1; ++z) {
        for (int x = 0; x < width - 1; ++x) {
            int a = (z * width) + x;
            int b = (z * width) + (x + 1);
            int c = ((z + 1) * width) + (x + 1);
            int d = ((z + 1) * width) + x;

            indices[i++] = a;
            indices[i++] = c;
            indices[i++] = b;

            indices[i++] = c;
            indices[i++] = a;
            indices[i++] = d;
        }
    }

    // Generate normals
    for (int i = 0; i < numVertices; ++i) {
        normals[i] = Vector3(0, 0, 0);
    }

    // Calculate face normals and accumulate
    for (int i = 0; i < numIndices; i += 3) {
        Vector3& a = vertices[indices[i]];
        Vector3& b = vertices[indices[i + 1]];
        Vector3& c = vertices[indices[i + 2]];

        Vector3 normal = Vector3::Cross(b - a, c - a);
        normal.Normalise();

        normals[indices[i]] += normal;
        normals[indices[i + 1]] += normal;
        normals[indices[i + 2]] += normal;
    }

    // Normalize all normals
    for (int i = 0; i < numVertices; ++i) {
        normals[i].Normalise();
    }

    // Generate tangents manually (simple version)
    for (int i = 0; i < numVertices; ++i) {
        tangents[i] = Vector4(1.0f, 0.0f, 0.0f, 1.0f);  // Default tangent pointing along X axis
    }

    BufferData();

    std::cout << "Custom terrain generated: " << width << "x" << depth
              << " (" << numVertices << " vertices, " << numIndices / 3 << " triangles)" << std::endl;
}

float CustomTerrain::GetHeightAt(float x, float z) const {
    // Convert world coordinates to grid coordinates
    float gridX = (x / terrainSize.x) * (float)(gridWidth - 1);
    float gridZ = (z / terrainSize.z) * (float)(gridDepth - 1);

    // Clamp to valid range
    if (gridX < 0 || gridX >= gridWidth - 1 || gridZ < 0 || gridZ >= gridDepth - 1) {
        return 0.0f;
    }

    // Bilinear interpolation
    int x0 = (int)gridX;
    int z0 = (int)gridZ;
    int x1 = x0 + 1;
    int z1 = z0 + 1;

    float fx = gridX - x0;
    float fz = gridZ - z0;

    float h00 = heightData[z0 * gridWidth + x0];
    float h10 = heightData[z0 * gridWidth + x1];
    float h01 = heightData[z1 * gridWidth + x0];
    float h11 = heightData[z1 * gridWidth + x1];

    float h0 = h00 * (1.0f - fx) + h10 * fx;
    float h1 = h01 * (1.0f - fx) + h11 * fx;

    return h0 * (1.0f - fz) + h1 * fz;
}
