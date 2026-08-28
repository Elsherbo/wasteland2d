#pragma once

#include <functional>
#include <memory>
#include <string>

#include "core/Time.h"
#include "core/Window.h"
#include "core/Logger.h"
#include "input/InputManager.h"

namespace engine {

struct ApplicationConfig {
    std::string title = "wasteland2d";
    int width = 1280;
    int height = 720;
    double fixedUpdateHz = 60.0;
    LoggerConfig loggerConfig; // Logger configuration
};

// Owns the SDL lifecycle, window, input, and the fixed-timestep loop.
// A specific game plugs in via the callbacks below rather than
// subclassing — keeps the framework/game boundary at a function
// pointer, not an inheritance hierarchy.
class Application {
public:
    // update: called at a fixed rate (dt is always fixedUpdateHz's period).
    //         Put gameplay simulation, physics, AI ticks here.
    // render: called once per real frame with interpolation alpha (0..1)
    //         for smooth motion between fixed steps.
    using UpdateFn = std::function<void(double fixedDt)>;
    using RenderFn = std::function<void(Window&, double alpha)>;

    explicit Application(ApplicationConfig config);
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    void setUpdateCallback(UpdateFn fn) { onUpdate_ = std::move(fn); }
    void setRenderCallback(RenderFn fn) { onRender_ = std::move(fn); }

    InputManager& input() { return input_; }
    Window& window() { return *window_; }

    void run();
    void quit() { quitting_ = true; }

private:
    ApplicationConfig config_;
    std::unique_ptr<Window> window_;
    InputManager input_;
    Clock clock_{60.0};
    bool quitting_ = false;

    UpdateFn onUpdate_;
    RenderFn onRender_;
};

} // namespace engine
