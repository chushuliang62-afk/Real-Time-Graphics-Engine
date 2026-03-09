#pragma once

#include <string>

// Forward declaration
class Window;

// ========================================
// FPS COUNTER
// ========================================
// Tracks frame rate and displays it in window title
// Keeps FPS tracking logic separate from Renderer

class FPSCounter {
public:
    FPSCounter(Window* window)
        : window(window), frameCount(0), fpsTimer(0.0f), fpsCounter(0), currentFPS(0) {}

    // Call this every frame with delta time
    void Update(float dt) {
        frameCount++;
        fpsCounter++;
        fpsTimer += dt;

        // Update FPS display every second
        if (fpsTimer >= 1.0f) {
            currentFPS = (int)(fpsCounter / fpsTimer);

            // Update window title with FPS
            if (window) {
                std::string title = "Graphics Coursework - FPS: " + std::to_string(currentFPS);
                if (currentFPS < 60) {
                    title += " [LOW]";
                }
                window->SetTitle(title);
            }

            // Reset counters
            fpsCounter = 0;
            fpsTimer = 0.0f;
        }
    }

    int GetCurrentFPS() const { return currentFPS; }
    int GetFrameCount() const { return frameCount; }

private:
    Window* window;      // Window to update title
    int frameCount;      // Total frames since start
    float fpsTimer;      // Timer for FPS calculation
    int fpsCounter;      // Frames counted in current second
    int currentFPS;      // Last calculated FPS
};
