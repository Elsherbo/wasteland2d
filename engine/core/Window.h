#pragma once

#include <SDL.h>
#include <string>

namespace engine {

// Thin RAII wrapper around an SDL_Window + SDL_Renderer pair.
// Nothing game-specific lives here on purpose — this is framework code.
class Window {
public:
    Window(const std::string& title, int width, int height);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    SDL_Renderer* renderer() const { return renderer_; }
    SDL_Window* handle() const { return window_; }

    int width() const { return width_; }
    int height() const { return height_; }

    void clear(Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255);
    void present();

private:
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    int width_ = 0;
    int height_ = 0;
};

} // namespace engine
