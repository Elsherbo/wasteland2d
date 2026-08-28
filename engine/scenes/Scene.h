#pragma once

#include <string>

namespace engine {

// Forward declarations
namespace render {
    class Renderer;
}

// Base class for all scenes
// Scenes represent different game states (menu, game, pause, etc.)
class Scene {
public:
    virtual ~Scene() = default;
    
    // Called when scene is entered
    virtual void onEnter() = 0;
    
    // Called when scene is exited
    virtual void onExit() = 0;
    
    // Called every frame
    virtual void update(double dt) = 0;
    
    // Called every frame for rendering
    virtual void render(render::Renderer& renderer) = 0;
    
    // Get scene name (for debugging)
    virtual const char* getName() const = 0;
    
    // Check if scene is active
    bool isActive() const { return active_; }
    
    // Set active state
    void setActive(bool active) { active_ = active; }

protected:
    bool active_ = false;
};

} // namespace engine
