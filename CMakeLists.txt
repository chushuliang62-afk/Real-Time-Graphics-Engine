cmake_minimum_required(VERSION 3.15)
project(GraphicsTutorials LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ========================================
# MSVC-specific settings
# ========================================
if(MSVC)
    # Multi-byte character set (matches vcxproj)
    add_definitions(-D_MBCS)
    # Suppress PDB warnings
    add_link_options(/ignore:4099)
    # Parallel compilation
    add_compile_options(/MP)
else()
    # GCC/MinGW: this project was written for MSVC, allow some non-standard patterns
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fpermissive> $<$<COMPILE_LANGUAGE:CXX>:-Wno-conversion-null>)
endif()

# ========================================
# Third Party include directory
# ========================================
set(THIRD_PARTY_DIR "${CMAKE_SOURCE_DIR}/Third Party")

# ========================================
# Find OpenGL
# ========================================
find_package(OpenGL REQUIRED)

# ========================================
# SOIL library
# ========================================
if(MSVC)
    # Use prebuilt MSVC .lib
    add_library(SOIL STATIC IMPORTED)
    set_target_properties(SOIL PROPERTIES
        IMPORTED_LOCATION_DEBUG   "${THIRD_PARTY_DIR}/SOIL/x64/Debug/SOIL.lib"
        IMPORTED_LOCATION_RELEASE "${THIRD_PARTY_DIR}/SOIL/x64/Release/SOIL.lib"
        IMPORTED_LOCATION         "${THIRD_PARTY_DIR}/SOIL/x64/Debug/SOIL.lib"
    )
    target_include_directories(SOIL INTERFACE "${THIRD_PARTY_DIR}/SOIL")
else()
    # MinGW/GCC: compile SOIL from source (prebuilt .lib is MSVC-only)
    set(SOIL_SRC_DIR "${THIRD_PARTY_DIR}/SOIL/Simple OpenGL Image Library/src")
    add_library(SOIL STATIC
        "${SOIL_SRC_DIR}/SOIL.c"
        "${SOIL_SRC_DIR}/image_DXT.c"
        "${SOIL_SRC_DIR}/image_helper.c"
        "${SOIL_SRC_DIR}/stb_image_aug.c"
    )
    target_include_directories(SOIL PUBLIC "${SOIL_SRC_DIR}/include" "${SOIL_SRC_DIR}" "${THIRD_PARTY_DIR}/SOIL")
    target_link_libraries(SOIL PUBLIC OpenGL::GL)
endif()

# ========================================
# nclgl static library
# ========================================
set(NCLGL_SOURCES
    nclgl/Camera.cpp
    nclgl/ComputeShader.cpp
    nclgl/CustomTerrain.cpp
    nclgl/GameTimer.cpp
    nclgl/HeightMap.cpp
    nclgl/Keyboard.cpp
    nclgl/Light.cpp
    nclgl/Matrix2.cpp
    nclgl/Matrix3.cpp
    nclgl/Matrix4.cpp
    nclgl/Mesh.cpp
    nclgl/MeshAnimation.cpp
    nclgl/MeshMaterial.cpp
    nclgl/Mouse.cpp
    nclgl/OGLRenderer.cpp
    nclgl/Quaternion.cpp
    nclgl/SceneNode.cpp
    nclgl/Shader.cpp
    nclgl/Window.cpp
    nclgl/Extra/GLTFLoader.cpp
    nclgl/Extra/OGLTexture.cpp
    "Third Party/glad/glad.c"
)

add_library(nclgl STATIC ${NCLGL_SOURCES})

target_include_directories(nclgl PUBLIC
    ${CMAKE_SOURCE_DIR}/nclgl
    ${CMAKE_SOURCE_DIR}/nclgl/Extra
    ${CMAKE_SOURCE_DIR}/nclgl/Extra/stb
    "${THIRD_PARTY_DIR}"
    "${THIRD_PARTY_DIR}/glad"
    "${THIRD_PARTY_DIR}/KHR"
    "${THIRD_PARTY_DIR}/SOIL"
)

target_link_libraries(nclgl PUBLIC
    OpenGL::GL
    SOIL
)

# ========================================
# Coursework executable
# ========================================
set(COURSEWORK_SOURCES
    Coursework/main.cpp
    Coursework/Renderer.cpp
    Coursework/AnimalManager.cpp
    Coursework/BlockRenderer.cpp
    Coursework/DayNightCycle.cpp
    Coursework/DeferredRenderer.cpp
    Coursework/FBOManager.cpp
    Coursework/GBuffer.cpp
    Coursework/GrassRenderer.cpp
    Coursework/ParticleSystem.cpp
    Coursework/PointLight.cpp
    Coursework/ScenePass.cpp
    Coursework/ShadowSystem.cpp
    Coursework/SkeletalAnimator.cpp
    Coursework/TerrainSampler.cpp
    Coursework/TreeGenerator.cpp
    Coursework/TreeSystem.cpp
    Coursework/VoxelTreeSystem.cpp
    Coursework/WaterRenderer.cpp
)

add_executable(Coursework ${COURSEWORK_SOURCES})

target_include_directories(Coursework PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/Coursework
    ${CMAKE_SOURCE_DIR}/nclgl
    "${THIRD_PARTY_DIR}"
)

# Preprocessor definitions for asset paths (relative to working directory)
target_compile_definitions(Coursework PRIVATE
    SHADERDIR="Shaders/"
    TEXTUREDIR="Textures/"
    MESHDIR="Meshes/"
)

target_link_libraries(Coursework PRIVATE nclgl)

# ========================================
# Set working directory for debugging
# ========================================
if(MSVC)
    # Visual Studio uses this property for working directory
    set_target_properties(Coursework PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/x64/Debug"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/x64/Release"
    )
else()
    # CLion/MinGW: output exe to project root so relative asset paths work
    set_target_properties(Coursework PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
endif()
