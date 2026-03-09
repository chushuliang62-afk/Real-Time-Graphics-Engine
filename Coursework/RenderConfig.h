#pragma once

// ========================================
// RENDER CONFIGURATION CONSTANTS
// ========================================
// This file defines all "magic numbers" used throughout the renderer
// to improve code readability and maintainability.
// Eliminates hardcoded constants scattered throughout the codebase.

namespace RenderConfig {
    // ========================================
    // Camera System Settings
    // ========================================
    constexpr float CAMERA_NEAR_PLANE = 1.0f;
    constexpr float CAMERA_FAR_PLANE = 15000.0f;
    constexpr float CAMERA_FOV = 45.0f;                 // Field of view in degrees
    constexpr float CAMERA_INITIAL_PITCH = -30.0f;
    constexpr float CAMERA_INITIAL_YAW = 225.0f;

    // ========================================
    // Terrain System Settings
    // ========================================
    constexpr int TERRAIN_GRID_WIDTH = 256;
    constexpr int TERRAIN_GRID_HEIGHT = 256;
    constexpr float TERRAIN_GRID_SCALE = 1.0f;

    // ========================================
    // Lighting Settings
    // ========================================
    // Sun direction (normalized vector pointing to sun)
    constexpr float SUN_DIRECTION_X = -0.3f;
    constexpr float SUN_DIRECTION_Y = -0.8f;
    constexpr float SUN_DIRECTION_Z = -0.3f;

    // Sun color (warm yellow sunlight)
    constexpr float SUN_COLOR_R = 1.0f;
    constexpr float SUN_COLOR_G = 0.95f;
    constexpr float SUN_COLOR_B = 0.85f;
    constexpr float SUN_COLOR_A = 1.0f;

    // ========================================
    // Shadow System Settings
    // ========================================
    constexpr int SHADOW_MAP_SIZE = 2048;               // Shadow map resolution (balance between quality and performance)
    constexpr float SHADOW_LIGHT_RANGE = 5000.0f;
    constexpr float SHADOW_NEAR_PLANE = 0.1f;
    constexpr float SHADOW_FAR_PLANE = 10000.0f;

    // ========================================
    // Grass Rendering Settings
    // ========================================
    constexpr int GRASS_PATCH_COUNT = 25000;
    constexpr int GRASS_BLADES_PER_PATCH = 20;

    // Grass exclusion zones
    constexpr float GRASS_HILL_EXCLUSION_RADIUS = 650.0f;
    constexpr float GRASS_BASIN_RADIUS_MULTIPLIER = 0.18f;
    constexpr float GRASS_BASIN_BUFFER = 100.0f;
    constexpr float GRASS_EDGE_MARGIN = 150.0f;

    // Grass height constraints
    constexpr float GRASS_MAX_HEIGHT = 40.0f;
    constexpr float GRASS_MIN_HEIGHT = 18.0f;           // Shore level

    // Grass patch parameters
    constexpr float GRASS_PATCH_SPREAD = 30.0f;
    constexpr float GRASS_SCALE_MIN = 0.7f;
    constexpr float GRASS_SCALE_VARIATION = 0.6f;       // 0.7 to 1.3 range
    constexpr float GRASS_ROTATION_RANGE = 360.0f;

    // Grass blade dimensions
    constexpr float GRASS_BLADE_HALF_WIDTH = 8.0f;
    constexpr float GRASS_BLADE_HEIGHT = 16.0f;

    // ========================================
    // Water Rendering Settings
    // ========================================
    constexpr float WATER_SURFACE_HEIGHT = 10.0f;
    constexpr float WATER_RADIUS = 360.0f;
    constexpr float WATER_CENTER_X = 500.0f;
    constexpr float WATER_CENTER_Z = 400.0f;

    // Default water parameters (if not specified)
    constexpr float DEFAULT_WATER_HEIGHT = 100.0f;
    constexpr float DEFAULT_WATER_SIZE = 2000.0f;

    // ========================================
    // Temple/GLTF Model Settings
    // ========================================
    constexpr float TEMPLE_SCALE = 27.0f;               // Scale factor for temple model
    constexpr float TEMPLE_ROTATION_Y = 180.0f;         // Y-axis rotation in degrees
    constexpr float TEMPLE_POSITION_X = 1000.0f;        // World X position
    constexpr float TEMPLE_POSITION_Z = 1500.0f;        // World Z position
    constexpr float TEMPLE_POSITION_Y = 359.974f;       // World Y position (terrain height + lift)

    // ========================================
    // Character Animation Settings
    // ========================================
    constexpr float CHARACTER_WALK_SPEED = 2.0f;        // Character walking speed (m/s)
    constexpr float CHARACTER_TURN_SPEED = 1.0f;        // Character turning speed (rad/s)
    constexpr float CHARACTER_SCALE = 80.0f;            // Character model scale (model is too small)
    constexpr float CHARACTER_GROUND_OFFSET = 5.0f;     // Height offset to prevent sinking into terrain
    constexpr float CHARACTER_MAX_DELTA_TIME = 0.1f;    // Clamp delta time to prevent large jumps
    constexpr int MAX_JOINT_COUNT = 128;                // Maximum skeletal animation joints

    // Character wandering area (grassland region)
    constexpr float CHARACTER_WANDER_MIN_X = 400.0f;
    constexpr float CHARACTER_WANDER_MAX_X = 1100.0f;
    constexpr float CHARACTER_WANDER_MIN_Z = 500.0f;
    constexpr float CHARACTER_WANDER_MAX_Z = 1100.0f;

    // ========================================
    // Scene Material Settings
    // ========================================
    constexpr float DEFAULT_REFLECTIVITY = 0.3f;        // Default reflectivity for scene objects

    // ========================================
    // Texture Unit Assignments
    // ========================================
    // These constants define which OpenGL texture unit each type of texture uses
    // This prevents conflicts and makes the binding logic clearer
    constexpr int TEXTURE_UNIT_DIFFUSE = 0;
    constexpr int TEXTURE_UNIT_SHADOW = 1;
    constexpr int TEXTURE_UNIT_CUBEMAP = 2;

    // Terrain-specific texture units
    constexpr int TEXTURE_UNIT_TERRAIN_BASE = 0;
    constexpr int TEXTURE_UNIT_TERRAIN_SAND = 1;
    constexpr int TEXTURE_UNIT_TERRAIN_ROCK = 2;
    constexpr int TEXTURE_UNIT_TERRAIN_MUD = 3;
    constexpr int TEXTURE_UNIT_TERRAIN_SHADOW = 4;

    // Water-specific texture units
    constexpr int TEXTURE_UNIT_WATER = 4;
    constexpr int TEXTURE_UNIT_WATER_BUMP = 5;
    constexpr int TEXTURE_UNIT_WATER_CUBEMAP = 6;

    // Grass texture unit
    constexpr int TEXTURE_UNIT_GRASS = 1;

    // ========================================
    // Transition System Settings
    // ========================================
    constexpr float TRANSITION_DURATION = 2.0f;         // Duration of time period transition (seconds)
    constexpr int MIN_FRAMES_BEFORE_INPUT = 10;         // Minimum frames to wait before accepting transition input
    constexpr float DEBUG_PRINT_INTERVAL = 0.5f;        // Interval for debug prints (seconds)

    // ========================================
    // Sun/Moon Rendering Settings
    // ========================================
    constexpr float SUN_ORBIT_RADIUS = 8000.0f;
    constexpr float SUN_MESH_SCALE = 150.0f;
    constexpr float SCENE_CENTER_X = 1000.0f;
    constexpr float SCENE_CENTER_Z = 1000.0f;

    // ========================================
    // Clear Color
    // ========================================
    constexpr float BACKGROUND_COLOR_R = 0.2f;
    constexpr float BACKGROUND_COLOR_G = 0.2f;
    constexpr float BACKGROUND_COLOR_B = 0.2f;
    constexpr float BACKGROUND_COLOR_A = 1.0f;

    // ========================================
    // Default Screen Settings
    // ========================================
    constexpr int DEFAULT_SCREEN_WIDTH = 1280;
    constexpr int DEFAULT_SCREEN_HEIGHT = 720;

    // ========================================
    // Performance Targets
    // ========================================
    constexpr float TARGET_FPS = 60.0f;                 // Target frame rate (coursework requirement)
    constexpr float TARGET_FRAME_TIME_MS = 1000.0f / TARGET_FPS;  // ~16.67ms per frame
}
