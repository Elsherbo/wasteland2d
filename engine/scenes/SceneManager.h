#pragma once

#include "scenes/Scene.h"
#include <memory>
#include <stack>
#include <mutex>

namespace engine {

// Manages scene transitions and lifecycle
class SceneManager {
public:
    SceneManager() = default;
    ~SceneManager() = default;
    
    // Delete copy operations
    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;
    
    // Load a new scene (replaces current)
    void loadScene(std::unique_ptr<Scene> scene);
    
    // Push a scene onto the stack (for pausing)
    void pushScene(std::unique_ptr<Scene> scene);
    
    // Pop the current scene (return to previous)
    void popScene();
    
    // Get the current scene
    Scene* currentScene();
    
    // Get the current scene (const)
    const Scene* currentScene() const;
    
    // Update the current scene
    void update(double dt);
    
    // Render the current scene
    void render();
    
    // Get scene stack size
    size_t sceneCount() const;
    
    // Clear all scenes
    void clear();

private:
    std::stack<std::unique_ptr<Scene>> sceneStack_;
    std::unique_ptr<Scene> pendingScene_;
    enum class PendingAction { None, Load, Push, Pop };
    PendingAction pendingAction_ = PendingAction::None;
};

} // namespace engine
