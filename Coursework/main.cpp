#include "../nclgl/window.h"
#include "Renderer.h"
#include <iostream>

int main() {
    std::cout << "=== Application Starting ===" << std::endl;

    Window w("CSC8502 - The Temporal Courtyard", 1280, 720, false);

    if (!w.HasInitialised()) {
        std::cout << "ERROR: Window failed to initialize!" << std::endl;
        std::cout << "Press any key to exit..." << std::endl;
        std::cin.get();
        return -1;
    }
    std::cout << "Window initialized successfully." << std::endl;

    Renderer renderer(w);
    if (!renderer.HasInitialised()) {
        std::cout << "ERROR: Renderer failed to initialize!" << std::endl;
        std::cout << "Press any key to exit..." << std::endl;
        std::cin.get();
        return -1;
    }

    std::cout << "Starting main loop..." << std::endl;

    w.LockMouseToWindow(true);
    w.ShowOSPointer(false);

    while (w.UpdateWindow() && !Window::GetKeyboard()->KeyDown(KEYBOARD_ESCAPE)) {
        renderer.UpdateScene(w.GetTimer()->GetTimeDeltaSeconds());
        renderer.RenderScene();
        renderer.SwapBuffers();

        if (Window::GetKeyboard()->KeyDown(KEYBOARD_F5)) {
            Shader::ReloadAllShaders();
        }
    }

    std::cout << "Program exiting normally." << std::endl;
    return 0;
}
