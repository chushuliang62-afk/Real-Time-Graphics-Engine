#pragma once

#include "Mesh.h"
#include <string>

class CustomTerrain : public Mesh {
public:
    CustomTerrain(int width, int depth, float heightScale = 1.0f);
    ~CustomTerrain(void) {}

    Vector3 GetTerrainSize() const { return terrainSize; }
    float GetHeightAt(float x, float z) const;

protected:
    void GenerateTerrain(int width, int depth, float heightScale);
    float CalculateHeight(float x, float z, int width, int depth);

    Vector3 terrainSize;
    int gridWidth;
    int gridDepth;
    float* heightData;  // Store height values for lookups
};
